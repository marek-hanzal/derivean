#include "core/service/Workers.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <climits>
#include <memory>
#include <stdexcept>

namespace derivean::sim {
namespace {

thread_local int claimCeiling = INT_MIN;

} // namespace

Workers::Claim::Claim(int ceiling) : previous_(claimCeiling) { claimCeiling = std::max(previous_, ceiling); }

Workers::Claim::~Claim() { claimCeiling = previous_; }

int Workers::ceiling() { return claimCeiling; }

int Workers::above(int priority, bool ordered) {
    if (priority > claimCeiling || (ordered && priority == claimCeiling)) return priority;
    throw std::logic_error("a thread waits for work that does not rank above what it holds");
}

Workers::Workers(unsigned threads) {
    if (threads == 0) threads = std::max(1u, std::thread::hardware_concurrency() - 1);
    for (unsigned index = 0; index < threads; ++index) threads_.emplace_back([this] { loop(); });
}

Workers::~Workers() { stop(); }

void Workers::stop() {
    {
        std::lock_guard lock(mutex_);
        if (stopping_) return;
        stopping_ = true;
    }
    wake_.notify_all();
    for (auto& thread : threads_) thread.join();
}

uint64_t Workers::submit(std::function<void()> task, int priority) {
    uint64_t ticket;
    {
        std::lock_guard lock(mutex_);
        ticket = submitted_++;
        Entry entry{priority, ticket, std::move(task)};
        const auto position = std::find_if(queue_.begin(), queue_.end(), [&](const Entry& other) {
            return other.priority < entry.priority;
        });
        queue_.insert(position, std::move(entry));
    }
    wake_.notify_one();
    return ticket;
}

bool Workers::cancelQueued(uint64_t ticket) {
    // Release the task's captures outside the pool lock.
    std::function<void()> canceled;
    {
        std::lock_guard lock(mutex_);
        const auto target = std::find_if(queue_.begin(), queue_.end(), [ticket](const Entry& entry) { return entry.order == ticket; });
        if (target == queue_.end()) return false;
        canceled = std::move(target->task);
        queue_.erase(target);
    }
    return true;
}

bool Workers::runOne(std::unique_lock<std::mutex>& lock) {
    if (queue_.empty()) return false;
    auto task = std::move(queue_.front().task);
    queue_.pop_front();
    lock.unlock();
    // A task has nobody to hand an exception to: one that throws ends the program, on a pool thread
    // and on a thread helping in helpUntil alike, rather than surfacing in an unrelated waiter.
    [&]() noexcept { task(); }();
    lock.lock();
    wake_.notify_all();
    return true;
}

void Workers::loop() {
    std::unique_lock lock(mutex_);
    while (true) {
        wake_.wait(lock, [this] { return stopping_ || !queue_.empty(); });
        if (stopping_) return;
        runOne(lock);
    }
}

void Workers::helpUntil(const std::function<bool()>& done) {
    std::unique_lock lock(mutex_);
    while (!done()) {
        if (!stopping_ && !queue_.empty() && queue_.front().priority > claimCeiling && runOne(lock)) continue;
        wake_.wait_for(lock, std::chrono::milliseconds(2));
    }
}

void Workers::notify() {
    // Taking the lock orders this after a waiter's check of its condition, so the wake is not lost.
    { std::lock_guard lock(mutex_); }
    wake_.notify_all();
}

void Workers::parallelFor(int count, const std::function<void(int, int)>& body, int grain, int priority) {
    if (count <= 0) return;
    const int parts = std::clamp(count / std::max(1, grain), 1, int(threads_.size()) + 1);
    if (parts == 1) {
        body(0, count);
        return;
    }
    // The caller shares the ranges with whichever pool threads are free and takes every range
    // nobody else has claimed, so it never waits for a thread to become free and never runs
    // another queued task while it waits: safe from inside a task that others depend on.
    struct Share {
        std::atomic<int> next{0}, finished{0};
        int parts{}, count{};
        const std::function<void(int, int)>* body{};   // lives as long as an unclaimed range does
        std::mutex mutex;
        std::condition_variable done;
    };
    const auto share = std::make_shared<Share>();
    share->parts = parts, share->count = count, share->body = &body;
    const auto work = [](Share& shared) {
        while (true) {
            const int part = shared.next.fetch_add(1);
            if (part >= shared.parts) return;
            (*shared.body)(int(int64_t(shared.count) * part / shared.parts), int(int64_t(shared.count) * (part + 1) / shared.parts));
            if (shared.finished.fetch_add(1) + 1 < shared.parts) continue;
            std::lock_guard lock(shared.mutex);
            shared.done.notify_all();
        }
    };
    // The caller waits for every part, and it may hold a claim: a part on another thread holds
    // its priority, so that it runs only tasks that rank above all the part may wait for.
    for (int helper = 1; helper < parts; ++helper)
        submit([share, work, priority] {
            const Claim claim(priority);
            work(*share);
        }, priority);
    work(*share);
    std::unique_lock lock(share->mutex);
    share->done.wait(lock, [&] { return share->finished.load() == share->parts; });
}

} // namespace derivean::sim
