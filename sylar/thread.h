#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__
#include "mutex.h"
#include <functional>
#include <memory>
#include <pthread.h>
#include <string>
#include <sys/types.h>
#include <thread>

namespace mysylar
{
class Thread
{
  public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb, const std::string& name);
    ~Thread();

    pid_t getId() const
    {
        return m_id;
    }
    const std::string& getName() const
    {
        return m_name;
    }

    void join();
    static Thread* GetThis();
    static const std::string& GetName();
    static void setName(const std::string& name);

    static void* run(void* args);

  private:
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
    Thread(const Thread&&) = delete;

  private:
    pid_t m_id; // 线程id
    pthread_t m_thread;
    std::function<void()> m_cb;
    std::string m_name; // 线程名称

    Semaphore m_semaphore; // 信号量
};
} // namespace mysylar

#endif //__SYLAR_THREAD_H__