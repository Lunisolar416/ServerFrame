#ifndef __SYLAR_FD_MANAGER_H__
#define __SYLAR_FD_MANAGER_H__

#include "singleton.h"
#include "thread.h"
#include <memory>
#include <vector>
namespace mysylar
{
class FdCtx : public std::enable_shared_from_this<FdCtx>
{
  public:
    typedef std::shared_ptr<FdCtx> ptr;
    FdCtx(int fd);
    ~FdCtx();

    bool isInit() const
    {
        return m_isInit;
    }
    bool isSocket() const
    {
        return m_isSocket;
    }
    bool isClose() const
    {
        return m_isClosed;
    }

    void setUserNonblock(bool v)
    {
        m_userNonblock = v;
    }
    bool getUserNonblock() const
    {
        return m_userNonblock;
    }

    void setSysNonblock(bool v)
    {
        m_sysNonblock = v;
    }
    bool getSysNonblock() const
    {
        return m_sysNonblock;
    }

    void setTimeout(int type, uint64_t v);
    uint64_t getTimeout(int type);

  private:
    bool init();

  private:
    bool m_isInit;
    bool m_isSocket;
    bool m_sysNonblock;
    bool m_userNonblock;
    bool m_isClosed;
    int m_fd;
    uint64_t m_recvTimeout;
    uint64_t m_sendTimeout;
};

class FdManager
{
  public:
    typedef RWMutex RwMutexType;
    FdManager();

    /**
     * @brief 获取/创建文件句柄类FdCtx
     *
     * @param fd
     * @param auto_create
     * @return FdCtx::ptr
     */
    FdCtx::ptr get(int fd, bool auto_create = false);

    void del(int fd);

  private:
    // 读写锁
    RwMutexType m_mutex;
    // 文件句柄合集
    std::vector<FdCtx::ptr> m_datas;
};

typedef Singleton<FdManager> FdMgr;

} // namespace mysylar

#endif // __SYLAR_FD_MANAGER_H__