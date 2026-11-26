#include "groundwater/fn/Stages.hpp"

namespace derivean::sim {
namespace {

// The thickest unbroken run of soluble rock met so far in the lower storey.
struct Run {
    float top{}, bottom{}, best{};
    Material rock{Material::Air};
    bool open{};

    void close(AquiferColumn& column) {
        if (open && top - bottom > best) {
            best = top - bottom;
            column.solubleTop = top, column.solubleBottom = bottom, column.solubleRock = rock;
        }
        open = false;
    }
};

} // namespace

void fillStoreys(AquiferColumn& column, const WaterLayer* layers, int count) {
    // A bed holds water up when it passes less than a share of what arrives. Cover and basement
    // are not beds: one soaks, the other is the same rock all the way down.
    // Where nothing soaks in the same beds still hold up what arrives from the side.
    const float rate = std::max(LeastRecharge, column.recharge / WaterCellArea);
    int tight = -1;
    for (int i = 0; i < count && tight < 0; ++i)
        if (layers[i].unit != 0 && layers[i].through < PerchShare * rate && layers[i].top - layers[i].bottom >= 1.0f) tight = i;

    column.perched = tight >= 0;
    column.roof = column.ground;
    column.upperT = column.upperStore = column.upperSoluble = 0;
    column.shallowT = column.deepT = column.lowerStore = 0;
    column.solubleTop = 0, column.solubleBottom = 1, column.solubleRock = Material::Air;
    if (tight >= 0) {
        float thickest = 0;
        for (int i = 0; i < tight; ++i) {
            const float thickness = layers[i].top - layers[i].bottom;
            column.upperT += layers[i].along * thickness;
            column.upperStore += layers[i].porosity * thickness;
            // Only soluble rock that the perched water runs through counts: what lies on the tight bed.
            column.upperSoluble = layers[i].soluble >= SolubleEnough ? column.upperSoluble + thickness : 0.0f;
            if (thickness > thickest) thickest = thickness, column.upperRock = layers[i].rock;
        }
        column.floor = layers[tight].top;
        column.floorLeak = layers[tight].through * WaterCellArea;
        column.sheet = layers[tight].unit, column.sheetOffset = layers[tight].offset;
        column.floorRock = layers[tight].rock;
        column.roof = layers[tight].bottom;
    }

    Run run;
    float thickest = 0;
    int last = -1;
    for (int i = tight + 1; i < count; ++i) {
        const float top = std::min(layers[i].top, column.roof), bottom = std::max(layers[i].bottom, column.base);
        if (top <= bottom) continue;
        const float shallow = std::max(0.0f, top - std::max(bottom, column.shallowBase));
        column.shallowT += layers[i].along * shallow;
        column.deepT += layers[i].along * (top - bottom - shallow);
        column.lowerStore += layers[i].porosity * (top - bottom);
        if (top - bottom > thickest) thickest = top - bottom, column.lowerRock = layers[i].rock;
        if (layers[i].soluble >= SolubleEnough) {
            if (!run.open) run.top = top, run.rock = layers[i].rock, run.open = true;
            run.bottom = bottom;
        } else {
            run.close(column);
        }
        last = i;
    }
    run.close(column);
    // Under a tight bed with nothing below it that the model reads, what leaks through is gone.
    column.baseLeak = last >= 0 ? layers[last].through * DeepGradient * WaterCellArea : column.floorLeak;
}

} // namespace derivean::sim
