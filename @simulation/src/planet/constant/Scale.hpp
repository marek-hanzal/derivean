#pragma once

namespace derivean::sim {

// The world is flat and repeats after PlanetSpan on both axes. A finite surface is what makes the
// water, salt and rock budgets closed; at 16 777 km the repeat is out of reach of ordinary play.
inline constexpr int PlanetCells = 256;
inline constexpr double PlanetCell = 65536.0;                    // m
inline constexpr double PlanetSpan = PlanetCells * PlanetCell;   // m
inline constexpr double PlanetCellArea = PlanetCell * PlanetCell;

// Vertical extent of the playable solid. Below WorldFloor is the void that water can escape into.
// Real depths are compressed: 8 km of game depth spans crust, lithospheric mantle and the melt zone.
inline constexpr double WorldFloor = -8192.0;                    // m
inline constexpr double MantleTop = -5120.0;                     // m, crystalline crust above
inline constexpr double MeltTop = -7168.0;                       // m, partially molten below

// Airy isostasy of felsic crust floating on the mantle.
inline constexpr double CrustDensity = 2800.0;                   // kg/m³
inline constexpr double MantleDensity = 3300.0;
inline constexpr double IsostaticRise = 1.0 - CrustDensity / MantleDensity;   // surface rise per m of thickening
inline constexpr double IsostaticSink = CrustDensity / MantleDensity;         // root growth per m of thickening
inline constexpr double CrustAtDatum = 33000.0;                  // m of crust that floats level with y = 0
inline constexpr double OceanicLimit = 6000.0;                   // m of felsic crust below which a column acts oceanic
inline constexpr double ContinentalLimit = 16000.0;              // m above which it floats as a continent

} // namespace derivean::sim
