#include "../sylar/mysylar.h"
#include <iostream>
mysylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

void fiber_func()
{
    SYLAR_LOG_INFO(g_logger) << "fiber start, thread_id = " << mysylar::GetThreadId()
                             << " fiber id = " << mysylar::GetFiberId();
}

int main()
{
    mysylar::Scheduler sc(3, false, "worker");
    // 3个线程 + use_caller = true（主线程参与调度）

    sc.start();

    // 提交多个协程任务
    for (int i = 0; i < 4; ++i)
    {
        sc.schedule(&fiber_func);
    }
    sleep(1);

    sc.stop();
    return 0;
}