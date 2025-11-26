#include "../sylar/log.h"
#include "../sylar/util.h"
using namespace mysylar;
int main()
{
    // LogEvent::ptr event(new LogEvent(__FILE__, __LINE__, 1224, getThreadId(), getFiberId(),
    // time(0),
    //                                  LogLevel::DEBUG));
    // event->getSS() << "hello this is mysylar log event";
    // Logger::ptr logger(new Logger("test"));
    // logger->addAppender(LogAppender::ptr(new StdoutLogAppender));
    // logger->log(LogLevel::DEBUG, event);
    Logger::ptr logger(new Logger());
    logger->addAppender(LogAppender::ptr(new StdoutLogAppender));

    LogAppender::ptr filelogappend(new FileLogAppender("./log.txt"));

    filelogappend->setLevel(LogLevel::ERROR);
    logger->addAppender(filelogappend);

    SYLAR_LOG_INFO(logger) << "this is a info message";
    SYLAR_LOG_ERROR(logger) << "this is a error message";
    SYLAR_LOG_FMT_ERROR(logger, "test macor fmt error %s", "bb");

    auto l = loggerMgr::GetInstance()->getLogger("xx");
    SYLAR_LOG_INFO(l) << "XX";

    return 0;
}