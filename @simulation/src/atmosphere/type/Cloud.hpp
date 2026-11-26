#pragma once

#include "atmosphere/constant/Snow.hpp"
#include "atmosphere/type/Column.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace derivean::sim {

// The ten cloud genera of the WMO International Cloud Atlas.
enum class Genus : uint8_t {
    Cumulus, Cumulonimbus, Stratus, Stratocumulus, Nimbostratus, Altocumulus, Altostratus, Cirrus, Cirrocumulus, Cirrostratus
};
inline constexpr int Genera = 10;

const char* genusName(Genus genus);

// The cloud of one band of a column.
struct CloudLayer {
    double cover{};          // share of the sky
    double base{}, top{};    // m above the sea
    double liquidPath{};     // kg/m²
    double icePath{};        // kg/m²
    double opticalDepth{};   // visible, straight down
    Genus genus{};           // the genus most of this layer belongs to
};

// What the clouds of a column are. Nothing here is drawn from chance or picked from a list: the
// genus shares are continuous functions of the column's state, so a cumulus that keeps growing
// turns into a cumulonimbus by degrees.
struct CloudReport {
    std::array<CloudLayer, AirBands> layer;
    std::array<double, Genera> share{};   // share of the sky each genus covers; layers overlap, so the shares can sum to more than one
    Genus genus{};                        // the genus with the largest share
    double cover{};                       // share of the sky, all layers together
    double opticalDepth{};
    double shade{};                       // share of sunlight turned away
    double convectiveTop{};               // m above the sea that updrafts reach; the ground height when there are none
};

// `precipitation` is what reaches the ground in kg/m²/s.
CloudReport describeClouds(const AirColumn& column, const ColumnProfile& profile, double precipitation);

// One sentence on which process saturated the column and how far its clouds grow.
std::string explainClouds(const AirColumn& column, const ColumnProfile& profile, const CloudReport& clouds);

// Share of sunlight the column's clouds turn away; the same measure CloudReport::shade gives.
double cloudShade(const AirColumn& column);

// Unit vector towards the sun in world axes (+X, up, +Z); below the horizon `up` is negative.
struct SunDirection {
    double x{}, up{}, z{};
};

// World-anchored cloud description for a renderer. Channel entry (x, z) of every band belongs to
// the column centred at ((x + ½), (z + ½)) · cell in world metres, and the whole field repeats
// after `span`. The renderer adds the detail below a cell; coverage, height, thickness, phase,
// shape and motion come from here.
struct CloudField {
    int size{};
    double cell{}, span{};   // m
    struct BandChannels {
        std::vector<float> cover;          // share of the sky
        std::vector<float> base, top;      // m above the sea
        std::vector<float> opticalDepth;
        std::vector<float> iceShare;       // of the condensed water
        std::vector<float> windX, windZ;   // m/s, direction of travel, to carry fine detail along
        std::vector<uint8_t> genus;        // Genus
    };
    std::array<BandChannels, AirBands> band;
    std::vector<float> heaping;      // 0 flat sheets to 1 towers: ColumnProfile::convection
    std::vector<float> rain, snow;   // kg/m²/s at the ground
    // kg/m² of snow water equivalent (a millimetre of water each) lying on ground at SnowLadder[rung]
    // metres above `seaLevel`: entry [cell · SnowRungs + rung], with cell = z · size + x. To read
    // it at the height of the terrain as Atmosphere's snowAt does: per cell nothing below
    // snowLine, linear from there to the first rung above it and between rungs further up, the end
    // rungs beyond the ladder; then linear between the four cells around the position.
    std::vector<float> snowLadder;
    std::vector<float> snowLine;     // m above `seaLevel` per cell (snowLineOf)
    std::vector<float> aerosolDepth; // visible optical depth of fine ash and sulphate veil over the cell above the low band: dims and reddens the sun
    std::vector<float> hazeDepth;    // optical depth at 550 nm of the low band's haze and ash, spread evenly through hazeThickness below hazeTop
    std::vector<float> hazeTop;      // m above `seaLevel` of the top of the low band
    double hazeThickness{};          // m of the low band below hazeTop
    std::vector<float> ashAloft;     // kg/m² of fine ash in the air over the cell: where there is an ash cloud to draw
    std::vector<float> ashLevel;     // m above `seaLevel` around which most of that ash floats
    double seaLevel{};               // m against the datum
};

} // namespace derivean::sim
