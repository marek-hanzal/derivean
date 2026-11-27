#include "core/service/Workers.hpp"
#include "simulation/constant/BuildPriority.hpp"

#include <atomic>
#include <climits>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <stdexcept>
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

namespace {

// A prepared build's parts may wait for any tile, so a parked ground tile leaves them alone, and
// a part's ceiling lets the waits of a prepared build through. The pool's one thread is held up;
// the caller's first range waits until the parked thread has passed the queued part by, then
// frees the pool, whose thread takes the part and with it the second range.
void preparedPart() {
    const int ground = terrainPriority(Level::Ground);
    Workers pool(1);
    std::atomic<bool> blocked{false}, release{false}, queued{false}, passed{false}, secondDone{false}, letThrough{false};
    std::atomic<int> ceilingInPart{0};
    std::atomic<std::thread::id> partOn{};
    pool.submit([&] {
        blocked = true;
        while (!release) std::this_thread::sleep_for(1ms);
    });
    while (!blocked) std::this_thread::sleep_for(1ms);
    std::thread parked([&] {
        const Workers::Claim claim(ground);
        int looks = 0;
        const auto deadline = std::chrono::steady_clock::now() + 20s;
        pool.helpUntil([&] {
            if (queued) ++looks;
            return looks >= 2 || std::chrono::steady_clock::now() > deadline;
        });
        passed = true;
    });
    const auto parkedId = parked.get_id();
    std::thread caller([&] {
        pool.parallelFor(2, [&](int begin, int) {
            if (begin == 0) {
                queued = true;
                while (!passed) std::this_thread::sleep_for(1ms);
                release = true;
                while (!secondDone) std::this_thread::sleep_for(1ms);
                return;
            }
            partOn = std::this_thread::get_id();
            ceilingInPart = Workers::ceiling();
            try {
                bool through = true;
                for (const Level level : {Level::Region, Level::Local, Level::Site, Level::Ground}) through = through && Workers::above(terrainPriority(level), true) == terrainPriority(level);
                letThrough = through && Workers::above(aquiferPriority(Level::Local)) == aquiferPriority(Level::Local) &&
                    Workers::above(aquiferPriority(Level::Region)) == aquiferPriority(Level::Region);
            } catch (const std::logic_error&) {
            }
            secondDone = true;
        }, 1, preparedPriority);
    });
    const auto callerId = caller.get_id();
    caller.join();
    parked.join();
    pool.stop();
    expect(partOn != std::thread::id{} && partOn != parkedId && partOn != callerId, "a parked ground tile took a part of a prepared build");
    expect(ceilingInPart == preparedPriority, "a part does not hold its priority");
    expect(letThrough, "a part of a prepared build refused the waits of a prepared build");
}

} // namespace

// A ground tile parked on its ground water works the queue above its level only. The pool's one
// thread is held up while the queue holds a neighbouring ground tile, queued first, and a local
// aquifer. The parked thread takes the aquifer, then looks at the queue again and leaves the
// neighbour alone, which the pool's thread runs once it is free.
void testWorkersCeiling() {
    const int ground = terrainPriority(Level::Ground), local = aquiferPriority(Level::Local);
    Workers pool(1);
    std::atomic<bool> blocked{false}, release{false}, neighbourDone{false};
    std::atomic<std::thread::id> aquiferOn{}, neighbourOn{};
    pool.submit([&] {
        blocked = true;
        while (!release) std::this_thread::sleep_for(1ms);
    });
    while (!blocked) std::this_thread::sleep_for(1ms);
    pool.submit([&] {
        neighbourOn = std::this_thread::get_id();
        neighbourDone = true;
        pool.notify();
    }, ground);
    pool.submit([&] { aquiferOn = std::this_thread::get_id(); }, local);
    std::thread parked([&] {
        const Workers::Claim claim(ground);
        expect(Workers::ceiling() == ground, "a claim does not raise the ceiling to its priority");
        // One look after the aquifer passes the neighbour by, which then heads the queue.
        int looksAfter = 0;
        const auto deadline = std::chrono::steady_clock::now() + 20s;
        pool.helpUntil([&] {
            if (aquiferOn != std::thread::id{}) ++looksAfter;
            return looksAfter >= 2 || std::chrono::steady_clock::now() > deadline;
        });
        expect(aquiferOn == std::this_thread::get_id(), "a parked ground tile did not help with a local aquifer");
        expect(neighbourOn == std::thread::id{}, "a parked ground tile took a neighbouring ground tile");
        bool refused = false;
        try {
            Workers::above(ground);
        } catch (const std::logic_error&) {
            refused = true;
        }
        expect(refused, "a wait at the thread's own level was let through");
        expect(Workers::above(ground, true) == ground && Workers::above(local) == local, "a wait above the ceiling was refused");
        {
            const Workers::Claim inner(local);
            expect(Workers::ceiling() == local, "a nested claim does not raise the ceiling");
            const Workers::Claim lower(ground);
            expect(Workers::ceiling() == local, "a lower nested claim lowered the ceiling");
        }
        expect(Workers::ceiling() == ground, "a claim does not give the ceiling back");
    });
    parked.join();
    expect(Workers::ceiling() == INT_MIN, "a thread holding nothing has a ceiling");
    release = true;
    while (!neighbourDone) std::this_thread::sleep_for(1ms);
    pool.stop();
    expect(neighbourOn != aquiferOn, "the neighbouring ground tile ran on the parked thread");
    preparedPart();
}

