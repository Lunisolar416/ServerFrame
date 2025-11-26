#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__
#include "singleton.h"
#include "util.h"
#include <cstdarg>
#include <cstdint>
#include <fstream>
#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdint.h>
#include <string>
#include <vector>
#define SYLAR_LOG_LEVEL(logger, level)                                                             \
    if (logger->getLevel() <= level)                                                               \
    mysylar::LogEventWarp(mysylar::LogEvent::ptr(new mysylar::LogEvent(                            \
                              logger, level, __FILE__, __LINE__, 0, mysylar::getThreadId(),        \
                              mysylar::getFiberId(), time(0))))                                    \
        .getSS()

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, mysylar::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, mysylar::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, mysylar::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, mysylar::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, mysylar::LogLevel::FATAL)

#define SYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...)                                               \
    if (logger->getLevel() <= level)                                                               \
    mysylar::LogEventWarp(mysylar::LogEvent::ptr(new mysylar::LogEvent(                            \
                              logger, level, __FILE__, __LINE__, 0, mysylar::getThreadId(),        \
                              mysylar::getFiberId(), time(0))))                                    \
        .getEvent()                                                                                \
        ->format(fmt, __VA_ARGS__)

#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...)                                                      \
    SYLAR_LOG_FMT_LEVEL(logger, mysylar::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...)                                                       \
    SYLAR_LOG_FMT_LEVEL(logger, mysylar::LogLevel::INFO, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...)                                                       \
    SYLAR_LOG_FMT_LEVEL(logger, mysylar::LogLevel::WARN, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...)                                                      \
    SYLAR_LOG_FMT_LEVEL(logger, mysylar::LogLevel::ERROR, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_FATAL(logger, fmt, ...)                                                      \
    SYLAR_LOG_FMT_LEVEL(logger, mysylar::LogLevel::FATAL, fmt, __VA_ARGS__)

#define SYLAR_LOG_ROOT() mysylar::loggerMgr::GetInstance()->getRoot()

namespace mysylar
{
class Logger;
class LogLevel
{
  public:
    /**
     * @brief 日志级别枚举
     */
    enum Level
    {
        // 未知级别
        UNKNOW = 0,
        // DEBUG 级别
        DEBUG = 1,
        // INFO 级别
        INFO = 2,
        // WARN 级别
        WARN = 3,
        // ERROR 级别
        ERROR = 4,
        // FATAL 级别
        FATAL = 5
    };
    /**
     * @brief 将日志级别转换为文本
     *@param[in] level 级别
     */
    static const char* ToString(LogLevel::Level level);
    /**
     * @brief 将文本转为日志级别
     *@param[in] str 日志级别版本
     */
    static LogLevel::Level FromString(const std::string& str);
};

/**
 * @brief 日志事件
 *
 */
class LogEvent
{
  public:
    typedef std::shared_ptr<LogEvent> ptr;

    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* filename,
             int32_t line, uint32_t elapse, int32_t thread_id, int32_t fiber_id, uint64_t time);

    const char* getFile() const
    {
        return m_file;
    }

    int32_t getLine() const
    {
        return m_line;
    }
    uint32_t getElapse() const
    {
        return m_elapse;
    }
    int32_t getThreadId() const
    {
        return m_threadid;
    }
    int32_t getFiberId() const
    {
        return m_fiberid;
    }
    uint64_t getTime() const
    {
        return m_time;
    }
    LogLevel::Level getLevel() const
    {
        return m_level;
    }
    const std::string& getThreadName() const
    {
        return m_threadname;
    }
    std::string getContent() const
    {
        return m_ss.str();
    }
    std::stringstream& getSS()
    {
        return m_ss;
    }
    std::shared_ptr<Logger> getLogger() const
    {
        return m_logger;
    }

    void format(const char* format, ...);
    void format(const char* format, va_list al);

  private:
    std::shared_ptr<Logger> m_logger; // 日志器
    LogLevel::Level m_level;          // 日志级别
    const char* m_file = nullptr;     // 文件名
    int32_t m_line = 0;               // 行号
    uint32_t m_elapse = 0;            // 程序启动开始到现在的毫秒数
    int32_t m_threadid = 0;           // 线程id
    int32_t m_fiberid = 0;            // 协程id
    uint64_t m_time;                  // 时间戳
    std::string m_threadname;         // 线程名称
    std::stringstream m_ss;           // 日志内容
};

class LogEventWarp
{
  public:
    LogEventWarp(LogEvent::ptr e);
    ~LogEventWarp();

    LogEvent::ptr getEvent() const
    {
        return m_event;
    }
    std::stringstream& getSS();

  private:
    LogEvent::ptr m_event;
};

/**
 * @brief 日志格式化器
 *
 */
class LogFormatter
{
  public:
    typedef std::shared_ptr<LogFormatter> ptr;
    /**
     * @brief 构造函数
     *
     * @param[in] pattern 格式模板
     * @details
     *  %m 消息
     *  %p 日志级别
     *  %r 累计毫秒数
     *  %c 日志名称
     *  %t 线程id
     *  %n 换行
     *  %d 时间
     *  %f 文件名
     *  %l 行号
     *  %T 制表符
     *  %F 协程id
     *  %N 线程名称
     *
     *  默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
     */
    LogFormatter(const std::string& pattern);
    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

  public:
    /**
     * @brief 日志格式化项
     *
     */
    class FormatItem
    {
      public:
        typedef std::shared_ptr<FormatItem> ptr;
        ~FormatItem() {}
        virtual void format(std::shared_ptr<Logger> logger, std::ostream& os, LogLevel::Level level,
                            LogEvent::ptr event) = 0;
    };

    /**
     * @brief 初始化，解析日志模板
     *
     */
    void init();

  private:
    std::string m_pattern;
    bool m_error = false;
    std::vector<FormatItem::ptr> m_items;
};
/**
 * @brief 日志输出地
 *
 */
class LogAppender
{
  public:
    typedef std::shared_ptr<LogAppender> ptr;
    virtual ~LogAppender() = default;

    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                     LogEvent::ptr event) = 0;

    LogFormatter::ptr getFormatter() const;
    void setFormatter(LogFormatter::ptr formatter);

    LogLevel::Level getLevel() const
    {
        return m_level;
    };
    void setLevel(LogLevel::Level level)
    {
        m_level = level;
    };

  protected:
    LogLevel::Level m_level = LogLevel::DEBUG;
    LogFormatter::ptr m_formatter;
};

