#pragma once

#include <condition_variable>
#include <climits>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace derivean::sim {

// Native CPU pool for the expensive, pure parts of the simulation. Tasks never decide results by
// arrival order: every task writes only its own output, so the thread count cannot change a world.
class Workers {
public:
    explicit Workers(unsigned threads = 0);
    ~Workers();
    Workers(const Workers&) = delete;
    Workers& operator=(const Workers&) = delete;

    // Join active workers before their owners go away. No external caller can still submit work.
    void stop();

    unsigned threads() const { return unsigned(threads_.size()); }

    // Higher priority runs first; equal priorities run in submission order.
    //
    // Priority also orders what waits for what. A task, and a thread that holds a Claim, waits only
    // for work of a higher priority, or for work of the same priority that an order of the
    // owner's own builds earlier (the owner states that order). A thread whose ceiling is C may
    // therefore run any queued task ranked above C while it waits. Whatever that task waits for
    // ranks above C as well, so the wait never closes through this thread's stack, where
    // everything it holds ranks at C or below.
    uint64_t submit(std::function<void()> task, int priority = 0);

    // Removes a queued task by its submission ticket. A claimed task keeps running.
    bool cancelQueued(uint64_t ticket);

    // Held by a thread while it builds something others may wait for, at that build's priority.
    // It raises the thread's ceiling to the highest priority it holds and puts the previous
    // ceiling back when it ends. A thread holding nothing has no ceiling (INT_MIN).
    class Claim {
    public:
        explicit Claim(int ceiling = INT_MAX);
        ~Claim();
        Claim(const Claim&) = delete;
        Claim& operator=(const Claim&) = delete;

    private:
        int previous_;
    };

    // The calling thread's ceiling.
    static int ceiling();

    // Returns `priority` if work of that priority ranks above the calling thread's ceiling, or,
    // with `ordered` set, for work whose waits within one priority follow an order of its own, at
    // least matches it. Otherwise it throws std::logic_error, in every build: the wait would break
    // the order above. That order within a priority is the caller's to keep; this does not see it.
    static int above(int priority, bool ordered = false);

    // Runs queued tasks on the calling thread until `done` holds. Only a task ranked above the
    // thread's ceiling is run; a thread whose pool is stopping only waits. Lets a waiter work for
    // the tasks it waits on without parking a pool thread. `done` is called with the pool's lock
    // held, so it may take locks of its own that are never held while calling into the pool;
    // whoever makes it true calls notify().
    void helpUntil(const std::function<bool()>& done);

    // Splits [0, count) into contiguous ranges; returns after every range has finished. The caller
    // works through the ranges itself and free pool threads join in; it runs no other queued task
    // meanwhile, so it may be called from inside a task. `body` must give the same result however
    // the ranges are cut. A part on another thread runs at `priority` and holds it as a Claim.
    // A body that waits for other work passes a priority below everything it waits for; at Leaf
    // a body must not wait at all. The caller's own wait for the parts breaks no order: it takes
    // every range nobody else has, so it only waits for ranges already running.
    static constexpr int Leaf = 1 << 20;   // a body that waits for nothing
    void parallelFor(int count, const std::function<void(int begin, int end)>& body, int grain = 1, int priority = Leaf);

    // Wakes helpers that poll a completion flag owned by the caller.
    void notify();

private:
    struct Entry {
        int priority;
        uint64_t order;
        std::function<void()> task;
    };

    bool runOne(std::unique_lock<std::mutex>& lock);
    void loop();

    std::vector<std::thread> threads_;
    std::deque<Entry> queue_;
    std::mutex mutex_;
    std::condition_variable wake_;
    uint64_t submitted_{};
    bool stopping_{};
};

} // namespace derivean::sim
