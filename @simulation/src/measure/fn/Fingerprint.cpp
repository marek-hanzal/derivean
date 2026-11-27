#include "measure/fn/Fingerprint.hpp"

#include "aeolian/type/Aeolian.hpp"
#include "coast/type/Shore.hpp"
#include "ground/type/Detail.hpp"
#include "icesheet/type/Sheet.hpp"
#include "measure/fn/Digest.hpp"
#include "volcanic/type/Pile.hpp"

namespace derivean::sim {

namespace {

// Feeds every field a finished tile keeps into one FNV state: a change of any bit of any of them
// changes the print. Structs with padding go in field by field.
struct Print {
    uint64_t state{0x9e3779b97f4a7c15ULL};
    template<typename T> void value(const T& v) { state = digestOf(&v, sizeof(T), state); }
    template<typename T> void array(const std::vector<T>& values) {
        value(values.size());
        if (!values.empty()) state = digestOf(values.data(), values.size() * sizeof(T), state);
    }
    void ground(const Ground& g) {
        array(g.rock), array(g.datum);
        for (const auto& layer : g.sediment) array(layer);
    }
    void labels(const WindLabels& l) { array(l.blown), array(l.loess), array(l.lag), array(l.salt); }
    void ledger(const TileLedger& l) { value(l.uplifted), value(l.eroded), value(l.deposited), value(l.weathered), value(l.dissolved), value(l.failed), value(l.sedimentIn), value(l.sedimentOut); }
    void coast(const CoastLedger& l) { value(l.cut), value(l.dissolved), value(l.entered), value(l.left); }
};

} // namespace

uint64_t fingerprintOf(const Tile& tile) {
    Print p;
    p.value(tile.key.x), p.value(tile.key.z), p.value(tile.time), p.value(tile.seaLevel);
    p.ground(tile.now), p.ground(tile.nowWide);
    for (const auto* field : {&tile.water, &tile.discharge, &tile.sunk, &tile.seeped, &tile.load, &tile.uplift, &tile.rebound, &tile.ice, &tile.iceAge, &tile.iceSpeed,
                              &tile.erosionRate, &tile.depositionRate, &tile.eroded, &tile.deposited, &tile.uplifted, &tile.rain, &tile.warmth, &tile.shed})
        p.array(*field);
    p.array(tile.flowTo), p.array(tile.rock);
    for (const IceSource& source : tile.upIce) p.value(source.source.x), p.value(source.source.z), p.value(source.rock);
    if (const GroundDetail* d = tile.detail.get()) {
        p.value(d->clasts.size());
        for (const Clast& c : d->clasts) {
            const float f[12] = {c.x, c.z, c.y, c.a, c.b, c.c, c.yaw, c.pitch, c.roll, c.roundness, c.travel, 0};
            p.value(f), p.value(c.source.x), p.value(c.source.z), p.value(c.rock), p.value(c.origin);
        }
        p.array(d->binStart), p.array(d->crust), p.array(d->crustKind), p.array(d->coarseKind), p.array(d->fineKind), p.array(d->bare), p.array(d->stepGaps);
        p.value(d->ledger.coarseBefore), p.value(d->ledger.dug), p.value(d->ledger.matrix), p.value(d->ledger.stones), p.value(d->ledger.offered), p.value(d->ledger.byOrigin);
    } else p.value(-1);
    if (const ShoreRecord* s = tile.shore.get()) p.array(s->height), p.array(s->notch), p.array(s->cut), p.array(s->laid), p.coast(s->ledger);
    else p.value(-2);
    if (const VolcanicTile* v = tile.volcanic.get()) {
        p.array(v->pile), p.array(v->laid), p.array(v->lava), p.array(v->flows), p.array(v->fall), p.array(v->wet), p.array(v->lavaUnit), p.array(v->vent);
        p.array(v->wide), p.array(v->wideLava), p.array(v->wideFlows), p.array(v->wideFall), p.array(v->wideWet), p.array(v->wideUnit);
        for (const Vent& vent : v->vents)
            p.value(vent.id), p.value(vent.centre.x), p.value(vent.centre.z), p.value(vent.vigor), p.value(vent.begin), p.value(vent.until), p.value(vent.supply),
                p.value(vent.erupted), p.value(vent.held), p.value(vent.chamber), p.value(vent.count), p.value(vent.recent.size()), p.value(vent.episodes.size());
        for (const Caldera& c : v->calderas) p.value(c.vent), p.value(c.site.x), p.value(c.site.z), p.value(c.volume), p.value(c.time), p.value(c.cut), p.value(c.kept), p.value(c.wall);
        const VolcanicLedger& l = v->ledger;
        p.value(l.erupted), p.value(l.lava), p.value(l.cone), p.value(l.ash), p.value(l.left), p.value(l.drifted), p.value(l.sealed), p.value(l.foundered), p.value(l.built);
    } else p.value(-3);
    if (const SheetRecord* s = tile.sheet.get()) {
        p.value(s->reached), p.array(s->slideX), p.array(s->slideZ), p.array(s->worn), p.array(s->till), p.array(s->moraine), p.array(s->outwash), p.array(s->esker);
        p.array(s->meltwater), p.array(s->meltTo), p.array(s->sheetThen);
        for (int c = 0; c < SedimentClasses; ++c) p.array(s->passing[c]), p.array(s->laidBack[c]);
        for (const auto* crossings : {&s->eskerArrived, &s->eskerSent})
            for (const EskerCrossing& e : *crossings) p.value(e.x), p.value(e.z), p.value(e.tx), p.value(e.tz), p.value(e.volume);
    } else p.value(-4);
    const WindRecord& w = tile.aeolian;
    p.labels(w.labels), p.labels(w.wide), p.array(w.ebb), p.value(w.span), p.value(w.rounds);
    const WindLedger& wl = w.ledger;
    for (const double v : {wl.sandIn, wl.sandOut, wl.sandLifted, wl.sandLaid, wl.sandDrowned, wl.dustRaised, wl.dustSettled, wl.dustDrowned, wl.dustIn, wl.dustOut, wl.salt}) p.value(v);
    p.ledger(tile.ledger);
    return p.state;
}

} // namespace derivean::sim
