#include "diagnostics/fn/Png.hpp"

#include <array>
#include <cstdio>

namespace derivean::sim {
namespace {

uint32_t crc(const uint8_t* data, size_t size, uint32_t value = 0xffffffffu) {
    static const auto table = [] {
        std::array<uint32_t, 256> entries{};
        for (uint32_t n = 0; n < 256; ++n) {
            uint32_t c = n;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xedb88320u ^ (c >> 1) : c >> 1;
            entries[n] = c;
        }
        return entries;
    }();
    for (size_t i = 0; i < size; ++i) value = table[(value ^ data[i]) & 0xff] ^ (value >> 8);
    return value;
}

void big(std::vector<uint8_t>& out, uint32_t value) {
    for (int shift = 24; shift >= 0; shift -= 8) out.push_back(uint8_t(value >> shift));
}

void chunk(std::vector<uint8_t>& out, const char* type, const std::vector<uint8_t>& body) {
    big(out, uint32_t(body.size()));
    const size_t start = out.size();
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), body.begin(), body.end());
    big(out, ~crc(out.data() + start, out.size() - start));
}

} // namespace

bool writePng(const std::string& path, int width, int height, const std::vector<uint8_t>& rgb) {
    if (rgb.size() != size_t(width) * size_t(height) * 3) return false;
    std::vector<uint8_t> raw;
    raw.reserve(size_t(height) * (size_t(width) * 3 + 1));
    for (int row = 0; row < height; ++row) {
        raw.push_back(0);
        const auto begin = rgb.begin() + ptrdiff_t(size_t(row) * size_t(width) * 3);
        raw.insert(raw.end(), begin, begin + ptrdiff_t(size_t(width) * 3));
    }

    std::vector<uint8_t> stream{0x78, 0x01};
    uint32_t a = 1, b = 0;
    for (uint8_t byte : raw) {
        a = (a + byte) % 65521;
        b = (b + a) % 65521;
    }
    for (size_t offset = 0; offset < raw.size() || offset == 0; offset += 65535) {
        const size_t size = std::min<size_t>(65535, raw.size() - offset);
        stream.push_back(offset + size >= raw.size() ? 1 : 0);
        stream.push_back(uint8_t(size));
        stream.push_back(uint8_t(size >> 8));
        stream.push_back(uint8_t(~size));
        stream.push_back(uint8_t(~size >> 8));
        stream.insert(stream.end(), raw.begin() + ptrdiff_t(offset), raw.begin() + ptrdiff_t(offset + size));
    }
    big(stream, (b << 16) | a);

    std::vector<uint8_t> file{0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a};
    std::vector<uint8_t> header;
    big(header, uint32_t(width));
    big(header, uint32_t(height));
    header.insert(header.end(), {8, 2, 0, 0, 0});
    chunk(file, "IHDR", header);
    chunk(file, "IDAT", stream);
    chunk(file, "IEND", {});

    std::FILE* output = std::fopen(path.c_str(), "wb");
    if (!output) return false;
    const bool written = std::fwrite(file.data(), 1, file.size(), output) == file.size();
    return std::fclose(output) == 0 && written;
}

} // namespace derivean::sim
