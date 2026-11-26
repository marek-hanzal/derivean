#pragma once

#include "Arguments.hpp"

#include "coast/fn/Survey.hpp"

#include "simulation/type/World.hpp"

#include <string>
#include <vector>

// CoastCommand.cpp — the survey of a shore as text.
void printShore(const derivean::sim::ShoreGrid& grid, const derivean::sim::ShoreSurvey& survey);
void printCoastLedger(const derivean::sim::CoastLedger& ledger, double area);

// CoastMap.cpp — a map of what a shore is made of, `zoom` pixels to the cell: water by depth, pale
// where it is shallow and tinted by a loose bed; sand yellow, gravel grey-brown, mud brown, bare rock grey, all shaded by
// the relief; cliffs at the water's edge red, wave-cut platform violet, strips with water behind
// them outlined in orange, lagoons green-blue, stacks white.
void writeShoreMap(const derivean::sim::ShoreGrid& grid, const derivean::sim::ShoreSurvey& survey, const std::string& path, int zoom);

// An oblique picture of a hand-built domain from `eye`, `above` metres over the ground there, towards `target`.
void writeShoreView(const derivean::sim::Domain& domain, derivean::sim::Vec2 eye, derivean::sim::Vec2 target, double above, const std::string& path);

// CoastFind.cpp — `terra coast --find 1`: sea coasts and lakes around a start, ranked on the Region and surveyed at a finer level.
int coastFind(const Arguments& arguments);

// CoastShow.cpp — a shore near `at` surveyed on 2 × 2 Local tiles, then for each of `kinds` the best
// shore of that kind surveyed on 2 × 2 Site tiles and pictured from its water and from the shore;
// maps and pictures go to `out`.
void examineShore(derivean::sim::World& world, derivean::sim::Vec2 at, const std::vector<derivean::sim::ShoreKind>& kinds, bool site, const std::string& out, const std::string& tag);
