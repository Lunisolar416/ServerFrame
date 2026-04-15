#ifndef __SYLAR_MUTEX_H__
#define __SYLAR_MUTEX_H__

#include "noncopyable.h"
#include <atomic>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
/**
 * @brief 信号量类
 *
 */
namespace mysylar
{
class Semaphore
{
  public:
    /**
     * @brief 构造函数
     * @param[in] count 信号量值的大小
     */
    Semaphore(uint32_t count = 0);
    /**
     * @brief 析构函数
     */
    ~Semaphore();
    /**
     * @brief 获取信号量
     */
    void wait();
    /**
     * @brief 释放信号量
     */
    void notify();

  private:
    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;
    Semaphore(Semaphore&&) = delete;

  private:
    sem_t m_semaphore;
};
/**
 * @brief 局部锁的模板实现
 */
template <class T>
struct ScopedLockImpl
{
  public:
    ScopedLockImpl(T& mutex) : m_mutex(mutex)
    {
        m_mutex.lock();
        m_locked = true;
    }
    ~ScopedLockImpl()
    {
        unlock();
    }
    void lock()
    {
        if (!m_locked)
        {
            m_mutex.lock();
            m_locked = true;
        }
    }
    void unlock()
    {
        if (m_locked)
        {
            m_mutex.unlock();
            m_locked = false;
        }
    }

  private:
    T& m_mutex;
    bool m_locked;
};

/**
 * @brief 局部读锁模板实现
 *
 */
template <class T>
struct ReadScopedLockImpl
{
  public:
    ReadScopedLockImpl(T& mutex) : m_mutex(mutex)
    {
        m_mutex.rdlock();
        m_locked = true;
    }
    ~ReadScopedLockImpl()
    {
        unlock();
    }
    void lock()
    {
        if (!m_locked)
        {
            m_mutex.rdlock();
            m_mutex.m_locked = true;
        }
    }
    void unlock()
    {
        if (m_locked)
        {
            m_mutex.unlock();
            m_locked = false;
        }
    }

  private:
    T& m_mutex;
    bool m_locked;
};

/**
 * @brief 局部写锁模板实现
 *
 */
template <class T>
struct WriteScopedLockImpl
{
  public:
    WriteScopedLockImpl(T& mutex) : m_mutex(mutex)
    {
        m_mutex.wrlock();
        m_locked = true;
    }
    ~WriteScopedLockImpl()
    {
        unlock();
    }
    void lock()
    {
        if (!m_locked)
        {
            m_mutex.wrlock();
            m_locked = true;
        }
    }
    void unlock()
    {
        if (m_locked)
        {
            m_mutex.unlock();
            m_locked = false;
        }
    }

  private:
    T& m_mutex;
    bool m_locked;
};
/**
 * @brief 互斥锁
 *
 */
class Mutex : Noncopyable
{
  public:
    typedef ScopedLockImpl<Mutex> Lock;
    Mutex()
    {
        pthread_mutex_init(&m_mutex, nullptr);
    }
    ~Mutex()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    void lock()
    {
        pthread_mutex_lock(&m_mutex);
    }
    void unlock()
    {
        pthread_mutex_unlock(&m_mutex);
    }

  private:
    pthread_mutex_t m_mutex;
};
/**
 * @brief 读写锁
 *
 */
class RWMutex
{
  public:
    /// 局部读锁
    typedef ReadScopedLockImpl<RWMutex> ReadLock;

    /// 局部写锁
    typedef WriteScopedLockImpl<RWMutex> WriteLock;
    RWMutex()
    {
        pthread_rwlock_init(&m_lock, nullptr);
    }
    ~RWMutex()
    {
        pthread_rwlock_destroy(&m_lock);
    }
    void rdlock()
    {
        pthread_rwlock_rdlock(&m_lock);
    }
    void wrlock()
    {
        pthread_rwlock_wrlock(&m_lock);
    }
    void unlock()
    {
        pthread_rwlock_unlock(&m_lock);
    }

  private:
    pthread_rwlock_t m_lock;
};
/**
 * @brief 空锁(用于调试)
 *
 */
class NullMutex : Noncopyable
{
  public:
    /// 局部锁
    typedef ScopedLockImpl<NullMutex> Lock;

    /**
     * @brief 构造函数
     */
    NullMutex() {}

    /**
     * @brief 析构函数
     */
    ~NullMutex() {}

    /**
     * @brief 加锁
     */
    void lock() {}

    /**
     * @brief 解锁
     */
    void unlock() {}
};
/**
 * @brief 空读写锁(用于调试)
 */
class NullRWMutex : Noncopyable
{
  public:
    /// 局部读锁
    typedef ReadScopedLockImpl<NullMutex> ReadLock;
    /// 局部写锁
    typedef WriteScopedLockImpl<NullMutex> WriteLock;

    /**
     * @brief 构造函数
     */
    NullRWMutex() {}
    /**
     * @brief 析构函数
     */
    ~NullRWMutex() {}

    /**
     * @brief 上读锁
     */
    void rdlock() {}

    /**
     * @brief 上写锁
     */
    void wrlock() {}
    /**
     * @brief 解锁
     */
    void unlock() {}
};
/**
 * @brief 自旋锁
 */
class Spinlock : Noncopyable
{
  public:
    /// 局部锁
    typedef ScopedLockImpl<Spinlock> Lock;

    Spinlock()
    {
        pthread_spin_init(&m_mutex, 0);
    }
    ~Spinlock()
    {
        pthread_spin_destroy(&m_mutex);
    }

    void lock()
    {
        pthread_spin_lock(&m_mutex);
    }
    void unlock()
    {
        pthread_spin_unlock(&m_mutex);
    }

  private:
    /// 自旋锁
    pthread_spinlock_t m_mutex;
};
/**
 * @brief 原子锁
 */
class CASLock : Noncopyable
{
  public:
    /// 局部锁
    typedef ScopedLockImpl<CASLock> Lock;

    /**
     * @brief 构造函数
     */
    CASLock()
    {
        m_mutex.clear();
    }

    /**
     * @brief 析构函数
     */
    ~CASLock() {}

    /**
     * @brief 上锁
     */
    void lock()
    {
        while (std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire))
            ;
    }

    /**
     * @brief 解锁
     */
    void unlock()
    {
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }

  private:
    /// 原子状态
    volatile std::atomic_flag m_mutex;
};
} // namespace mysylar
#endif // __SYLAR_MUTEX_H__