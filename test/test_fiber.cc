#include "../sylar/mysylar.h"
#include <vector>
mysylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

void run_in_fiber()
{
    SYLAR_LOG_INFO(g_logger) << "run in fiber begin";
    mysylar::Fiber::GetThis()->swapOut();
    SYLAR_LOG_INFO(g_logger) << "run in fiber end";
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
    SYLAR_LOG_INFO(g_logger) << "main end";
    return 0;
}