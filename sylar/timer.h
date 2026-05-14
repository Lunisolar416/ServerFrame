#ifndef __SYLAR_TIMER_H__
#define __SYLAR_TIMER_H__

#include "mutex.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <set>
#include <sys/types.h>
namespace mysylar
{
class TimerManager;

class Timer : public std::enable_shared_from_this<Timer>
{
    friend class TimerManager;

  public:
    typedef std::shared_ptr<Timer> ptr;

    /**
     * @brief 取消定时器
     *
     * @return true
     * @return false
     */
    bool cancel();
    /**
     * @brief 刷新设置定时器的执行时间
     *
     * @return true
     * @return false
     */
    bool refresh();

    /**
     * @brief 重置定时器时间
     *
     * @param ms
     * @param from_now
     * @return true
     * @return false
     */
    bool reset(uint64_t ms, bool from_now);

  private:
    Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager);
    Timer(uint64_t next);

  private:
    /// 是否循环定时器
    bool m_recurring = false;
    /// 执行周期
    uint64_t m_ms = 0;
    /// 精确的执行时间
    uint64_t m_next = 0;
    /// 回调函数
    std::function<void()> m_cb;
    // 定时器管理器
    TimerManager* m_manager = nullptr;

  private:
    struct Comparator
    {
        bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
    };
};

class TimerManager
{
    friend class Timer;

  public:
    typedef mysylar::RWMutex RWMutexType;
    TimerManager();
    virtual ~TimerManager();

    /**
     * @brief 添加定时器
     *
     * @param ms 定时器执行间隔时间
     * @param cb 定时器回调函数
     * @param recurring 是否循环定时器
     * @return Timer::ptr
     */
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);

    /**
     * @brief 添加条件定时器
     *
     * @param ms 定时器执行间隔时间
     * @param cb 定时器回调函数
     * @param weak_cond 条件，需要满足的条件
     * @param recurring 是否循环定时器
     * @return Timer::ptr
     */
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb,
                                 std::weak_ptr<void> weak_cond, bool recurring = false);

    /**
     * @brief 获取下一个定时器的时间
     *
     * @return uint64_t
     */
    uint64_t getNextTimer();

    /**
     * @brief 获取需要执行的定时器的回调函数列表
     *
     * @param cbs
     */
    void listExpiredCb(std::vector<std::function<void()>>& cbs);

  protected:
    virtual void onTimerInsertedAtFront() = 0;
    void addTimer(Timer::ptr val, RWMutexType::WriteLock& lock);
    bool hasTimer();

  private:
    /**
     * @brief 检测服务器时间是否被调后了
     */
    bool detectClockRollover(uint64_t now_ms);

  private:
    RWMutexType m_mutex;
    /// 定时器集合
    std::set<Timer::ptr, Timer::Comparator> m_timers;
    /// 是否触发onTimerInsertedAtFront
    bool m_tickled = false;
    // 上一次执行的时间
    uint64_t m_previouseTime = 0;
};

}; // namespace mysylar

#endif //__SYLAR_TIMER_H__