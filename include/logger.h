#pragma once

#include <sstream>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <ctime>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    #include <windows.h>
#else
    #include <sys/time.h>
#endif

namespace logging {

/// Logging levels can be used for runtime message filtering
enum class Level
{
    LTRACE = -2,
    LDEBUG = -1,
    LINFO = 0,
    LWARNING = 1,
    LERROR = 2,
    LFATAL = 3
};

class LogMessage {
public:
    LogMessage(const std::stringstream& stream, const Level level) :
        time_( time(nullptr) ),
        milliseconds_( getMilliseconds() ),
        level_( level ),
        text_( stream.str() )
    {
    }

    std::string getPrefix() const {
        char result[64];
        setLevelPrefix(result, level_);
        const tm timeInfo = *localtime( &time_);
        size_t offset = 2+strftime(result+2, sizeof(result), "%Y.%m.%d %H:%M:%S", &timeInfo);
        std::sprintf(result+offset, ".%03d ", milliseconds_);
        return result;
    }

    std::string getText() const { return text_; }

    time_t getTime() const { return time_; }

private:

    void setLevelPrefix(char* buf, const Level level) const {
        switch(level) {
        case Level::LINFO:
            *buf = 'I';
            break;
        case Level::LERROR:
            *buf = 'E';
            break;
        case Level::LDEBUG:
            *buf = 'D';
            break;
        case Level::LTRACE:
            *buf = 'T';
            break;
        default:
            *buf = '*';
            break;
        }
        buf[1] = ' ';
    }

    int getMilliseconds() const {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
        static const DWORD first = GetTickCount();
        return (GetTickCount() - first) % 1000;
#else
        struct timeval tv;
        gettimeofday(&tv, 0);
        return tv.tv_usec / 1000;
#endif
    }

    time_t time_;
    int milliseconds_;
    const Level level_;
    std::string text_;
};

/// External handler for log messages
typedef void(*logging_callback_t)(void* data, const std::string& prefix, const std::string& msg);

/// Logger singletone. Controls life time of internal logging thread
class Logger {
public:

    ///set callback for custom logging widget etc
    void setMessageHandler(logging_callback_t handler, void *handlerData) {
        std::unique_lock<std::mutex> lk(writelock_);
        externalHandlerData_ = handlerData;
        externalMessageHandler_ = handler;
    }

    ///write stringstream to log. stream will be cleared
    void writeMessage(std::stringstream& stream, Level level)  {
        if(level >= minLevel_) {
            std::unique_lock<std::mutex> lk(writelock_);
            messageQueue_.emplace( LogMessage(stream, level) );
            if(messageQueue_.size() > 1024)
                cv_.notify_one();
        }
    }

    ///directory where all logs are placed
    void setOutputDirectory(const std::string& logDir) {
        stopFileThread();

        std::unique_lock<std::mutex> lk(writelock_);
        logDir_ = logDir;
        if(!logDir_.empty()) {
            char lastChar = logDir_.at(logDir_.length() - 1);
            if(lastChar != '/' &&  lastChar != '\\')
                logDir_ += '/';
        }
        lk.unlock();

        startFileThread();
    }

    std::string getOutputDirectory() {
        std::unique_lock<std::mutex> lk(writelock_);
        return logDir_;
    }

    ///Log file names are made of <prefix>_yyyymmdd.log
    void setBaseFileName(const std::string& logFilePrefix) {
        stopFileThread();

        std::unique_lock<std::mutex> lk(writelock_);
        logFilePrefix_ = logFilePrefix;
        lk.unlock();

        startFileThread();
    }

    std::string getBaseFileName() {
        std::unique_lock<std::mutex> lk(writelock_);
        return logFilePrefix_;
    }

    ///Minimum logging level threshold. All messages which have level less than  will be dropped.
    void setMinLevel(const Level minLevel) {
        std::unique_lock<std::mutex> lk(writelock_);
        minLevel_ = minLevel;
    }

    Level getMinLevel() {
        std::unique_lock<std::mutex> lk(writelock_);
        return minLevel_;
    }

    static Logger& instance() { static Logger instance; return instance; }

    ~Logger() {
        setMessageHandler(nullptr, nullptr);
        stopFileThread();
    }

private:

