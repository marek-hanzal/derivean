// Coverage by union (S3-design.md §4, §11 test 7, Codex V2 and C2): a strip of 10⁻⁶ of a cell, the
// threshold ½ ± 10⁻⁶ from both sides, two footprints over the same 30 %, a ring of four with a hole,
// the mirror cases for births and a union independent of the order of its inputs.
#include "GlobeFixture.hpp"

#include "geometry/fn/Union.hpp"

#include <algorithm>

using namespace derivean::sim;
using namespace derivean::sim::globe_test;

namespace {

constexpr double H = 0.0055;   // half a level-7 cell in gnomonic units

PlanePolygon rect(double x0, double x1, double y0, double y1) {
    PlanePolygon p;
    p.count = 4;
    p.x = {x0, x1, x1, x0};
    p.y = {y0, y0, y1, y1};
    return p;
}

const PlanePolygon Cell = rect(-H, H, -H, H);

// The right edge x for which [-H, x] × [-H, H] covers `share` of the cell.
double edgeFor(double share) {
    const double target = share * sphericalArea(Cell);
    double low = -H, high = H;
    for (int i = 0; i < 200; ++i) {
        const double mid = 0.5 * (low + high);
        (sphericalArea(rect(-H, mid, -H, H)) < target ? low : high) = mid;
    }
    return 0.5 * (low + high);
}

void checkNarrowStrip() {
    const double width = 2 * H * 1e-6;
    const double strip = sphericalArea(rect(H - width, H, -H, H));
    const double overlap = coveredBy(Cell, {rect(H - width, 3 * H, -H, H)});
    expect(overlap >= 0, "a strip of 1e-6 of a cell gives a negative overlap", overlap);
    expect(std::abs(overlap / strip - 1) < 1e-3, "a strip of 1e-6 of a cell is off by 1e-3", overlap / strip - 1);
}

void checkThreshold() {
    const double area = sphericalArea(Cell);
    for (const double side : {+1.0, -1.0}) {
        // Two footprints over 30 % each with 10 % in common: the union is ½ ± 10⁻⁶.
        const double right = edgeFor(0.5 + side * 1e-6);
        const std::vector<PlanePolygon> over{clipped(rect(-H, edgeFor(0.3), -H, H), Cell), clipped(rect(edgeFor(0.2), right, -H, H), Cell)};
        const double covered = coveredBy(Cell, over);
        expect((covered >= 0.5 * area) == (side > 0), "the union decides the wrong side of ½ ± 1e-6", covered / area - 0.5);
        // The same in the births' view: uncovered ½ ∓ 10⁻⁶ of the cell, born exactly on the minus side.
        const double uncovered = area - unionArea(over).area;
        expect((uncovered >= 0.5 * area) == (side < 0), "a birth decides the wrong side of ½ ± 1e-6", uncovered / area - 0.5);
    }
    // Two footprints over the same 30 %: the sum says 60 %, the union 30 %.
    const PlanePolygon same = clipped(rect(edgeFor(0.7), 3 * H, -H, H), Cell);
    const double twice = coveredBy(Cell, {same, same});
    expect(std::abs(twice / area - 0.3) < 1e-9, "two footprints over the same 30 % do not cover 30 %", twice / area);
    // Two footprints over 60 % each with 30 % in common: 90 %, nothing is born.
    const double ninety = unionArea({rect(-H, edgeFor(0.6), -H, H), rect(edgeFor(0.3), edgeFor(0.9), -H, H)}).area / area;
    expect(std::abs(ninety - 0.9) < 1e-9, "60 % and 60 % with 30 % in common do not cover 90 %", ninety);
}

void checkHole() {
    const std::vector<PlanePolygon> ring{rect(-3 * H, 3 * H, -3 * H, -H), rect(-3 * H, 3 * H, H, 3 * H), rect(-3 * H, -H, -H, H), rect(H, 3 * H, -H, H)};
    const UnionArea u = unionArea(ring);
    expect(u.outers == 1 && u.holes == 1, "a ring of four is not one outer contour round one hole", u.holes);
    const double expected = sphericalArea(rect(-3 * H, 3 * H, -3 * H, 3 * H)) - sphericalArea(Cell);
    expect(std::abs(u.area / expected - 1) < 1e-9, "a ring's union is not outer minus hole", u.area / expected - 1);
}

void checkOrder() {
    std::vector<PlanePolygon> pieces;
    Random random{17};
    for (int i = 0; i < 12; ++i) {
        const double x = (2 * random.next() - 1) * H, y = (2 * random.next() - 1) * H, w = H * (0.2 + random.next());
        pieces.push_back(clipped(rect(x - w, x + w, y - w, y + w), Cell));
    }
    const double forward = unionArea(pieces).area;
    std::reverse(pieces.begin(), pieces.end());
    const double backward = unionArea(pieces).area;
    std::rotate(pieces.begin(), pieces.begin() + 5, pieces.end());
    const double turned = unionArea(pieces).area;
    expect(sameBits(forward, backward) && sameBits(forward, turned), "a union depends on the order of its footprints", forward - backward);
}

} // namespace

void testGlobeUnion() {
    checkNarrowStrip();
    checkThreshold();
    checkHole();
    checkOrder();
}
