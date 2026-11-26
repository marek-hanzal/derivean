#pragma once

#include "ground/fn/SiteView.hpp"
#include "ground/type/Stones.hpp"

namespace derivean::sim {

// Lays the stone if the place is free and the coarse layer around it holds its volume; a layer that
// holds a good part of it gets a smaller stone. False if nothing was laid.
bool layStone(StoneBed& bed, const StoneOffer& offer);

// Talus.cpp — blocks that broke off the site level's steep faces within reach and came to rest in the
// tile. A face releases what its rock mass cuts out; `setting` gives the layering it stands in.
void dropBlocks(StoneBed& bed, const SiteView& sites, const Setting& setting);

// Fabric.cpp — the surface of the coarse layer as individual stones, where one shows at the surface:
// lag over the rock it weathered from, scree under a failing face, the armour of a stream bed, the
// boulders of a till, the blocks of frozen ground.
void packFabric(StoneBed& bed, const FlowScratch& scratch, const SiteView& sites, const TileLookup& lookup);

// Bed.cpp — steps and pools: the largest stones of a steep channel hold where they lie, smaller ones
// jam against them into ribs, and the water scours a pool below each and below every fall over rock.
void organiseBeds(StoneBed& bed, const FlowScratch& scratch);

} // namespace derivean::sim
