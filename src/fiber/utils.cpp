#include "include/utils.hpp"

namespace monsoon {
    pid_t GetThreadId() { return syscall(SYS_gettid); }
    // Todo
    u_int32_t GetFiberId() { return 0; }
}