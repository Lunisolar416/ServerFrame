#include "log.h"
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <tuple>
#include <vector>
namespace mysylar
{

const char* LogLevel::ToString(LogLevel::Level level)
{
    switch (level)
    {
#define XX(name)                                                                                   \
    case LogLevel::name:                                                                           \
        return #name;                                                                              \
        break;
        XX(DEBUG);
        XX(INFO);
        XX(WARN);
        XX(ERROR);
        XX(FATAL);
#undef XX
    default:
        return "UNKNOW";
    }
}
LogLevel::Level LogLevel::FromString(const std::string& str)
{
#define XX(level, v)                                                                               \
    if (str == #v)                                                                                 \
    {                                                                                              \
        return LogLevel::level;                                                                    \
    }
    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);

    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);
    return LogLevel::UNKNOW;
#undef XX
}
LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* filename,
                   int32_t line, uint32_t elapse, int32_t thread_id, int32_t fiber_id,
                   uint64_t time)
    : m_logger(logger), m_level(level), m_file(filename), m_line(line), m_elapse(elapse),
      m_threadid(thread_id), m_fiberid(fiber_id), m_time(time)
{
}

void LogEvent::format(const char* fmt, ...)
{
    va_list al;
    va_start(al, fmt);
    format(fmt, al);
    va_end(al);
}
void LogEvent::format(const char* fmt, va_list al)
{
    char* buf = nullptr;
    int len = vasprintf(&buf, fmt, al);
    if (len != -1)
    {
        m_ss << std::string(buf, len);
        free(buf);
    }
}
LogEventWarp::LogEventWarp(LogEvent::ptr e) : m_event(e) {}
LogEventWarp::~LogEventWarp()
{
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}
std::stringstream& LogEventWarp::getSS()
{
    return m_event->getSS();
}
Logger::Logger(const std::string& name) : m_name(name), m_level(LogLevel::DEBUG)
{
    m_formatter.reset(
        new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}
void Logger::addAppender(LogAppender::ptr appender)
{
    if (!appender->getFormatter())
    {
        appender->setFormatter(m_formatter);
    }
    m_appenders.push_back(appender);
}
void Logger::delAppender(LogAppender::ptr appender)
{
    for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it)
    {
        if (*it == appender)
        {
            m_appenders.erase(it);
            break;
        }
    }
}
void Logger::log(LogLevel::Level level, LogEvent::ptr event)
{
    if (level >= m_level)
    {
        auto self = shared_from_this();
        for (auto& i : m_appenders)
        {
            i->log(self, level, event);
        }
    }
}

void Logger::debug(LogEvent::ptr event)
{
    log(LogLevel::DEBUG, event);
}
void Logger::info(LogEvent::ptr event)
{
    log(LogLevel::INFO, event);
}
void Logger::warn(LogEvent::ptr event)
{
    log(LogLevel::WARN, event);
}
void Logger::error(LogEvent::ptr event)
{
    log(LogLevel::ERROR, event);
}
void Logger::fatal(LogEvent::ptr event)
{
    log(LogLevel::FATAL, event);
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                            LogEvent::ptr event)
{
    if (level >= m_level)
    {
        std::cout << m_formatter->format(logger, level, event);
    }
}
FileLogAppender::FileLogAppender(const std::string& filename) : m_filestream(filename) {}
void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                          LogEvent::ptr event)
{
    if (level >= m_level)
    {
        m_filestream << m_formatter->format(logger, level, event);
    }
}
bool FileLogAppender::reopen()
{
    if (m_filestream)
    {
        m_filestream.close();
    }
    m_filestream.open(m_filename);
    return !!m_filestream;
}

