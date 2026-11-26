#include "core/service/Workers.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>

namespace derivean::sim {

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

void Workers::submit(std::function<void()> task, int priority) {
    {
        std::lock_guard lock(mutex_);
        Entry entry{priority, submitted_++, std::move(task)};
        const auto position = std::find_if(queue_.begin(), queue_.end(), [&](const Entry& other) {
            return other.priority < entry.priority;
        });
        queue_.insert(position, std::move(entry));
    }
    wake_.notify_one();
}

bool Workers::runOne(std::unique_lock<std::mutex>& lock) {
    if (queue_.empty()) return false;
    auto task = std::move(queue_.front().task);
    queue_.pop_front();
    lock.unlock();
    task();
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
        if (runOne(lock)) continue;
        wake_.wait_for(lock, std::chrono::milliseconds(2));
    }
}

void Workers::notify() { wake_.notify_all(); }

void Workers::parallelFor(int count, const std::function<void(int, int)>& body, int grain) {
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
    for (int helper = 1; helper < parts; ++helper) submit([share, work] { work(*share); }, 1 << 20);
    work(*share);
    std::unique_lock lock(share->mutex);
    share->done.wait(lock, [&] { return share->finished.load() == share->parts; });
}

} // namespace derivean::sim
