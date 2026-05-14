#include "../sylar/mysylar.h"
#include <vector>
mysylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

void run_in_fiber()
{
    SYLAR_LOG_INFO(g_logger) << "run in fiber begin";
    mysylar::Fiber::GetThis()->back();
    // 运行完这个就该返回到主线程里
    SYLAR_LOG_INFO(g_logger) << "run in fiber end"; // 运行完这个，子协程就会结束
}
void test_fiber()
{
    SYLAR_LOG_INFO(g_logger) << "test_fiber begin";
    mysylar::Fiber::ptr fiber(new mysylar::Fiber(run_in_fiber));
    // SYLAR_LOG_INFO(g_logger) << "create fiber";
    fiber->swapIn();
    SYLAR_LOG_INFO(g_logger) << "test_fiber after swapIn";
    fiber->swapIn();
    SYLAR_LOG_INFO(g_logger) << "test_fiber after end";
}
int main()
{
    /*
    mysylar::Thread::setName("Main Thread");
    SYLAR_LOG_INFO(g_logger) << "main begin";
    std::vector<mysylar::Thread::ptr> thrs;
    for (int i = 0; i < 3; ++i)
    {
        thrs.push_back(
            mysylar::Thread::ptr(new mysylar::Thread(test_fiber, "name_" + std::to_string(i + 1))));
    }
    for (auto i : thrs)
    {
        i->join();
    }
    SYLAR_LOG_INFO(g_logger) << "main end";*/
    SYLAR_LOG_INFO(g_logger) << "test_fiber begin";
    mysylar::Fiber::
        GetThis(); // 初始，t_fiber
                   // 不存在，getthis初始化了main_fiber,main_fiber只保存ctx,t_threadFiber
                   // = main_fiber
    mysylar::Fiber::ptr fiber(new mysylar::Fiber(run_in_fiber));

    SYLAR_LOG_DEBUG(g_logger) << "create fiber";
    fiber->call(); // 通过call函数里面的SetThis(),将t_fiber = fiber,然后就swap
    SYLAR_LOG_INFO(g_logger) << "test_fiber after swapIn";
    fiber->call();
    SYLAR_LOG_INFO(g_logger) << "test_fiber after end";
    return 0;
}