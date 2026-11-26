#include "atmosphere/type/Cloud.hpp"

#include "climate/constant/Moisture.hpp"

#include <cstdio>

namespace derivean::sim {
namespace {

std::string text(const char* format, double a = 0, double b = 0, double c = 0) {
    char line[200];
    std::snprintf(line, sizeof line, format, a, b, c);
    return line;
}

} // namespace

// Names the process that did most to saturate the column, then how far its clouds grow and of
// what they are made. Every figure is read from the column; nothing is looked up by genus.
std::string explainClouds(const AirColumn& column, const ColumnProfile& profile, const CloudReport& clouds) {
    if (clouds.cover < 0.05) {
        const double humidity = column.vapour[Low] * profile.perCapacity[Low];
        return text("clear: the low air holds %.0f %% of what it could and nothing lifts it", 100 * humidity);
    }
    std::string cause;
    const bool heaped = profile.release > 0.5;
    if (heaped) {
        cause = column.surfaceWarmth > 0.5 ? text("surface air heated %.1f °C above its surroundings rises", column.surfaceWarmth)
                                           : text("moist surface air rises on its own buoyancy");
        cause += text(", condenses at %.0f m", column.ground + profile.condensationLevel);
        if (profile.reach * profile.tower > 0.5)
            cause += text(" and stays buoyant to %.1f km (+%.1f °C at mid-level), with ice above %.1f km", clouds.convectiveTop / 1000,
                          profile.middleBuoyancy, (column.ground + profile.height[2]) / 1000);
        else if (profile.reach > 0.5)
            cause += text(" and is still %.1f °C warmer than the air at mid-level, stopping near %.1f km", profile.middleBuoyancy, clouds.convectiveTop / 1000);
        else if (clouds.convectiveTop > column.ground)
            cause += text(" and is stopped by stable air near %.0f m", clouds.convectiveTop);
        else
            cause += " and spreads out under a lid of stable air";
    } else if (column.glide > std::max(150.0, column.lift)) {
        cause = text("warm air glides %.0f m up over colder air and saturates from the top down", column.glide);
    } else if (column.lift > 150.0) {
        cause = text("converging air under cyclonic flow is lifted %.0f m and saturates", column.lift);
    } else if (column.climb > 100.0) {
        cause = text("moist air climbed %.0f m of terrain (ground at %.0f m) and saturated", column.climb, column.ground);
    } else if (column.surfaceWarmth < -1.0 && clouds.layer[Low].cover > 0.2) {
        cause = text("the ground cooled the air above it by %.1f °C at night until it saturated", -column.surfaceWarmth);
        if (column.hollow > 0.3) cause += ", and the cold air pools in the hollow";
    } else {
        cause = text("the low air holds %.0f %% of its capacity, enough to condense without being lifted", 100 * column.vapour[Low] * profile.perCapacity[Low]);
    }
    // The layer the named genus lives in; heaped cloud is described from its base to where its updraft ends.
    const CloudLayer* named = &clouds.layer[Low];
    for (const CloudLayer& layer : clouds.layer)
        if (layer.genus == clouds.genus && layer.cover > 0) named = &layer;
    const bool towering = clouds.genus == Genus::Cumulus || clouds.genus == Genus::Cumulonimbus;
    if (towering) named = &clouds.layer[Low];
    const double path = named->liquidPath + named->icePath;
    cause += text("; cloud from %.0f m to %.0f m", named->base, towering ? std::max(named->top, clouds.convectiveTop) : named->top);
    cause += path > 0 && named->icePath > 0.5 * path ? " of ice" : named->icePath > 0.05 * path ? " of droplets and ice" : " of droplets";
    cause += std::string(" → ") + genusName(clouds.genus);
    return cause;
}

} // namespace derivean::sim
