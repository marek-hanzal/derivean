#pragma once

#include <cstdio>
#include <string>

namespace derivean::sim {

// printf into a string, for the one-line reasons the measurements attach to what they find.
template<typename... Values>
std::string formatText(const char* format, Values... values) {
    char buffer[320];
    std::snprintf(buffer, sizeof(buffer), format, values...);
    return buffer;
}

} // namespace derivean::sim
