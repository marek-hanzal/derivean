#include "planet/fn/Crossing.hpp"

namespace derivean::sim {
namespace {

constexpr double JamScale = 2.5e6;        // m·cells of jammed continent that slows a plate to a quarter
constexpr double JamMemory = 30 * Myr;

// A maximal stretch of one plate along a grid line; `first` to `last` ascend and may wrap.
struct PlateRun {
    int plate{}, first{}, last{};
};

void shiftRun(Line line, PlateRun run, int direction, PlateMotion& motion, float strike, double speed) {
    const int count = PlanetCells;
    const int length = wrapIndex(run.last - run.first, count) + 1;
    const int lead = direction > 0 ? run.last : run.first;
    const int trail = direction > 0 ? run.first : run.last;

    Parcel carry = line.take(lead);
    (line.alongX ? carry.column.driftX : carry.column.driftZ) += int16_t(direction);
    int cell = lead;
    for (int moved = 1; moved < length; ++moved) {
        const int from = wrapIndex(cell - direction, count);
        line.column(cell) = line.column(from);
        (line.alongX ? line.column(cell).driftX : line.column(cell).driftZ) += int16_t(direction);
        line.record(cell) = line.record(from);
        cell = from;
    }
    // A plate behind that keeps pace will fill the gap itself; only a real parting stretches crust.
    const int behind = line.owner(wrapIndex(trail - direction, count));
    const Plate& follower = line.planet.epoch.plates[size_t(behind)];
    const double pursuit = (line.alongX ? follower.velocity.x : follower.velocity.z) * direction;
    const bool stretch = pursuit < 0.5 * speed;
    const int next = wrapIndex(trail + direction, count);
    if (length > 1) {
        line.put(trail, openGap(line.column(next), line.record(next), line.planet.time, stretch));
        crossContact(line, carry, wrapIndex(lead + direction, count), lead, direction, length, run.plate, motion, strike);
    } else {
        Parcel only = carry;
        line.put(trail, openGap(only.column, only.record, line.planet.time, stretch));
        crossContact(line, only, wrapIndex(lead + direction, count), lead, direction, length, run.plate, motion, strike);
    }
    // New floor at a parting belongs to both sides in turn, so the ridge stays between them.
    if (motion.shifts % 2 == 1 && behind != run.plate) line.claim(trail, behind);
}

// Lists the stretches of every plate along a line. A plate that circles the planet on it has no
// end to move and is left out.
void listRuns(const Line& line, std::vector<PlateRun>& runs) {
    const int count = PlanetCells;
    runs.clear();
    int start = -1;
    for (int i = 0; i < count && start < 0; ++i)
        if (line.owner(i) != line.owner(wrapIndex(i - 1, count))) start = i;
    if (start < 0) return;
    for (int k = 0; k < count;) {
        const int first = (start + k) % count;
        const int plate = line.owner(first);
        int length = 1;
        while (k + length < count && line.owner((start + k + length) % count) == plate) ++length;
        runs.push_back({plate, first, (first + length - 1) % count});
        k += length;
    }
}

// Moves every plate that is due along one axis by one cell. A plate is the set of cells it owns,
// so its stretches are read off the grid: the outline travels with the crust. Plates take their
// turn in index order on each line, and the line is read again whenever a cell changed hands.
void shiftPlates(Planet& planet, bool alongX, const std::vector<int>& direction) {
    const float strike = alongX ? 1.5707963f : 0.0f;
    std::vector<PlateRun> runs, own;
    for (int fixed = 0; fixed < PlanetCells; ++fixed) {
        bool changed = false;
        Line line{planet, fixed, alongX, &changed};
        listRuns(line, runs);
        for (int plate = 0; plate < int(direction.size()) && !runs.empty(); ++plate) {
            if (direction[size_t(plate)] == 0) continue;
            if (changed) {
                listRuns(line, runs);
                changed = false;
            }
            own.clear();
            for (const PlateRun& run : runs)
                if (run.plate == plate) own.push_back(run);
            const Vec2 velocity = planet.epoch.plates[size_t(plate)].velocity;
            for (const PlateRun& run : own)
                shiftRun(line, run, direction[size_t(plate)], planet.motion[size_t(plate)], strike, std::abs(alongX ? velocity.x : velocity.z));
        }
    }
    for (size_t plate = 0; plate < direction.size(); ++plate)
        if (direction[plate] != 0) ++planet.motion[plate].shifts;
}

} // namespace

// Convection reorganises: the crust is dealt out anew among the plates of the epoch.
void enterEpoch(Planet& planet, Workers& workers) {
    planet.epoch = epochAt(planet.seed, planet.time);
    planet.motion.assign(planet.epoch.plates.size(), PlateMotion{});
    workers.parallelFor(PlanetCells, [&](int begin, int end) {
        for (int z = begin; z < end; ++z) {
            for (int x = 0; x < PlanetCells; ++x) {
                planet.owner.at(x, z) = uint16_t(samplePlate(planet.epoch, planetCellCenter(x, z)).plate);
                planet.swell.at(x, z) = float(dynamicTopography(planet.epoch, planetCellCenter(x, z)));
            }
        }
    }, 8);
    traceContacts(planet);
}

void movePlates(Planet& planet, double dt) {
    const double activity = epochActivity(planet.epoch, planet.time);
    const size_t plates = planet.epoch.plates.size();
    std::vector<int> alongX(plates, 0), alongZ(plates, 0);
    bool any = false;
    for (size_t plate = 0; plate < plates; ++plate) {
        PlateMotion& motion = planet.motion[plate];
        motion.resistance *= std::exp(-dt / JamMemory);
        const double jam = 1.0 + motion.resistance / JamScale;
        const double pace = activity / (jam * jam) * dt / PlanetCell;
        motion.travelled = motion.travelled + planet.epoch.plates[plate].velocity * pace;
        if (std::abs(motion.travelled.x) >= 1) alongX[plate] = motion.travelled.x > 0 ? 1 : -1;
        if (std::abs(motion.travelled.z) >= 1) alongZ[plate] = motion.travelled.z > 0 ? 1 : -1;
        motion.travelled.x -= alongX[plate];
        motion.travelled.z -= alongZ[plate];
        any = any || alongX[plate] != 0 || alongZ[plate] != 0;
    }
    if (!any) return;
    shiftPlates(planet, true, alongX);
    shiftPlates(planet, false, alongZ);
    traceContacts(planet);
}

} // namespace derivean::sim
