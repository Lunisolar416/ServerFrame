#ifndef __SYLAR_SCHEDULER_H__
#define __SYLAR_SCHEDULER_H__

#include "fiber.h"
#include "mutex.h"
#include "thread.h"
#include <atomic>
#include <list>
#include <memory>
namespace mysylar
{

/**
 * @brief 协程调度器
 * @details 封装的是N-M的协程调度器
 *          内部有一个线程池,支持协程在线程池里面切换
 */

class Scheduler
{
  public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    /**
     * @brief 构造函数
     *
     * @param threads 线程池线程数
     * @param use_caller 是否把“当前线程”也当作调度线程
     * @param name 调度器名字
     */
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    virtual ~Scheduler();

    const std::string& getName() const
    {
        return m_name;
    }

    static Scheduler* GetThis();
    // 主协程
    static Fiber* GetMainFiber();

    void start();
    void stop();

    template <class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1)
    {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }
        if (need_tickle)
        {
            tickle();
        }
    }

    template <class InputIterator>
    void schedule(InputIterator begin, InputIterator end)
    {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while (begin != end)
            {
                need_tickle = scheduleNoLock(&*begin) || need_tickle;
                ++begin;
            }
        }
        if (need_tickle)
        {
            tickle();
        }
    }

  protected:
    virtual void tickle();
    virtual void run();
    virtual bool stopping();
    virtual void idle();
    void setThis();

  private:
    template <class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread = -1)
    {
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if (ft.fiber || ft.cb)
        {
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }

  private:
    struct FiberAndThread
    {
        Fiber::ptr fiber;
        std::function<void()> cb;
        int thread;

        FiberAndThread(Fiber::ptr f, int thr) : fiber(f), thread(thr) {}
        FiberAndThread(Fiber::ptr* f, int thr) : thread(thr)
        {
            fiber.swap(*f);
        }
        FiberAndThread(std::function<void()> f, int thr) : cb(f), thread(thr) {}
        FiberAndThread(std::function<void()>* f, int thr) : thread(thr)
        {
            cb.swap(*f);
        }
        FiberAndThread() : thread(-1) {}

        void reset()
        {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };

  private:
    MutexType m_mutex;
    std::vector<Thread::ptr> m_threads; // 线程池
    std::list<FiberAndThread> m_fibers; // 协程队列
    Fiber::ptr m_rootFiber;             // use_caller为true时有效, 调度协程
    std::string m_name;                 // 协程调度器名称
  protected:
    std::vector<int> m_threadIds;                  // 协程下的线程id数组
    size_t m_threadCount = 0;                      // 线程数量
    std::atomic<size_t> m_activeThreadCount = {0}; // 活跃线程数量
    std::atomic<size_t> m_idleThreadCount = {0};   // 空闲线程数量
    bool m_stopping = true;                        // 是否正在停止
    bool m_autoStop = false;                       // 是否自动停止
    int m_rootThread = 0;                          // 主线程id(use_caller)
};

} // namespace mysylar

#endif