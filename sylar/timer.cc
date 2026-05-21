#include "timer.h"
#include "util.h"

namespace mysylar
{

bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const
{
    if (!lhs && !rhs)
    {
        return false;
    }
    if (!lhs)
        return true;
    if (!rhs)
        return false;
    if (lhs->m_next < rhs->m_next)
        return true;
    if (lhs->m_next > rhs->m_next)
        return false;
    // 时间都一样的话就比地址值
    return lhs.get() < rhs.get();
}

Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager)
    : m_recurring(recurring), m_ms(ms), m_cb(cb), m_manager(manager)
{
    m_next = mysylar::GetCurrentMS() + ms;
}
Timer::Timer(uint64_t next) : m_next(next) {}

bool Timer::cancel()
{
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (m_cb)
    {
        m_cb = nullptr;
        auto it = m_manager->m_timers.find(shared_from_this());
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}
bool Timer::refresh()
{
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (!m_cb)
        return false;
    auto it = m_manager->m_timers.find(shared_from_this());
    if (it == m_manager->m_timers.end())
        return false;
    m_manager->m_timers.erase(it);
    m_next = mysylar::GetCurrentMS() + m_ms;
    m_manager->m_timers.insert(shared_from_this());
    return true;
}
bool Timer::reset(uint64_t ms, bool from_now)
{
    if (ms == m_ms && !from_now)
    {
        return true;
    }
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (!m_cb)
        return false;
    auto it = m_manager->m_timers.find(shared_from_this());
    if (it == m_manager->m_timers.end())
    {
        return false;
    }
    m_manager->m_timers.erase(it);
    uint64_t start = 0;
    if (from_now)
    {
        start = mysylar::GetCurrentMS();
    }
    else
    {
        start = m_next - m_ms;
    }
    m_ms = ms;
    m_next = start + m_ms;

    m_manager->addTimer(shared_from_this(), lock);
    return true;
}

TimerManager::TimerManager()
{
    m_previouseTime = mysylar::GetCurrentMS();
}
TimerManager::~TimerManager() {}

Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring)
{
    Timer::ptr timer(new Timer(ms, cb, recurring, this));
    RWMutexType::WriteLock lock(m_mutex);
    addTimer(timer, lock);
    return timer;
}

static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb)
{
    std::shared_ptr<void> tmp = weak_cond.lock();
    if (tmp)
    {
        cb();
    }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb,
                                           std::weak_ptr<void> weak_cond, bool recurring)
{
    return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}

uint64_t TimerManager::getNextTimer()
{
    RWMutexType::ReadLock lock(m_mutex);
    m_tickled = false;
    if (m_timers.empty())
    {
        return ~0ull;
    }

    const Timer::ptr& next = *m_timers.begin();
    uint64_t now_ms = mysylar::GetCurrentMS();
    if (now_ms >= next->m_next)
    {
        return 0;
    }
    else
    {
        return next->m_next - now_ms;
    }
}

void TimerManager::listExpiredCb(std::vector<std::function<void()>>& cbs)
{
    uint64_t now_ms = mysylar::GetCurrentMS();
    std::vector<Timer::ptr> expired;
    {
        RWMutexType::ReadLock lock(m_mutex);
        if (m_timers.empty())
        {
            return;
        }
    }
    RWMutexType::WriteLock lock(m_mutex);
    if (m_timers.empty())
        return;
    bool rollover = detectClockRollover(now_ms);
    if (!rollover && ((*m_timers.begin())->m_next > now_ms))
        return;
    Timer::ptr now_timer(new Timer(now_ms));
    // 找到第一个大于当前时间的定时器，也就是返回第一个未过期定时器的迭代器,如果服务器时间被调后了的话,就所有定时器都过期了，全部放入expired中
    auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
    // 如果是当前的时间和定时器的时间相等的话，说明这个定时器也是过期的
    while (it != m_timers.end() && (*it)->m_next == now_ms)
    {

        ++it;
    }
    // 将过期的定时器放到expired中
    expired.insert(expired.begin(), m_timers.begin(), it);
    // 将过期的定时器从定时器集合中删除
    m_timers.erase(m_timers.begin(), it);
    cbs.reserve(expired.size());
    for (auto& timer : expired)
    {
        cbs.push_back(timer->m_cb);
        if (timer->m_recurring)
        {
            // 如果是循环定时器的话，就重新设置下次执行的时间，并且放回定时器集合中
            timer->m_next = now_ms + timer->m_ms;
            m_timers.insert(timer);
        }
        else
        {
            timer->m_cb = nullptr;
        }
    }
}
void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock& lock)
{
    auto it = m_timers.insert(val).first;
    bool at_front = (it == m_timers.begin()) && !m_tickled;
    if (at_front)
    {
        m_tickled = true;
    }
    lock.unlock();

    if (at_front)
    {
        onTimerInsertedAtFront();
    }
}
bool TimerManager::detectClockRollover(uint64_t now_ms)
{
    bool rollover = false;
    // 如果当前时间小于上一次执行的时间，并且当前时间和上一次执行的时间的差值大于1小时的话，说明服务器时间被调后了
    if (now_ms < m_previouseTime && now_ms < (m_previouseTime - 60 * 60 * 1000))
    {

        rollover = true;
    }
    m_previouseTime = now_ms;
    return rollover;
}
bool TimerManager::hasTimer()
{
    RWMutexType::ReadLock lock(m_mutex);
    return !m_timers.empty();
}
} // namespace mysylar