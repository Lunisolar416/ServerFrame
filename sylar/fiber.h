#ifndef _SYLAR_FIBER_H_
#define _SYLAR_FIBER_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <sys/types.h>
#include <ucontext.h>

/**
 * @brief 协程类，协程的上下文切换使用ucontext实现
 * ucontext_t结构体定义了一个上下文，包含了寄存器状态、栈信息和信号掩码等内容，可以通过getcontext、setcontext、swapcontext等函数来操作上下文，实现协程的切换和调度。
 * typedef struct ucontext_t
    {
    struct ucontext_t* uc_link; // 下一个上下文
    sigset_t uc_sigmask;        // 信号掩码
    stack_t uc_stack;           // 栈信息
    mcontext_t uc_mcontext;     // CPU寄存器上下文（核心）
    } ucontext_t;
 */

namespace mysylar
{
class Fiber : public std::enable_shared_from_this<Fiber>
{
    friend class Scheduler;

  public:
    typedef std::shared_ptr<Fiber> ptr;
    enum State
    {
        // 协程状态
        INIT,  // 初始化
        HOLD,  // 暂停
        EXEC,  // 执行
        TERM,  // 结束
        READY, // 就绪
        EXCEPT // 异常
    };

  private:
    Fiber();

  public:
    Fiber(std::function<void()> cb, size_t stacksize = 0);
    ~Fiber();

    // 重置协程函数，并重置状态，只有当协程处于INIT或者TERM状态时才可以调用
    void reset(std::function<void()> cb);
    // 切换到当前协程
    void swapIn();
    // 切换到后台执行
    void swapOut();

    State getState() const
    {
        return m_state;
    }

    uint64_t getId() const
    {
        return m_id;
    }

  public:
    // 设置当前协程
    static void SetThis(Fiber* f);
    // 返回当前执行点协程
    static Fiber::ptr GetThis();
    // 协程切换到后台并设置为ready
    static void YieldToReady();
    // 协程切换到后台并设置为hold
    static void YieldToHold();

    // 总协程数
    static uint64_t TotalFibers();

    //
    static uint64_t GetFiberId();

    static void MainFunc();

  private:
    uint64_t m_id = 0;        // 协程ID
    uint32_t m_stacksize = 0; // 协程栈大小
    State m_state = INIT;     // 协程状态
    ucontext_t m_ctx;         // 协程上下文

    void* m_stack = nullptr;    // 协程栈指针
    std::function<void()> m_cb; // 协程执行函数
};
} // namespace mysylar
#endif // _SYLAR_FIBER_H_