    Logger() :
        minLevel_(Level::LTRACE)
       ,logFilePrefix_("log")
       ,externalMessageHandler_(nullptr)
       ,externalHandlerData_(nullptr)
       ,run_(false)
    {
        setOutputDirectory(".");
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;


    void startFileThread() {
        run_ = true;
        worker_ = std::thread(&Logger::threadProc, this, logDir_ + logFilePrefix_);
    }

    void stopFileThread() {
        //mutex must be unlocked here
        run_ = false;
        if(worker_.joinable())
            worker_.join();
    }

    std::string getLogFileName(const std::string& logFilePrefix, time_t fileTime = time(0)) {
        char timeFormatBuffer[80];

        const tm& timeInfo = *localtime( &fileTime );
        strftime(timeFormatBuffer, sizeof(timeFormatBuffer), "%Y%m%d", &timeInfo);

        return logFilePrefix + "_" + timeFormatBuffer + ".log";
    }

    int getDayOfMonth(time_t unixTime) {
        // localtime is not thread safe
        return localtime( &unixTime )->tm_mday;
    }

    void threadProc(const std::string& logFilePrefix) {
        std::ofstream file;

        int dayOfMonth = getDayOfMonth( time(0) );

        while(run_) {
            std::unique_lock<std::mutex> lk(writelock_);
            cv_.wait_for(lk,
                        std::chrono::milliseconds(300),
                        [&]{ return run_ && !messageQueue_.empty(); }
            );

            if(messageQueue_.empty())
                continue;

            std::queue<LogMessage> tmpQueue;
            tmpQueue.swap(messageQueue_);

            lk.unlock();

            if(!writeLogMessageQueueToFile(file, tmpQueue, dayOfMonth, logFilePrefix))
                return;
        }

        //flush the messages left in queue
        std::unique_lock<std::mutex> lk(writelock_);
        writeLogMessageQueueToFile(file, messageQueue_, dayOfMonth, logFilePrefix);
        file.close();
    }


    bool openLogFile(std::ofstream& file, const std::string &logFilePrefix) {
        if(file.is_open())
            file.close();

        std::string filename = getLogFileName(logFilePrefix);

        file.open(filename, std::ios_base::app );

        return file.good();
    }

    void writeToFile(std::ofstream& file, const LogMessage& lm) {
        const std::string& prefix = lm.getPrefix();
        const std::string& text = lm.getText();

        file << prefix << text << std::endl;

        if(externalMessageHandler_)
            externalMessageHandler_(externalHandlerData_, prefix, text);
    }

    bool writeLogMessageQueueToFile(std::ofstream& file,
                                    std::queue<LogMessage> &lmQueue,
                                    int dayOfMonth,
                                    const std::string &logFilePrefix)
    {
        while(!lmQueue.empty()) {
            LogMessage& lm = lmQueue.front();

            //open new log file if day have passed
            if (!file.is_open()
                || !file.good()
                || getDayOfMonth(lm.getTime()) > dayOfMonth)
            {
                if (!openLogFile(file, logFilePrefix)) {
                    std::string err_msg = "Error opening log file";
                    std::cerr << err_msg << std::endl;
                    throw std::runtime_error(err_msg);
                }
            }

            writeToFile(file, lm);
            lmQueue.pop();
        }
        return true;
    }

    std::mutex writelock_;

    Level minLevel_;
    std::string logDir_;
    std::string logFilePrefix_;
    logging_callback_t externalMessageHandler_;
    void *externalHandlerData_;
    bool run_;
    std::condition_variable cv_;
    std::thread worker_;
    std::queue<LogMessage> messageQueue_;
};


template <Level Level>
class StreamWriter {
public:
    StreamWriter() {}
    ~StreamWriter() { Logger::instance().writeMessage(stream, Level); }
    StreamWriter& operator << (const std::string& str) { stream << str; return *this; }

    template<class T>
    StreamWriter& operator << (const T& msg) { stream << msg; return *this; }

    template<class T>
    StreamWriter(const T& msg) { *this << msg; }

private:
    StreamWriter(const StreamWriter&) = delete;
    StreamWriter& operator=(const StreamWriter&) = delete;

    std::stringstream stream;
};

typedef StreamWriter<Level::LTRACE>     trace;
typedef StreamWriter<Level::LDEBUG>     debug;
typedef StreamWriter<Level::LINFO>      info;
typedef StreamWriter<Level::LWARNING>   warning;
typedef StreamWriter<Level::LERROR>     error;
typedef StreamWriter<Level::LFATAL>     fatal;

} //ns logging
