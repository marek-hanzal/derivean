#pragma once

#include "icesheet/fn/Parent.hpp"

namespace derivean::sim {

// The last sheet at the finer levels (Local, Site). A finer domain settles without ice sheets; once
// it has, the sheet its parent recorded passes over it once more and leaves what is seen today:
// the cover it found taken up, rock dug out along weak zones, till lodged in the lee of every rise
// of the bed, and the tunnel fills of its streams standing on top as ridges. Every part reaches a
// few cells at most, well inside the halo, and cells a finished neighbour owns feed the ice as
// their own bed would, so two tiles that share an edge agree on it.

// Esker.cpp — the tunnel fill the parent spread over its cells is set aside from their cover, to
// stand as a ridge along the stream's path once the ice has gone (`layEskers`). A stretch's ridge
// is as wide as its tunnel and holds the stretch's own fill, so its height is the same in every
// domain that holds it. Tunnels come from a Region parent only: at Site the Local ridge is part of
// the parent's surface already.
EskerFill setAsideEskers(Domain& domain, const ParentSheet& parent, const std::vector<Tunnel>& tunnels);
void layEskers(Domain& domain, SheetWork& work, const EskerFill& fill);

// Lodge.cpp — the ice takes up the loose cover of the cells it slid over (as much as one glaciation
// strips) and, where the parent's sheets wore rock, digs the weaker rock deeper than the stronger
// around it, by up to BasinDepth (at Local only): the rock basins of a shield.
SheetLoad takeUpBed(Domain& domain, SheetWork& work, const ParentSheet& parent);

// Lodge.cpp — carries the load down the parent's ice surface and lodges it on the way, all the more
// in the cavity behind every rise of the bed, so till fills the lee of knobs, grows its own lee
// and runs out in tails: drumlins and flutes aligned with the flow. Where the ice ended, what
// arrives stays as moraine as steep as till stands; the meltwater takes the rest.
void lodgeTill(Domain& domain, SheetWork& work, const ParentSheet& parent, SheetLoad& carried);

// Local.cpp — the stages of the hook at the finer levels. `leaveFiner` returns false, and does
// nothing, where none of the domain lay under the last maximum's ice.
bool leaveFiner(Domain& domain, SheetWork& work, const ParentSheet& parent, const std::vector<Tunnel>& tunnels);
// The parent's ice over the domain's own ground (valleys take more than the ridges beside them),
// kept where it is thicker than what the domain's own glaciers hold; the tile's record, or null
// where the last sheet never reached the domain.
std::shared_ptr<const SheetRecord> presentFiner(Domain& domain, const SheetWork& work, const ParentSheet& parent);

} // namespace derivean::sim
