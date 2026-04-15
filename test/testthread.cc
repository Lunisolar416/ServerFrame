#include "../sylar/thread.h"
#include "../sylar/util.h"
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
void func1()
{
    std::cout << "name:" << mysylar::Thread::GetName() << " this.name "
              << mysylar::Thread::GetThis()->getName() << " id:" << mysylar::GetThreadId()
              << " this.id:" << mysylar::Thread::GetThis()->getId() << std::endl;
}

int main()
{
    std::vector<mysylar::Thread::ptr> thrs;
    for (int i = 0; i < 5; ++i)
    {
        mysylar::Thread::ptr thr(new mysylar::Thread(&func1, "thread_" + std::to_string(i)));
        thrs.push_back(thr);
    }
    for (size_t i = 0; i < thrs.size(); ++i)
    {
        thrs[i]->join();
    }
    std::cout << "test thread end" << std::endl;
    return 0;
}