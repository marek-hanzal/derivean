#pragma once

#include "hydrology/type/Exchange.hpp"

namespace derivean::sim {

// Exchange.cpp — the ledger of one integration. `openExchange` takes what every lake holds from
// the cells. `book` debits the donor basin and pours the same volume, with the donor lake's salt,
// at `leaf` into the basin with room there (which must be `receiver`), books it to both bodies and
// dissolves the crust of the cells the receiver's surface now covers. `relake` follows lakes that
// merged or parted; their water is the sum of their basins' own, their salt goes with it at the
// strength it had, and both are booked to the bodies they left and joined. `closeExchange` writes
// the lakes that took part back to their cells, leaves every other cell exactly as it was, counts
// a change of the sea in `Ocean::changes`, and closes each body against its account.
Exchange openExchange(const Ocean& ocean);
Tally book(Ocean& ocean, Exchange& exchange, int32_t donor, int32_t receiver, int32_t leaf, double volume);
void relake(Ocean& ocean, Exchange& exchange);
void closeExchange(Ocean& ocean, Exchange& exchange);

// Strait.cpp — `seconds` of one throat, integrated exactly between the moments its ends, its donor,
// its receiver or their wet areas change. What it moves is credited to its straits by area.
void flowThrough(Ocean& ocean, Exchange& exchange, const Throat& throat, double seconds);

} // namespace derivean::sim
