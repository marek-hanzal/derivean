#include "core/service/Workers.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <thread>

namespace {

using namespace derivean::sim;
using namespace std::chrono_literals;

// The circle Workers::Claim keeps shut. Thread B has claimed local ground water X and waits for
// regional water R, which thread C is building. The pool's one thread is busy, and the queue holds
// a task that needs X. If B works the queue while it waits, that task runs on B's stack and waits
// for X, which only B can finish, once the task has returned: B hangs, though R comes in. True
// when B came through within `patience`; a hang is let go by `abort` so the test can end.
bool comesThrough(bool guarded, std::chrono::milliseconds patience) {
    Workers pool(1);
    std::atomic<bool> xDone{false}, rDone{false}, abort{false}, blocked{false}, finished{false}, takenByB{false};
    std::atomic<int> checks{0};   // how often B looked whether R is in
    std::atomic<std::thread::id> bId{};
    // The pool thread is held up until R is in, so the task for X stays queued.
    pool.submit([&] {
        blocked = true;
        while (!rDone && !abort) std::this_thread::sleep_for(1ms);
    });
    while (!blocked) std::this_thread::sleep_for(1ms);
    pool.submit([&] {
        if (std::this_thread::get_id() == bId) takenByB = true;
        pool.helpUntil([&] { return xDone || abort; });
    });
    // C finishes R once B has shown which way it waits: it took the task, or it looked twice
    // and so passed the queued task by once.
    std::thread c([&] {
        while (!takenByB && checks < 2 && !abort) std::this_thread::sleep_for(1ms);
        rDone = true;
        pool.notify();
    });
    std::thread b([&] {
        bId = std::this_thread::get_id();
        {
            // B builds X: it waits for R, then X is done.
            std::optional<Workers::Claim> claim;
            if (guarded) claim.emplace();
            pool.helpUntil([&] {
                ++checks;
                return rDone || abort;
            });
            xDone = true;
        }
        pool.notify();
        finished = true;
    });
    const auto start = std::chrono::steady_clock::now();
    while (!finished && std::chrono::steady_clock::now() - start < patience) std::this_thread::sleep_for(1ms);
    const bool through = finished;
    abort = true;
    pool.notify();
    b.join();
    c.join();
    pool.stop();
    return through;
}

void expect(bool accepted, const char* message) {
    if (accepted) return;
    std::fprintf(stderr, "Workers: %s\n", message);
    std::exit(1);
}

} // namespace

void testWorkersClaim() {
    expect(!comesThrough(false, 2000ms), "a waiter holding a claim it does not declare came through; the test no longer builds the circle");
    expect(comesThrough(true, 20000ms), "a waiter holding a claim ran a queued task that waits for that claim");
}
