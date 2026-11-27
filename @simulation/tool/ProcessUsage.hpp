#pragma once

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/resource.h>
#endif
#if defined(__APPLE__)
#include <libproc.h>
#include <unistd.h>
#endif

#include <cstdint>

// What the process has used so far: processor seconds, and the instructions and cycles it retired
// where the system counts them (instructions on macOS, cycles on macOS and Windows; zero
// elsewhere). Unlike times, the counts hardly move with the machine's load.
struct Usage {
    double user{}, sys{}, instructions{}, cycles{};
};
inline Usage usageNow() {
    Usage usage;
#if defined(_WIN32)
    FILETIME created, exited, kernel, user;
    const auto ticks = [](FILETIME time) { return double((uint64_t(time.dwHighDateTime) << 32) | time.dwLowDateTime); };
    if (GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)) usage.user = 1e-7 * ticks(user), usage.sys = 1e-7 * ticks(kernel);
    ULONG64 cycles = 0;
    if (QueryProcessCycleTime(GetCurrentProcess(), &cycles)) usage.cycles = double(cycles);
#else
    rusage self{};
    getrusage(RUSAGE_SELF, &self);
    const auto seconds = [](const timeval& t) { return double(t.tv_sec) + 1e-6 * double(t.tv_usec); };
    usage.user = seconds(self.ru_utime), usage.sys = seconds(self.ru_stime);
#endif
#if defined(__APPLE__)
    rusage_info_v4 info{};
    if (proc_pid_rusage(getpid(), RUSAGE_INFO_V4, reinterpret_cast<rusage_info_t*>(&info)) == 0) usage.instructions = double(info.ri_instructions), usage.cycles = double(info.ri_cycles);
#endif
    return usage;
}