class MessageFormatItem : public LogFormatter::FormatItem
{
  public:
    MessageFormatItem(const std::string& str = "") {}
    void format(std::shared_ptr<Logger> logger, std::ostream& os, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << event->getContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem
{
  public:
    LevelFormatItem(const std::string& str = "") {}
    void format(std::shared_ptr<Logger> logger, std::ostream& os, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << LogLevel::ToString(level);
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem
{
  public:
    ElapseFormatItem(const std::string& str = "") {}
    void format(std::shared_ptr<Logger> logger, std::ostream& os, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << event->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem
{
  public:
    NameFormatItem(const std::string& str = "") {}
    void format(std::shared_ptr<Logger> logger, std::ostream& os, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << logger->getName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem
{
  public:
    ThreadIdFormatItem(const std::string& str = "") {}
    void format(std::shared_ptr<Logger> logger, std::ostream& os, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << event->getThreadId();
    }
};

class ThreadNameFormatItem : public LogFormatter::FormatItem
{
  public:
    ThreadNameFormatItem(const std::string& str = "") {}
    void format(std::shared_ptr<Logger> logger, std::ostream& os, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << event->getThreadName();
    }
};

class FiberFormatItem : public LogFormatter::FormatItem
{
  public:
    FiberFormatItem(const std::string& str = "") {}
    void format(std::shared_ptr<Logger> logger, std::ostream& os, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << event->getFiberId();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem
{
  public:
    DateTimeFormatItem(const std::string& format = "%Y:%m:%d %H:%M:%S") : m_formatter(format)
    {
        if (m_formatter.empty())
        {
            m_formatter = "%Y-%m-%d %H:%M:%S";
        }
    }
    void format(std::shared_ptr<Logger> logger, std::ostream& os, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), m_formatter.c_str(), &tm);
        os << buf;
    }

  private:
    std::string m_formatter;
};
class FilenameFormatItem : public LogFormatter::FormatItem
{
  public:
    FilenameFormatItem(const std::string& str = "") {}
    void format(std::shared_ptr<Logger> logger, std::ostream& os, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem
{
  public:
    LineFormatItem(const std::string& str = "") {}
    void format(std::shared_ptr<Logger> logger, std::ostream& os, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem
{
  public:
    NewLineFormatItem(const std::string& str = "") {}
    void format(std::shared_ptr<Logger> logger, std::ostream& os, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter::FormatItem
{
  public:
    StringFormatItem(const std::string& str) : m_string(str) {}
    void format(std::shared_ptr<Logger> logger, std::ostream& os, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << m_string;
    }

  private:
    std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem
{
  public:
    TabFormatItem(const std::string& str = "") {}
    void format(std::shared_ptr<Logger> logger, std::ostream& os, LogLevel::Level level,
                LogEvent::ptr event) override
    {
        os << "\t";
    }
};

LogFormatter::LogFormatter(const std::string& pattern) : m_pattern(pattern)
{
    init();
}
std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level,
                                 LogEvent::ptr event)
{
    std::stringstream ss;
    for (auto& i : m_items)
    {
        i->format(logger, ss, level, event);
    }
    return ss.str();
}

LogFormatter::ptr LogAppender::getFormatter() const
{
    return m_formatter;
}
void LogAppender::setFormatter(LogFormatter::ptr formatter)
{
    m_formatter = formatter;
}

//%xxx %xxx{xxx} %%
void LogFormatter::init()
{
    // str, format, type
    std::vector<std::tuple<std::string, std::string, int>> vec;
    std::string nstr;
    for (size_t i = 0; i < m_pattern.size(); ++i)
    {
        if (m_pattern[i] != '%')
        {
            nstr.append(1, m_pattern[i]);
            continue;
        }
        size_t n = i + 1;
        int fmt_status = 0;
        size_t fmt_begin = 0; // 0普通格式标识状态（还没进入 {}），1解析 {} 内的格式

        std::string str;
        std::string fmt;

        // 解析%xxx %xxx{xxx}格式
        while (n < m_pattern.size())
        {
            //%xxx格式解析
            if (!fmt_status &&
                (!isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}'))
            {
                str = m_pattern.substr(i + 1, n - i - 1);
                break;
            }
            //%xxx{xxx} 格式解析
            if (fmt_status == 0)
            {
                if (m_pattern[n] == '{')
                {
                    // 这时候已经解析到{，要进入字串解析了。
                    str = m_pattern.substr(i + 1, n - i - 1); // 取出%xxx
                    fmt_status = 1;
                    fmt_begin = n;
                    ++n;
                    continue;
                }
            }
            else if (fmt_status == 1)
            {
                if (m_pattern[n] == '}')
                {
                    // 这时候字串解析完毕
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    fmt_status = 0; // 重新回到普通格式标识状态
                    ++n;
                    break;
                }
            }
            ++n;
            if (n == m_pattern.size())
            {
                if (str.empty())
                {
                    // 就说明一直没有格式符号
                    str = m_pattern.substr(i + 1); // 全部字符放入str中
                }
            }
        }

        if (fmt_status == 0)
        {
            if (!nstr.empty())
            {
                vec.push_back(std::make_tuple(nstr, std::string(), 0)); // 普通字符串
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        }
        else if (fmt_status == 1)
        {
            // 就说明一直没有遇到}，说明格式不完整，错误
            std::cout << "pattern parse error:" << m_pattern << " - " << m_pattern.substr(i)
                      << std::endl;
            m_error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }
    } // end for
    if (!nstr.empty())
    {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }

    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)>>
        s_format_items = {
#define XX(str, C)                                                                                 \
    {                                                                                              \
        #str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt)); }                   \
    }
            XX(m, MessageFormatItem),   // m 消息
            XX(p, LevelFormatItem),     // p 日志级别
            XX(r, ElapseFormatItem),    // r 累计毫秒数
            XX(c, NameFormatItem),      // c 日志名称
            XX(t, ThreadIdFormatItem),  // t 线程id
            XX(n, NewLineFormatItem),   // n 换行
            XX(d, DateTimeFormatItem),  // d 时间
            XX(f, FilenameFormatItem),  // f 文件名
            XX(l, LineFormatItem),      // l 行号
            XX(T, TabFormatItem),       // T 制表符
            XX(F, FiberFormatItem),     // F 协程id
            XX(N, ThreadNameFormatItem) // N 线程名称
#undef XX
        };

    for (auto& i : vec)
    {
        if (std::get<2>(i) == 0)
        {
            // 说明是普通字符串
            m_items.push_back(LogFormatter::FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        }
        else
        {
            auto it = s_format_items.find(std::get<0>(i));
            if (it == s_format_items.end())
            {
                // 说明没有找到对应的格式化符号
                m_items.push_back(LogFormatter::FormatItem::ptr(
                    new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                m_error = true;
            }
            else
            {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }
    }
}

LoggerManger::LoggerManger()
{
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
}

Logger::ptr LoggerManger::getLogger(const std::string& name)
{
    auto it = m_logger.find(name);
    return it == m_logger.end() ? m_root : it->second;
}

void LoggerManger::init() {}

}; // namespace mysylar
