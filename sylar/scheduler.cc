#include "scheduler.h"
#include "fiber.h"
#include "log.h"
#include "macro.h"
#include "thread.h"
#include "util.h"
namespace mysylar
{
static mysylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static thread_local Scheduler* t_scheduler = nullptr;   // 当前线程的调度器
static thread_local Fiber* t_scheduler_fiber = nullptr; // 当前线程的协程

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
{
    SYLAR_ASSERT(threads > 0);

    if (use_caller)
    {
        // 如果是要将当前线程也当作调度线程
        mysylar::Fiber::GetThis(); // 初始化当前线程的 主协程（main fiber）
        --threads; // 由于当前线程已经作为了调度线程，那么整体的线程数量就要减1
        SYLAR_ASSERT(GetThis() == nullptr); // 防止这个线程里面已经存在调度器
        t_scheduler = this;                 // 设置当前线程的调度器为 this
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this),
                                    0));  // 创建调度协程，协程函数为 Scheduler::run
        mysylar::Thread::setName(m_name); // 设置当前调度器线程的名字
        t_scheduler_fiber = m_rootFiber.get(); // 设置线程的协程为调度协程
        m_threadIds.push_back(m_rootThread);
    }
    else
    {
        m_rootThread = -1;
    }
    m_threadCount = threads;
}
Scheduler::~Scheduler()
{
    SYLAR_ASSERT(m_stopping);
    if (GetThis() == this)
    {
        t_scheduler = nullptr;
    }
}
Scheduler* Scheduler::GetThis()
{
    return t_scheduler;
}
// 主协程
Fiber* Scheduler::GetMainFiber()
{
    return t_scheduler_fiber;
}
// 启动协程调度器
void Scheduler::start()
{
    //
    MutexType::Lock lock(m_mutex);
    if (!m_stopping)
    {
        return;
    }
    m_stopping = false;
    SYLAR_ASSERT(m_threads.empty());
    m_threads.resize(m_threadCount);
    // 这里就是一个线程池，每个线程中都有一个协程调度函数，是
    // Scheduler::run，这个函数就是调度器的核心函数，负责调度协程的执行
    for (size_t i = 0; i < m_threadCount; ++i)
    {
        m_threads[i].reset(
            new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
}
// 停止协程调度器
void Scheduler::stop()
{
    m_autoStop = true;
    if (m_rootFiber && m_threadCount == 0 &&
        (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT))
    {
        SYLAR_LOG_INFO(g_logger) << "stopping root fiber";
        m_stopping = true;
        if (stopping())
        {
            return;
        }
    }

    m_stopping = true;
    for (size_t i = 0; i < m_threadCount; ++i)
    {
        tickle();
    }
    if (m_rootFiber)
    {
        tickle();
    }

    // 将所有线程结束
    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }
    for (auto& i : thrs)
    {
        i->join();
    }
}
// 通知协程调度器有任务了
void Scheduler::tickle() {}
// 将
void Scheduler::setThis()
{
    t_scheduler = this;
}
// 协程无任务时执行idle协程
void Scheduler::idle() {}
// 协程调度函数
void Scheduler::run()
{
    SYLAR_LOG_DEBUG(g_logger) << m_name << " run";
    setThis();
    // 当前线程的id不是主线程id
    if (mysylar::GetThreadId() != m_rootThread)
    {
        t_scheduler_fiber = Fiber::GetThis().get(); // 在当前线程创建了一个主协程
    }

    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this))); // 创建一个idle协程
    Fiber::ptr cb_fiber; // 用于执行回调的协程

    FiberAndThread ft;
    while (true)
    {
        ft.reset();
        bool tickle_me = false; //
        bool is_active = false; // 是否活跃
        {
            // 在协程的消息队列中取出任务
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while (it != m_fibers.end())
            {
                // 该线程处理不了这个任务，跳过
                if (it->thread != -1 && it->thread != mysylar::GetThreadId())
                {
                    ++it;
                    tickle_me = true; // 需要通知其他线程有任务处理
                    continue;
                }

                SYLAR_ASSERT(it->fiber || it->cb);
                // 当前任务正在执行，跳过
                if (it->fiber && it->fiber->getState() == Fiber::EXEC)
                {
                    ++it;
                    continue;
                }

                // 匹配到所需任务
                ft = *it;
                m_fibers.erase(it++);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
        }
        if (tickle_me)
        {
            tickle();
        }
        if (ft.fiber &&
            (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT))
        {
            ft.fiber->swapIn();    // 开始执行任务
            --m_activeThreadCount; // 这时候任务完成，活跃线程-1
            if (ft.fiber->getState() == Fiber::READY)
            {
                // 重新入队,再次执行
                schedule(ft.fiber);
            }
            else if (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)
            {
                ft.fiber->m_state = Fiber::HOLD;
            }
            ft.reset();
        }
        else if (ft.cb)
        {
            if (cb_fiber)
            {
                cb_fiber->reset(ft.cb);
            }
            else
            {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            cb_fiber->swapIn();
            --m_activeThreadCount; // 这时候任务完成，活跃线程-1
            if (cb_fiber->getState() == Fiber::READY)
            {
                schedule(cb_fiber);
                cb_fiber.reset();
            }
            else if (cb_fiber->getState() == Fiber::EXCEPT || cb_fiber->getState() == Fiber::TERM)
            {
                // 如果这时候任务已经结束或异常，则抛弃
                cb_fiber->reset(nullptr);
            }
            else
            {
                // if(cb_fiber->getState() != Fiber::TERM)
                //
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        }
        else
        {
            // 这时候就已经没任务了，idle协程
            if (is_active)
            {
                --m_activeThreadCount;
                continue;
            }
            if (idle_fiber->getState() == Fiber::TERM)
            {
                SYLAR_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }

            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if (idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::EXCEPT)
            {
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }
}
// 返回是否可以停止
bool Scheduler::stopping()
{
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
}

} // namespace mysylar