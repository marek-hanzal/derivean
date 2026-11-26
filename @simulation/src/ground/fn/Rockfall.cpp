#include "ground/fn/Rockfall.hpp"

#include "core/fn/Hash.hpp"
#include "core/service/Workers.hpp"
#include "ground/type/Clast.hpp"

namespace derivean::sim {
namespace {

constexpr uint64_t FallStream = 0x46414c4c;
constexpr float Gravity = 9.81f;
constexpr float SizeSpread = 0.55f;      // log standard deviation of block size about the habit's median
constexpr float FrictionLarge = 0.55f;   // rolling resistance of a block far larger than the ground's pieces
constexpr float FrictionSmall = 0.50f;   // added for a small block
constexpr float FrictionRough = 0.25f;   // added on ground whose pieces are as large as the block
constexpr float SizeScale = 0.5f;        // m over which the small-block resistance dies away
constexpr float Catch = 0.4f;            // chance per metre of lodging between pieces of the block's own size, at rest speed
constexpr float CatchSpeed = 20.0f;      // m²/s² of speed that halves that chance
constexpr float Blow = 0.7f;             // share of v² lost per radian by which the slope eases under a moving block
constexpr int MostSteps = 600;

// Standard normal from two uniform draws.
float normal(uint64_t bits) {
    const double u = std::max(1e-12, unit(bits)), v = unit(scramble(bits));
    return float(std::sqrt(-2.0 * std::log(u)) * std::cos(6.283185307179586 * v));
}

} // namespace

bool FallGrid::contains(Vec2 p) const {
    const double fx = (p.x - origin.x) / cell - 0.5, fz = (p.z - origin.z) / cell - 0.5;
    return fx >= 0 && fz >= 0 && fx < width - 1 && fz < height - 1;
}

size_t FallGrid::cellAt(Vec2 p) const {
    const int x = std::clamp(int((p.x - origin.x) / cell), 0, width - 1), z = std::clamp(int((p.z - origin.z) / cell), 0, height - 1);
    return size_t(z) * size_t(width) + size_t(x);
}

float FallGrid::heightAt(Vec2 p) const {
    const double fx = std::clamp((p.x - origin.x) / cell - 0.5, 0.0, width - 1.001), fz = std::clamp((p.z - origin.z) / cell - 0.5, 0.0, height - 1.001);
    const int ix = int(fx), iz = int(fz);
    const float tx = float(fx - ix), tz = float(fz - iz);
    const float* a = &ground[size_t(iz) * size_t(width) + size_t(ix)];
    return (a[0] * (1 - tx) + a[1] * tx) * (1 - tz) + (a[width] * (1 - tx) + a[width + 1] * tx) * tz;
}

Vec2 FallGrid::gradientAt(Vec2 p) const {
    const double h = 0.5 * cell;
    return {(heightAt({p.x + h, p.z}) - heightAt({p.x - h, p.z})) / (2 * h), (heightAt({p.x, p.z + h}) - heightAt({p.x, p.z - h})) / (2 * h)};
}

std::vector<FallenBlock> shedBlocks(uint64_t seed, const FallGrid& surface, const std::vector<FallSource>& sources, double farthest, Workers* workers) {
    std::vector<std::vector<FallenBlock>> bySource(sources.size());
    const uint64_t base = stream(seed, FallStream);
    const float stride = float(0.25 * surface.cell);
    const auto follow = [&](int begin, int end) {
    for (int index = begin; index < end; ++index) {
        const FallSource& source = sources[size_t(index)];
        std::vector<FallenBlock>& fallen = bySource[size_t(index)];
        const float median = source.habit.size;
        const float typical = 0.55f * source.habit.flatness * median * median * median;   // m³ of a block of median size
        const int blocks = std::min(MostBlocksPerSource, int(source.volume / std::max(1e-4f, typical)));
        for (int n = 0; n < blocks; ++n) {
            const uint64_t draw = hash(base, int64_t(source.id), n);
            FallenBlock block;
            block.size = median * std::exp(SizeSpread * normal(draw));
            if (0.5f * block.size < SmallestClast) continue;   // waste too small to be a stone joins the scree
            block.size = std::min(block.size, 2.0f * LargestClast / 1.5f);
            block.flatness = source.habit.flatness;
            block.rock = source.rock;
            block.source = source.at;
            block.draw = draw;

            Vec2 at{source.at.x + (unit(hash(draw, 1)) - 0.5) * surface.cell, source.at.z + (unit(hash(draw, 2)) - 0.5) * surface.cell};
            Vec2 heading{};
            float speed = 0, travelled = 0, dip = 0;   // speed is v², m²/s²
            bool lost = !surface.contains(at), moving = false;
            for (int step = 0; step < MostSteps && !lost; ++step) {
                const Vec2 slope = surface.gradientAt(at);
                const double steep = length(slope);
                if (steep < 1e-4 && speed <= 0) break;
                const float rough = surface.roughness[surface.cellAt(at)];
                // The fall line, turned a little by whatever the block strikes; a moving block keeps some of its course.
                Vec2 fall = steep > 1e-6 ? slope * (-1.0 / steep) : heading;
                const double turn = (unit(hash(draw, 16 + step)) - 0.5) * 2.0 * std::min(0.7, 0.3 + 0.35 * std::sqrt(rough / block.size));
                fall = {fall.x * std::cos(turn) - fall.z * std::sin(turn), fall.x * std::sin(turn) + fall.z * std::cos(turn)};
                heading = moving ? normalized(heading * 0.55 + fall * 0.45) : fall;
                const Vec2 next = at + heading * double(stride);
                if (!surface.contains(next) || length(next - source.at) > farthest) {
                    lost = true;
                    break;
                }
                const float drop = surface.heightAt(at) - surface.heightAt(next);
                const float friction = FrictionLarge + FrictionSmall * std::exp(-block.size / SizeScale) + FrictionRough * std::min(1.0f, rough / block.size) +
                                       surface.drag[surface.cellAt(next)];
                const float angle = std::atan(drop / stride);
                if (moving && angle < dip) speed *= std::exp(-Blow * (dip - angle));
                dip = angle;
                speed += 2 * Gravity * (drop - friction * stride);
                if (speed <= 0) break;
                at = next;
                travelled += stride;
                moving = true;
                const float ratio = rough / block.size;
                const float caught = Catch * stride * ratio * ratio / (1.0f + speed / CatchSpeed);
                if (unit(hash(draw, 1024 + step)) < caught) break;
            }
            if (lost) continue;
            block.rest = at;
            block.travel = travelled;
            block.heading = float(std::atan2(heading.z, heading.x));
            fallen.push_back(block);
        }
    }
    };
    if (workers) workers->parallelFor(int(sources.size()), follow, 8);
    else follow(0, int(sources.size()));
    std::vector<FallenBlock> fallen;
    for (const std::vector<FallenBlock>& blocks : bySource) fallen.insert(fallen.end(), blocks.begin(), blocks.end());
    return fallen;
}

} // namespace derivean::sim
