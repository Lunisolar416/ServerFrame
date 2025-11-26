#include "util.h"
#include <sys/syscall.h>
#include <unistd.h>
namespace mysylar
{
pid_t getThreadId()
{
    return syscall(SYS_gettid);
}
uint32_t getFiberId()
{
    return 0;
}
} // namespace mysylar
