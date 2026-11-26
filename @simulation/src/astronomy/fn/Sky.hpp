#pragma once

#include "astronomy/type/Instant.hpp"
#include "astronomy/type/Sky.hpp"

namespace derivean::sim {

Sky skyAt(const Instant& instant);

} // namespace derivean::sim
