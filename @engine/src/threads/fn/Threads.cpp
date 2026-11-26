#include "threads/type/Threads.hpp"

#include <algorithm>
#include <thread>

#if defined(__APPLE__)
#include <pthread/qos.h>
#include <sys/sysctl.h>
#endif

namespace derivean {

unsigned backgroundThreads() {
#if defined(__APPLE__)
    int cores = 0;
    size_t size = sizeof cores;
    if (sysctlbyname("hw.perflevel0.physicalcpu", &cores, &size, nullptr, 0) == 0 && cores > 0) return unsigned(cores);
#endif
    return std::max(2u, std::thread::hardware_concurrency() - std::min(2u, std::thread::hardware_concurrency()));
}

void favourCallingThread() {
#if defined(__APPLE__)
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#endif
}

} // namespace derivean
