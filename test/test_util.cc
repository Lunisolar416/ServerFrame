#include "../sylar/mysylar.h"

mysylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();
void test_assert()
{
    SYLAR_LOG_INFO(g_logger) << "test assert";
    SYLAR_ASSERT(false);
}
int main()
{
    test_assert();
    return 0;
}