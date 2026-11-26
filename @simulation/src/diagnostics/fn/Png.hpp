#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace derivean::sim {

// Uncompressed 8-bit RGB PNG for maps and probes; diagnostics must not pull in an image library.
bool writePng(const std::string& path, int width, int height, const std::vector<uint8_t>& rgb);

} // namespace derivean::sim
