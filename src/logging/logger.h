#ifndef LOGGER_H
#define LOGGER_H

#include <sstream>

#ifdef QT_CORE_LIB
#include <QString>
#endif

namespace logging {

///Logging levels
enum Level
{
    l_trace = -2,
    l_debug = -1,
    l_info = 0,
    l_error = 1
};

/// Handler for gui log message realtime output
typedef void(*GuiLogMessageHandler)(void* data, const std::string& prefix, const std::string& msg);

/// Controls life time of internal logging thread
class Logger
{
public:
    static Logger& instance();
    ~Logger();

    void setMessageHandler(GuiLogMessageHandler handler, void *handlerData);
    void writeStream(std::stringstream& stream, Level level);

    void setLogDir(const std::string& logDir);
    std::string getLogDir();
    void setLogFilePrefix(const std::string& logFilePrefix);
    std::string getFilePrefix();
    Level getMinLevel() const;
    void setMinLevel(const Level minLevel);

private:
    Logger();
    Logger(const Logger&);
    Logger(const Logger&&);
    Logger& operator=(const Logger&);
    Logger& operator=(const Logger&&);

    class LoggerImpl;
    LoggerImpl* d_;
};

/// Helper class for simple syntax logging.
/// Feel free to add overloads for operator << for unsupported types.
template <Level level>
class StreamLoggerHelper {
public:
    StreamLoggerHelper() {}

#ifdef QT_CORE_LIB
    StreamLoggerHelper& operator<< (const QString& str) { stream << str.toStdString(); return *this; }
    StreamLoggerHelper& operator<< (const QByteArray& str) { stream << QString(str).toStdString(); return *this; }
#else
    StreamLoggerHelper& operator<< (const std::string& str) { stream << str; return *this; }
#endif

    ///general logging method
    template<class T>
    StreamLoggerHelper& operator << (const T& msg)      { stream << msg;               return *this; }

    ///template ctor
    template<class T>
    StreamLoggerHelper(const T& str) { *this << str; }

    ///dtor flushes stream contents to logging queue using writeStream() method
    ~StreamLoggerHelper()                               { Logger::instance().writeStream(stream, level); }

private:
    std::stringstream stream;

    //disable copying
    StreamLoggerHelper(const StreamLoggerHelper&);
    StreamLoggerHelper& operator=(const StreamLoggerHelper&);
};

typedef StreamLoggerHelper<l_trace> trace;
typedef StreamLoggerHelper<l_debug> debug;
typedef StreamLoggerHelper<l_info>  info;
typedef StreamLoggerHelper<l_error> error;

} //ns logging



#endif //LOGGER_H
