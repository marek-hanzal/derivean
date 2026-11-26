#pragma once

#include "coast/fn/Survey.hpp"

#include <string>

// CoastCommand.cpp — the survey of a shore as text.
void printShore(const derivean::sim::ShoreGrid& grid, const derivean::sim::ShoreSurvey& survey);
void printCoastLedger(const derivean::sim::CoastLedger& ledger, double area);

// CoastMap.cpp — a map of what a shore is made of, `zoom` pixels to the cell: water by depth, pale
// where it is shallow; sand yellow, gravel grey-brown, mud brown, bare rock grey, all shaded by
// the relief; cliffs at the water's edge red, wave-cut platform violet, strips with water behind
// them outlined in orange, lagoons green-blue, stacks white.
void writeShoreMap(const derivean::sim::ShoreGrid& grid, const derivean::sim::ShoreSurvey& survey, const std::string& path, int zoom);

// An oblique picture of a hand-built domain from `eye`, `above` metres over the ground there, towards `target`.
void writeShoreView(const derivean::sim::Domain& domain, derivean::sim::Vec2 eye, derivean::sim::Vec2 target, double above, const std::string& path);
