#include "aeolian/fn/Dunes.hpp"

namespace derivean::sim {

std::vector<int> windTurns(const WindRegime& regime, int year) {
    std::vector<int> turns;
    if (regime.potential <= 0) return turns;
    // Largest remainders: the rounds add up to the year and follow the shares as closely as whole rounds can.
    int given[RoseSectors], total = 0;
    double rest[RoseSectors];
    for (int k = 0; k < RoseSectors; ++k) {
        const double exact = year * regime.drift[size_t(k)] / regime.potential;
        given[k] = int(exact), rest[k] = exact - given[k], total += given[k];
    }
    for (; total < year; ++total) {
        int most = 0;
        for (int k = 1; k < RoseSectors; ++k)
            if (rest[k] > rest[most]) most = k;
        ++given[most], rest[most] = -1;
    }
    for (int k = 0; k < RoseSectors; ++k) turns.insert(turns.end(), size_t(given[k]), k);
    return turns;
}

} // namespace derivean::sim
