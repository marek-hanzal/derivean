#pragma once

#include <mutex>
#include <utility>
#include <vector>

namespace derivean {

// Results of background tasks on their way to the game thread. A task owns a share of the mailbox,
// so posting stays valid whatever happens to the state that asked for the work.
template<typename T>
class Mailbox {
public:
    void post(T item) {
        std::lock_guard lock(mutex_);
        items_.push_back(std::move(item));
    }

    std::vector<T> collect() {
        std::lock_guard lock(mutex_);
        return std::exchange(items_, {});
    }

private:
    std::mutex mutex_;
    std::vector<T> items_;
};

} // namespace derivean
