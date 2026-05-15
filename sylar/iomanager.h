#ifndef __SYLAR__IOMANAGER_H__
#define __SYLAR__IOMANAGER_H__
#include "mutex.h"
#include "scheduler.h"
#include "timer.h"
namespace mysylar
{

class IOManager : public Scheduler, public TimerManager
{
  public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    /**
     * @brief IO事件
     */
    enum Event
    {
        /// 无事件
        NONE = 0x0,
        /// 读事件(EPOLLIN)
        READ = 0x1,
        /// 写事件(EPOLLOUT)
        WRITE = 0x4,
    };

  private:
    struct FdContext
    {
        typedef Mutex MutexType;
        struct EventContext
        {
            /// 事件执行的调度器
            Scheduler* scheduler;
            /// 事件协程
            Fiber::ptr fiber;
            std::function<void()> cb;
        };

        EventContext& getContext(Event event);
        void resetContext(EventContext& ctx);
        void triggerEvent(Event event);
        // 事件关联的句柄
        int fd = 0;
        // 读事件
        EventContext read;
        // 写事件
        EventContext write;
        // 已经注册的事件
        Event events = NONE;

        MutexType mutex;
    };

  public:
    IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    ~IOManager();

    /**
     * @brief 添加事件
     *
     * @param fd socket句柄
     * @param event
     * @param cb
     * @return int
     */
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    /**
     * @brief 删除事件
     *
     * @return true
     * @return false
     */
    bool delEvent(int fd, Event event);
    /**
     * @brief
     *
     */
    bool cancelEvent(int fd, Event event);

    /**
     * @brief
     *
     * @param fd
     * @return true
     * @return false
     */
    bool cancelAll(int fd);

    static IOManager* GetThis();

  protected:
    void tickle() override;
    bool stopping(uint64_t& timeout);
    bool stopping() override;
    void idle() override;

    void contexResize(size_t size);

    // 继承TimerManager的接口
    void onTimerInsertedAtFront() override;

  private:
    /// epoll 文件句柄
    int m_epfd;
    ///
    int m_tickleFds[2];
    /// 当前等待执行的事件数量
    std::atomic<size_t> m_pendingEventCount;
    /// IOManager的Mutex
    RWMutexType m_mutex;
    /// socket事件上下文的容器
    std::vector<FdContext*> m_fdContexts;
};

} // namespace mysylar

#endif