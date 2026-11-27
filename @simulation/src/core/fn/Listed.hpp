#pragma once

#include <vector>

namespace derivean::sim {

// Whether a short list (a dozen or two entries) holds a value: a plain loop, which the compiler keeps
// inline, where std::find on int calls a library search (wmemchr) that costs more than it finds.
template <class T>
bool listed(const std::vector<T>& list, T value) {
    for (const T& entry : list)
        if (entry == value) return true;
    return false;
}

} // namespace derivean::sim
