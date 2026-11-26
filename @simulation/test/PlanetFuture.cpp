#include "LandscapeFixture.hpp"

#include "measure/fn/History.hpp"

using namespace derivean::sim;

// The stock of rock is finite. However long the planet runs, relief, crust and land stay bounded:
// nothing may build without limit. The bounds are those of a rocky planet at all, well clear of
// where tuning moves the figures; they catch growth, not style.
void testPlanetFuture() {
    const PlanetSummary& far = farFuture();
    expectLandscape(far.highest < 12000, "the highest ground keeps growing", far.highest);
    expectLandscape(far.high99 < 8000, "high ground has spread over the land", far.high99);
    expectLandscape(far.thickest < 120000, "the thickest crust keeps growing", far.thickest);
    // Crust that averages CrustAtDatum floats level with the datum everywhere: no ocean basin is left.
    expectLandscape(far.meanCrust < 0.75 * CrustAtDatum, "the planet has made crust until its basins are gone", far.meanCrust);
    expectLandscape(far.landShare > 0.03 && far.landShare < 0.6, "the land has drowned or taken over the planet", far.landShare);
    expectLandscape(far.deepest > -9000, "the deepest sea floor left the range the sea level solver covers", far.deepest);
    const double moved = std::max(far.ledger.eroded, 1.0);
    expectLandscape(std::abs(far.ledger.eroded - far.ledger.deposited) < 0.01 * moved, "eroded rock is not laid down again", (far.ledger.eroded - far.ledger.deposited) / moved);
}