/**
 * @brief 日志输出器
 *
 */
class Logger : public std::enable_shared_from_this<Logger>
{
  public:
    typedef std::shared_ptr<Logger> ptr;

    Logger(const std::string& name = "root");
    void log(LogLevel::Level level, LogEvent::ptr event);

    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    LogLevel::Level getLevel() const
    {
        return m_level;
    };
    void setLevel(LogLevel::Level level)
    {
        m_level = level;
    };
    const std::string& getName() const
    {
        return m_name;
    }

  private:
    std::string m_name;                      // 日志名称
    LogLevel::Level m_level;                 // 日志级别
    std::list<LogAppender::ptr> m_appenders; // 输出到目的地的集合
    LogFormatter::ptr m_formatter;           // 日志格式器
};

/**
 * @brief 输出到控制台的Appender
 *
 */
class StdoutLogAppender : public LogAppender
{
  public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;

  private:
    LogLevel::Level m_level;
};

/**
 * @brief 输出到文件的Appender
 *
 */
class FileLogAppender : public LogAppender
{
  public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string& filename);
    void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;

    // 重新打开文件
    bool reopen();

  private:
    std::string m_filename;
    std::ofstream m_filestream;
};

class LoggerManger
{
  public:
    LoggerManger();
    Logger::ptr getLogger(const std::string& name);

    void init();
    Logger::ptr getRoot() const
    {
        return m_root;
    }

  private:
    std::map<std::string, Logger::ptr> m_logger;
    Logger::ptr m_root;
};

typedef Singleton<LoggerManger> loggerMgr;

}; // namespace mysylar

#endif //__SYLAR_LOG_H__