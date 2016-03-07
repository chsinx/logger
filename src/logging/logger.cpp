#include <cstring>
#include <iostream>
#include <fstream>
#include <ctime>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>

#include "logger.h"

namespace logging {

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    #include <windows.h>
#else
    #include <sys/time.h>
#endif

int getMilliseconds() {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)

    static DWORD first = GetTickCount();
    return (GetTickCount() - first) % 1000;

#else

    static struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_usec / 1000;

#endif
}

void printLevelPrefix(char* buf, const Level level) {

    if( level == l_info )
        buf[0] = 'I';
    else if (level == l_error)
        buf[0] = 'E';
    else if(level == l_debug)
        buf[0] = 'D';
    else
        buf[0] = 'T';
    buf[1] = ' ';
}

std::string getLogFileName(const std::string& logFilePrefix, time_t fileTime = time(0)) {

    char timeFormatBuffer[80];

    const tm& timeInfo = *localtime( &fileTime );
    strftime(timeFormatBuffer, sizeof(timeFormatBuffer), "%Y%m%d", &timeInfo);

    return logFilePrefix + "_" + timeFormatBuffer + ".log";
}

bool fileExists(const std::string& fileName) {

    std::ifstream check(fileName);
    return check.good();
}

int getDayOfMonth(time_t unixTime) {
    return localtime( &unixTime )->tm_mday;
}

struct LogMessage {
    time_t msgTime;
    int milliseconds;
    const Level level;
    std::string msg;

    LogMessage(const std::stringstream& stream, const Level level_) :
        msgTime( time(nullptr) ),
        milliseconds( getMilliseconds() ),
        level( level_ ),
        msg( stream.str() )
    {
    }

    std::string getPrefix() const {

        char result[64];

        printLevelPrefix(result, level);
        const tm timeInfo = *localtime( &msgTime);
        size_t offset = 2+strftime(result+2, sizeof(result), "%Y.%m.%d %H:%M:%S", &timeInfo);
        std::sprintf(result+offset, ".%03d ", milliseconds);

        return result;
    }
};

class Logger::LoggerImpl {

public:

    LoggerImpl() {
        minLevel_ = l_trace;
        logDir_ = ".";
        logFilePrefix_ = "log";
        guiLogMessageHandler_ = nullptr;
        guiLogMessageHandlerData_ = nullptr;
        run_ = false;
        startFileThread();
    }

    void writeToFile(std::ofstream& file, const LogMessage& lm) {

        std::string prefix = lm.getPrefix();

        file << prefix << lm.msg << std::endl;

        if(guiLogMessageHandler_)
            guiLogMessageHandler_(guiLogMessageHandlerData_, prefix, lm.msg);
    }

    bool writeLogMessageQueueToFile(std::ofstream& file,
                                    std::queue<LogMessage> &lmQueue,
                                    int dayOfMonth,
                                    const std::string &logFilePrefix)
    {
        while(!lmQueue.empty() && file.good()) {
            LogMessage& lm = lmQueue.front();

            int newDayOfMonth = getDayOfMonth(lm.msgTime);

            //open new log file if day have passed
            if(newDayOfMonth > dayOfMonth && !openNewLogFile(file, logFilePrefix) ) {
                std::cerr << "Error opening log file"<< std::endl;
                return false;
            }

            writeToFile(file, lm);
            lmQueue.pop();

            dayOfMonth = newDayOfMonth;
        }
        return true;
    }

    void writeStream(std::stringstream &stream, Level level) {
        if(level >= minLevel_) {
            std::unique_lock<std::mutex> lk(writelock_);
            messageQueue_.emplace( LogMessage(stream, level) );
            if(messageQueue_.size() > 1024)
                cv_.notify_one();
        }
    }
    void setMessageHook(GuiLogMessageHandler handler, void *handlerData) {
        std::unique_lock<std::mutex> lk(writelock_);
        guiLogMessageHandlerData_ = handlerData;
        guiLogMessageHandler_ = handler;
    }

    ~LoggerImpl() {
        setMessageHook(nullptr, nullptr);
        run_ = false;
        if(worker_.joinable())
            worker_.join();
    }

    std::string getLogFilePrefix() {
        return logFilePrefix_;
    }
    void setLogFilePrefix(const std::string &logFilePrefix) {
        logFilePrefix_ = logFilePrefix;
    }

    std::string getLogDir() {
        return logDir_;
    }
    void setLogDir(const std::string &logDir) {

        //TODO
        logDir_ = logDir;
        if(!logDir_.empty()) {
            char lastChar = logDir_.at(logDir_.length() - 1);
            if(lastChar != '/' &&  lastChar != '\\')
                logDir_ += '/';
        }
    }

private:
    Level minLevel_;
    std::string logDir_;
    std::string logFilePrefix_;
    GuiLogMessageHandler guiLogMessageHandler_;
    void *guiLogMessageHandlerData_;
    bool run_;
    std::mutex writelock_;
    std::condition_variable cv_;
    std::thread worker_;
    std::queue<LogMessage> messageQueue_;

    void threadProc(const std::string& logFilePrefix) {

        std::ofstream file;

        //BUG file prefix check
        if(!openNewLogFile(file, logFilePrefix)) {
            std::cerr << "Error opening log file"<< std::endl;
            return;
        }

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

    void startFileThread() {
        run_ = true;
        worker_ = std::thread(&Logger::LoggerImpl::threadProc, this, logDir_ + logFilePrefix_);
    }

    bool openNewLogFile(std::ofstream& file, const std::string &logFilePrefix) {

        if(file.is_open())
            file.close();

        std::string filename = getLogFileName(logFilePrefix);

        file.open(filename, std::ios_base::app );

        return file.good();
    }
};

Logger &Logger::instance() {
    static Logger instance;
    return instance;
}
Logger::Logger() :
    d_(new LoggerImpl)
{
}

Logger::~Logger() {
    delete d_;
    d_=nullptr;
}

void Logger::setMessageHook(GuiLogMessageHandler handler, void *handlerData) {
    d_->setMessageHook(handler,handlerData);
}

void Logger::writeStream(std::stringstream &stream, Level level) {
    d_->writeStream(stream,level);
}

void Logger::setLogDir(const std::string &logDir) {
    d_->setLogDir(logDir);
}

std::string Logger::getLogDir() {
    return d_->getLogDir();
}

void Logger::setLogFilePrefix(const std::string &logFilePrefix) {
    d_->setLogFilePrefix(logFilePrefix);
}

std::string Logger::getFilePrefix() {
    return d_->getLogFilePrefix();
}







} //ns logging
