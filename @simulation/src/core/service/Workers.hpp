#pragma once

#include <condition_variable>
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
    void submit(std::function<void()> task, int priority = 0);

    // Runs queued tasks on the calling thread until `done` holds. Lets a task wait for the tasks
    // it depends on without parking a pool thread.
    void helpUntil(const std::function<bool()>& done);

    // Splits [0, count) into contiguous ranges; returns after every range has finished. The caller
    // works through the ranges itself and free pool threads join in; it runs no other queued task
    // meanwhile, so it may be called from inside a task. `body` must give the same result however
    // the ranges are cut.
    void parallelFor(int count, const std::function<void(int begin, int end)>& body, int grain = 1);

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
