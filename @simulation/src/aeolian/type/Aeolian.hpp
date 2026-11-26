#pragma once

#include "aeolian/type/Regime.hpp"

#include <vector>

namespace derivean::sim {

// What wind and evaporation have left in the cover, per cell in metres. Each is a part of one of
// the three cover classes, lying at its top; none adds height of its own, so every process that
// measures the surface as rock plus cover stays right. A process that takes cover away without
// knowing these is followed by `tidyLabels`, which never lets a label exceed its class.
struct WindLabels {
    std::vector<float> blown;   // of Sand: laid by wind
    std::vector<float> loess;   // of Fines: dust that settled from the air
    std::vector<float> lag;     // of Coarse: stones left at the surface when sand and dust between them were blown out
    std::vector<float> salt;    // of Fines: evaporite left where a basin's water left only as vapour

    void resize(size_t cells) {
        for (auto* label : {&blown, &loess, &lag, &salt}) label->assign(cells, 0.0f);
    }
};

// m³ of solid over the Free cells of a domain, halo included, for the whole replay. Sand:
// blown-in + lifted-from-cover = laid + drowned + blown-out. Dust: raised + arrived = settled + drowned + left.
struct WindLedger {
    double sandIn{}, sandOut{};       // across the edge of the free cells
    double sandLifted{}, sandLaid{};  // out of the cover and back onto it
    double sandDrowned{};             // into standing or running water, where it joins the bed
    double dustRaised{}, dustSettled{}, dustDrowned{};
    double dustIn{}, dustOut{};
    double salt{};                    // precipitated
};

// The wind's side of a domain while it is replayed.
struct WindField {
    WindLabels labels;
    WindRegime regime;                    // at the middle of the tile
    // Region only: the regime varies across a domain 300 km wide.
    std::vector<float> driftX, driftZ;    // m²/yr, resultant drift of a dry saturated bed
    std::vector<float> potential;         // m²/yr, drift of all sectors together
    // Local and Site: wind-laid sand is renewed by the wind far faster than water and slopes work,
    // so it is kept off the ground while the tile settles and laid back for the dune field.
    std::vector<float> parked;            // m
    float halfRange{10.0f};               // °C between the annual mean and the warmest month
    double span{};                        // yr of wind work the dune field of the tile stands for
    int rounds{};                         // rounds of the transport model behind it
    WindLedger ledger;
};

// What a finished tile keeps: the labels over its own cells, row-major.
struct WindRecord {
    WindLabels labels;
    WindRegime regime;
    double span{};
    int rounds{};
    WindLedger ledger;
};

} // namespace derivean::sim
