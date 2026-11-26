#include "landscape/fn/Blur.hpp"

#include <algorithm>
#include <cstddef>

namespace derivean::sim {

void blur(std::vector<float>& field, std::vector<float>& work, int size, int radius) {
    const float scale = 1.0f / float(2 * radius + 1);
    for (int pass = 0; pass < 2; ++pass) {
        for (int z = 0; z < size; ++z) {
            float sum = 0;
            const auto at = [&](int x) { return field[size_t(z) * size_t(size) + size_t(std::clamp(x, 0, size - 1))]; };
            for (int x = -radius; x <= radius; ++x) sum += at(x);
            for (int x = 0; x < size; ++x) {
                work[size_t(z) * size_t(size) + size_t(x)] = sum * scale;
                sum += at(x + radius + 1) - at(x - radius);
            }
        }
        for (int x = 0; x < size; ++x) {
            float sum = 0;
            const auto at = [&](int z) { return work[size_t(std::clamp(z, 0, size - 1)) * size_t(size) + size_t(x)]; };
            for (int z = -radius; z <= radius; ++z) sum += at(z);
            for (int z = 0; z < size; ++z) {
                field[size_t(z) * size_t(size) + size_t(x)] = sum * scale;
                sum += at(z + radius + 1) - at(z - radius);
            }
        }
    }
}

void meanOver(std::vector<float>& value, std::vector<float>& mask, std::vector<float>& work, int size, int radius) {
    blur(value, work, size, radius);
    blur(mask, work, size, radius);
    for (size_t i = 0; i < value.size(); ++i) value[i] = mask[i] > 1e-6f ? value[i] / mask[i] : 0.0f;
}

} // namespace derivean::sim
