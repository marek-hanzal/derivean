#include "threads/type/Threads.hpp"

#include <algorithm>
#include <thread>

#if defined(__APPLE__)
#include <pthread/qos.h>
#include <sys/sysctl.h>
#elif defined(_WIN32)
#include <windows.h>

#include <vector>
#endif

namespace derivean {

namespace {

#if defined(_WIN32)
// Physical cores of the highest efficiency class, or 0 when every core is of one class: a hybrid
// processor ranks its performance cores above its efficient ones, a uniform one ranks none
// (GetLogicalProcessorInformationEx, docs/credits/libraries.md).
unsigned performanceCores() {
    DWORD bytes = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &bytes);
    if (bytes == 0) return 0;
    std::vector<char> buffer(bytes);
    auto* first = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data());
    if (!GetLogicalProcessorInformationEx(RelationProcessorCore, first, &bytes)) return 0;
    BYTE highest = 0, lowest = 0xff;
    unsigned atHighest = 0;
    for (DWORD at = 0; at < bytes;) {
        const auto* core = reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data() + at);
        const BYTE efficiency = core->Processor.EfficiencyClass;
        if (efficiency > highest) highest = efficiency, atHighest = 0;
        if (efficiency == highest) ++atHighest;
        lowest = std::min(lowest, efficiency);
        at += core->Size;
    }
    return highest > lowest ? atHighest : 0;
}
#endif

} // namespace

unsigned backgroundThreads() {
#if defined(__APPLE__)
    int cores = 0;
    size_t size = sizeof cores;
    if (sysctlbyname("hw.perflevel0.physicalcpu", &cores, &size, nullptr, 0) == 0 && cores > 0) return unsigned(cores);
#elif defined(_WIN32)
    if (const unsigned cores = performanceCores()) return cores;
#endif
    return std::max(2u, std::thread::hardware_concurrency() - std::min(2u, std::thread::hardware_concurrency()));
}

void favourCallingThread() {
#if defined(__APPLE__)
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#elif defined(_WIN32)
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#endif
}

} // namespace derivean
