#include "fiber.h"
#include "config.h"
#include "log.h"
#include "macro.h"
#include <atomic>
#include <cstdint>
#include <exception>
#include <functional>
#include <ucontext.h>
namespace mysylar
{
static Logger::ptr g_logger = SYLAR_LOG_NAME("system");
static std::atomic<uint64_t> s_fiber_id{0};    // 协程ID
static std::atomic<uint64_t> s_fiber_count{0}; // 协程总数

static thread_local Fiber* t_fiber = nullptr;           // 当前协程
static thread_local Fiber::ptr t_threadFiber = nullptr; // 主协程

static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
    Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

class MallocStackAllocator
{
  public:
    static void* Alloc(size_t size)
    {
        return malloc(size);
    }
    static void Dealloc(void* vp, size_t size)
    {
        return free(vp);
    }
};

using StackAllocator = MallocStackAllocator;

Fiber::Fiber()
{
    m_state = EXEC;
    SetThis(this);
    if (getcontext(&m_ctx))
    {
        SYLAR_ASSERT2(false, "getcontext");
    }
    ++s_fiber_count;
}
Fiber::Fiber(std::function<void()> cb, size_t stacksize) : m_id(++s_fiber_id), m_cb(cb)
{
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();
    m_stack = StackAllocator::Alloc(m_stacksize);
    if (getcontext(&m_ctx))
    {
        SYLAR_ASSERT2(false, "getcontext");
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_size = m_stacksize;
    m_ctx.uc_stack.ss_sp = m_stack;
    // makecontext函数作用就是修改 context，使其从某个函数开始执行，创建协程入口函数
    // SYLAR_LOG_INFO(g_logger) << "begin makecontext, fiber id=" << getId();
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
}
Fiber::~Fiber()
{
    --s_fiber_count;
    if (m_stack)
    {
        SYLAR_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
        // 协程结束后，释放栈内存
        StackAllocator::Dealloc(m_stack, m_stacksize);
    }
    else
    {
        SYLAR_ASSERT(!m_cb);
        SYLAR_ASSERT(m_state == EXEC);
        Fiber* cur = t_fiber;
        if (cur == this)
        {
            SetThis(nullptr);
        }
    }
}

// 重置协程函数，并重置状态，只有当协程处于INIT或者TERM状态时才可以调用
void Fiber::reset(std::function<void()> cb)
{
    SYLAR_ASSERT(m_stack);
    SYLAR_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
    m_cb = cb;
    if (getcontext(&m_ctx))
    {
        SYLAR_ASSERT2(false, "getcontext");
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_size = m_stacksize;
    m_ctx.uc_stack.ss_sp = m_stack;
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT;
}
// 切换到当前协程
void Fiber::swapIn()
{
    if (!t_threadFiber)
    {
        Fiber::GetThis();
        SYLAR_ASSERT(t_threadFiber);
    }
    SetThis(this);
    SYLAR_ASSERT(m_state != EXEC);
    m_state = EXEC;
    // SYLAR_LOG_INFO(g_logger) << "get in swapIn, fiber id=" << getId();
    if (swapcontext(&t_threadFiber->m_ctx, &m_ctx))
    {
        SYLAR_LOG_ERROR(g_logger) << "swapcontext error";
        SYLAR_ASSERT2(false, "swapcontext");
    }
}
// 切换到后台执行
void Fiber::swapOut()
{
    if (!t_threadFiber)
    {
        Fiber::GetThis();
        SYLAR_ASSERT(t_threadFiber);
    }
    SetThis(t_threadFiber.get());
    // 这时候协程切换到后台了，状态设置为 HOLD，等到下次被调度的时候才会切换到 READY 状态
    m_state = TERM;
    if (swapcontext(&m_ctx, t_threadFiber ? &t_threadFiber->m_ctx : nullptr))
    {
        SYLAR_ASSERT2(false, "swapcontext");
    }
}
void Fiber::SetThis(Fiber* f)
{
    t_fiber = f;
}
// 返回当前执行点协程
Fiber::ptr Fiber::GetThis()
{
    if (t_fiber)
    {
        return t_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(new Fiber);
    SYLAR_LOG_INFO(g_logger) << "main fiber create";
    SYLAR_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}
// 协程切换到后台并设置为ready
void Fiber::YieldToReady()
{
    Fiber::ptr cur = GetThis();

    cur->m_state = READY;
    cur->swapOut();
}
// 协程切换到后台并设置为hold
void Fiber::YieldToHold()
{
    Fiber::ptr cur = GetThis();

    cur->m_state = HOLD;
    cur->swapOut();
}

// 总协程数
uint64_t Fiber::TotalFibers()
{
    return s_fiber_count;
}

uint64_t Fiber::GetFiberId()
{
    if (t_fiber)
    {
        return t_fiber->getId();
    }
    return 0;
}

void Fiber::MainFunc()
{
    Fiber::ptr cur = GetThis();
    // SYLAR_LOG_INFO(g_logger) << "MainFunc in fiber, fiber id=" << cur->getId();
    SYLAR_ASSERT(cur);
    try
    {
        cur->m_cb();
        cur->m_cb = nullptr; // 协程执行完毕，释放回调函数
        cur->m_state = TERM;
    }
    catch (std::exception& ex)
    {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what();
    }
    catch (...)
    {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: unknown";
    }
    // 协程执行完毕，切换到后台
    // 应该准备接着执行后台程序
    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->swapOut(); // 返回到主协程继续执行，主协程会根据状态决定是否销毁这个协程对象
}
} // namespace mysylar