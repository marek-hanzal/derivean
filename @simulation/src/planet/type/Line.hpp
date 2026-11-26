#pragma once

#include "planet/type/Planet.hpp"

namespace derivean::sim {

// A column with its record, in flight between two cells.
struct Parcel {
    CrustColumn column;
    ColumnRecord record;
};

// One grid line of the planet along the axis a plate is moving on; cells are addressed along it.
struct Line {
    Planet& planet;
    int fixed;
    bool alongX;
    bool* changed;   // set when a cell of the line changes hands
    size_t index(int i) const { return alongX ? planet.columns.index(i, fixed) : planet.columns.index(fixed, i); }
    CrustColumn& column(int i) { return planet.columns.cells[index(i)]; }
    ColumnRecord& record(int i) { return planet.records.cells[index(i)]; }
    int owner(int i) const { return planet.owner.cells[index(i)]; }
    void claim(int i, int plate) {
        planet.owner.cells[index(i)] = uint16_t(plate);
        *changed = true;
    }
    Parcel take(int i) { return {column(i), record(i)}; }
    void put(int i, const Parcel& parcel) {
        column(i) = parcel.column;
        record(i) = parcel.record;
    }
};

} // namespace derivean::sim
