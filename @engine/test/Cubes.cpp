#include "horizon/type/FarCubes.hpp"
#include "nature/type/SurfaceReader.hpp"

#include "PlanetSession.hpp"

#include <cmath>
#include <unordered_set>

using namespace derivean;

namespace {

struct Quad {
    int axis, sign, plane, u, v, width, height, material, shade;
    int corner(int alongU, int alongV) const { return (shade >> (2 * (alongU + 2 * alongV))) & 3; }
};

std::vector<Quad> quadsOf(const FaceMesh& faces) {
    expect(faces.size() % FaceWords == 0, "A face mesh must hold whole quads");
    std::vector<Quad> quads;
    for (size_t at = 0; at < faces.size(); at += FaceWords)
        quads.push_back({faces[at], faces[at + 1], faces[at + 2], faces[at + 3], faces[at + 4], faces[at + 5], faces[at + 6], faces[at + 7], faces[at + 8]});
    return quads;
}

// Every exposed face of a small scene is covered once, by a quad of its material whose corner
// values, spread across the quad, are the occlusion each of its cells has on its own.
void occlusionIsExact() {
    const Bounds inner{{0, 0, 0}, {8, 8, 8}};
    Voxels voxels(inner.padded(1));
    for (int z = -1; z < 9; ++z)
        for (int x = -1; x < 9; ++x) {
            voxels[{x, -1, z}] = voxels[{x, 0, z}] = Material::Bedrock;
            if (x >= 5) voxels[{x, 1, z}] = Material::Sand;
        }
    for (int y = 1; y < 4; ++y) voxels[{2, y, 3}] = Material::Soil;
    voxels[{3, 3, 3}] = Material::Soil;
    for (int y = 1; y < 8; ++y) voxels[{0, y, 7}] = Material::Shale;   // against two sides and the top of the meshed box

    size_t exposed = 0;
    for (int y = 0; y < 8; ++y)
        for (int z = 0; z < 8; ++z)
            for (int x = 0; x < 8; ++x)
                for (Cell normal : Neighbors) exposed += voxels[{x, y, z}] != Material::Air && voxels[Cell{x, y, z} + normal] == Material::Air;
    size_t covered = 0, shaded = 0;
    for (const Quad& quad : quadsOf(meshFaces(voxels, inner))) {
        const int u = (quad.axis + 1) % 3, v = (quad.axis + 2) % 3;
        if (quad.width > 1) expect(quad.corner(0, 0) == quad.corner(1, 0) && quad.corner(0, 1) == quad.corner(1, 1), "A quad was stretched along a change of occlusion");
        if (quad.height > 1) expect(quad.corner(0, 0) == quad.corner(0, 1) && quad.corner(1, 0) == quad.corner(1, 1), "A quad was stretched along a change of occlusion");
        for (int q = 0; q < quad.height; ++q)
            for (int p = 0; p < quad.width; ++p) {
                Cell solid{};
                solid[quad.axis] = quad.plane - (quad.sign > 0 ? 1 : 0);
                solid[u] = quad.u + p;
                solid[v] = quad.v + q;
                Cell front = solid;
                front[quad.axis] += quad.sign;
                expect(int(voxels[solid]) == quad.material && voxels[front] == Material::Air, "A quad covers a face that is not there or not of its material");
                for (int corner = 0; corner < 4; ++corner) {
                    Cell alongU{}, alongV{};
                    alongU[u] = corner & 1 ? 1 : -1;
                    alongV[v] = corner & 2 ? 1 : -1;
                    const bool first = voxels[front + alongU] != Material::Air, second = voxels[front + alongV] != Material::Air;
                    const int own = first && second ? 3 : int(first) + int(second) + int(voxels[front + alongU + alongV] != Material::Air);
                    expect(own == quad.corner(corner & 1, corner >> 1), "A quad's corners do not shade its cells as they shade themselves");
                    shaded += own > 0;
                }
                ++covered;
            }
    }
    expect(covered == exposed && shaded > 20, "The mesh must cover every exposed face exactly once, shaded where cells meet");
}

// Every quad of a column of chunks with its vertical coordinate in whole cubes of the level.
std::vector<Quad> quadsOf(const CubeColumn& column) {
    std::vector<Quad> all;
    for (const CubeChunk& chunk : column.chunks) {
        expect(chunk.level == column.key.level && chunk.at.x == column.key.x && chunk.at.z == column.key.z, "A chunk lies outside its column");
        for (Quad quad : quadsOf(chunk.faces)) {
            expect(quad.material > 0 && quad.material < int(Materials.size()), "A cube face names no material");
            expect(quad.plane >= 0 && quad.plane <= CubeSide && quad.u >= 0 && quad.v >= 0 && quad.u + quad.width <= CubeSide && quad.v + quad.height <= CubeSide,
                   "A cube face leaves its chunk");
            (quad.axis == 1 ? quad.plane : quad.axis == 0 ? quad.u : quad.v) += chunk.at.y * CubeSide;
            all.push_back(quad);
        }
    }
    return all;
}

// Tops of the columns in whole cubes of the level, from the upward faces.
std::vector<int> topsOf(const std::vector<Quad>& quads) {
    std::vector<int> tops(size_t(CubeSide) * CubeSide, std::numeric_limits<int>::min());
    for (const Quad& quad : quads) {
        if (quad.axis != 1 || quad.sign < 0) continue;
        for (int x = quad.v; x < quad.v + quad.height; ++x)
            for (int z = quad.u; z < quad.u + quad.width; ++z) {
                int& top = tops[size_t(z) * CubeSide + x];
                expect(top == std::numeric_limits<int>::min(), "A column of far cubes has two tops");
                top = quad.plane;
            }
    }
    return tops;
}

// Cubes of the faces on the side towards +x or -x that stand in the row of columns `z`.
int sideCubes(const std::vector<Quad>& quads, int sign, int z) {
    int cubes = 0;
    for (const Quad& quad : quads)
        if (quad.axis == 0 && quad.sign == sign && quad.plane == (sign > 0 ? CubeSide : 0) && quad.v <= z && z < quad.v + quad.height) cubes += quad.width;
    return cubes;
}

// Chunks are cut alone and must still fit: tops follow the rule, neighbours close the wall between
// them, and water lies in its basin.
void chunksFit(const sim::World& world, Position around) {
    for (int level : {FinestCubes, 5, 9, CoarsestCubes}) {
        const double edge = edgeOf(level), size = CubeSide * edge;
        const CubeKey key{level, int32_t(std::floor(around[0] / size)), int32_t(std::floor(around[2] / size))};
        const CubeColumn here = buildCubes(world, key), beside = buildCubes(world, {level, key.x + 1, key.z});
        const std::vector<Quad> quads = quadsOf(here), others = quadsOf(beside);
        const std::vector<int> tops = topsOf(quads), next = topsOf(others);
        const std::vector<WaterSheet>& water = here.chunks.back().water;
        SurfaceReader reader = surfaceReader(world, cubeSource(level));
        int near = 0;
        for (int z = 0; z < CubeSide; ++z) {
            for (int x = 0; x < CubeSide; ++x) {
                const int top = tops[size_t(z) * CubeSide + x];
                expect(top != std::numeric_limits<int>::min(), "A column of far cubes has no top");
                const double ground = surfaceEased(reader, {(key.x * CubeSide + x + 0.5) * edge, (key.z * CubeSide + z + 0.5) * edge}).ground;
                expect(std::abs(top * edge - ground) <= 1.5 * edge + 1e-3, "A column of far cubes ends far from the ground");
                near += std::abs(top * edge - ground) <= 0.5 * edge + 1e-3;
            }
            const int step = tops[size_t(z) * CubeSide + CubeSide - 1] - next[size_t(z) * CubeSide];
            expect(sideCubes(quads, 1, z) == std::max(step, 0) && sideCubes(others, -1, z) == std::max(-step, 0), "Neighbouring chunks leave their border open or doubled");
        }
        expect(water.size() > 0 || near == CubeSide * CubeSide, "Away from water a column's top is the cube nearest the ground");
        for (const WaterSheet& sheet : water) {
            std::vector<uint8_t> wet(tops.size(), 0);
            for (const Quad& quad : quadsOf(sheet.faces))
                for (int x = quad.v; x < quad.v + quad.height; ++x)
                    for (int z = quad.u; z < quad.u + quad.width; ++z) wet[size_t(z) * CubeSide + x] = 1;
            for (int z = 1; z < CubeSide - 1; ++z)
                for (int x = 1; x < CubeSide - 1; ++x) {
                    if (!wet[size_t(z) * CubeSide + x]) continue;
                    expect(tops[size_t(z) * CubeSide + x] * edge < sheet.level, "Cubes stand out of the water that covers them");
                    for (Cell side : {Cell{1, 0, 0}, Cell{-1, 0, 0}, Cell{0, 0, 1}, Cell{0, 0, -1}}) {
                        const size_t shore = size_t(z + side.z) * CubeSide + size_t(x + side.x);
                        if (!wet[shore] && water.size() == 1) expect(tops[shore] * edge >= sheet.level - 0.01, "Water stands above the dry cubes beside it");
                    }
                }
        }
        std::cout << "cubes of " << edge << " m: " << here.faces << " faces in " << here.chunks.size() << " chunks, " << water.size() << " sheets, built in "
                  << here.milliseconds << " ms from data level " << int(here.chunks.front().dataLevel) << "\n";
    }
}

struct Held {
    std::unordered_map<uint32_t, CubeChunk> byId;   // without their faces
    std::unordered_map<Cell, int, CellHash> keys;   // {x, level, z}: chunks held over that square of ground
};

// Applies a frame's delta as a renderer would and checks what the frame promises: no chunk twice,
// boxes nested, and every level whole inside its box and a band beyond.
void follow(FarCubes& far, Held& held, Nature& nature, Position eye, double clearance, double speed) {
    CubeDelta delta;
    advanceCubes(far, nature, eye, clearance, speed, delta);
    for (uint32_t id : delta.removed) {
        const auto chunk = held.byId.find(id);
        expect(chunk != held.byId.end(), "A cube chunk was removed that was never added");
        const Cell square{chunk->second.at.x, chunk->second.level, chunk->second.at.z};
        if (--held.keys[square] == 0) held.keys.erase(square);
        held.byId.erase(chunk);
    }
    // A square of ground is swapped whole: what was held over it is gone before the new chunks count.
    std::unordered_set<Cell, CellHash> arrived;
    for (const CubeChunk& chunk : delta.added) {
        const Cell square{chunk.at.x, chunk.level, chunk.at.z};
        expect(held.byId.emplace(chunk.id, CubeChunk{chunk.id, chunk.level, chunk.at, {}, {}, chunk.dataLevel}).second, "A cube chunk id was used twice");
        if (arrived.insert(square).second) expect(!held.keys.contains(square), "Two builds of one square of ground are held at once");
        for (const auto& [id, other] : held.byId) expect(id == chunk.id || other.level != chunk.level || !(other.at == chunk.at), "Two cube chunks fill the same place");
        ++held.keys[square];
        expect(chunk.dataLevel <= uint8_t(cubeSource(chunk.level)), "A cube chunk reads finer data than its cubes can show");
    }
    double outer = std::numeric_limits<double>::max();
    for (int level = CoarsestCubes; level >= FinestCubes; --level) {
        const CubeLevel& state = far.levels[size_t(level - FinestCubes)];
        if (!state.box) { outer = 0; continue; }
        const double side = state.box->max[0] - eye[0], size = CubeSide * edgeOf(level), reach = side + CubeBand * edgeOf(level);
        expect(side <= outer + 1e-6, "The box of a finer level reaches beyond the coarser one");
        outer = side;
        for (int z = int(std::floor((eye[2] - reach) / size)); z <= int(std::floor((eye[2] + reach) / size)); ++z)
            for (int x = int(std::floor((eye[0] - reach) / size)); x <= int(std::floor((eye[0] + reach) / size)); ++x)
                expect(held.keys.contains({x, level, z}), "A level's box reaches over ground the renderer holds no chunk for");
    }
}

template<typename Done>
bool settle(FarCubes& far, Held& held, Nature& nature, Position eye, double clearance, double seconds, Done done) {
    const auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() < seconds) {
        follow(far, held, nature, eye, clearance, 0);
        if (done()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return false;
}

void levelsFollowTheEye(Nature& nature, Position eye) {
    FarCubes far;
    far.finished = std::make_shared<Mailbox<CubeColumn>>();
    Held held;
    const auto coarsest = [&] { return far.levels.back().box.has_value(); };
    const auto complete = [&] { return std::all_of(far.levels.begin(), far.levels.end(), [](const CubeLevel& level) { return level.held == level.wanted; }); };
    const auto whole = [&] { return complete() && far.levels.front().box && far.levels.front().box->max[0] - eye[0] > far.reach * edgeOf(FinestCubes) - 1e-6; };
    expect(settle(far, held, nature, eye, 1.7, 20, coarsest), "The coarsest cubes must cover the view within moments");
    expect(settle(far, held, nature, eye, 1.7, 300, whole), "The levels of far cubes never became whole");
    for (const CubeLevel& level : far.levels)
        std::cout << "level of " << edgeOf(FinestCubes + int(&level - far.levels.data())) << " m: " << level.shown << " chunks, "
                  << level.faces / std::max<size_t>(1, level.builds) << " faces and " << level.buildMs / double(std::max<size_t>(1, level.builds)) << " ms a chunk\n";

    // A run across the ground, then a climb at a speed the fine levels cannot keep up with.
    for (int frame = 0; frame < 600; ++frame) {
        eye[0] += 0.5;
        follow(far, held, nature, eye, 1.7, 30);
        if (frame % 8 == 0) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    expect(far.levels.back().box.has_value(), "The coarsest level must stay whole under a moving eye");
    for (int frame = 0; frame < 120; ++frame) {
        eye[0] += 30;
        follow(far, held, nature, {eye[0], eye[1] + 3000, eye[2]}, 3000, 1800);
    }
    expect(!far.levels.front().active && far.levels.front().slots.empty() && far.levels.back().active, "Levels too fine for a fast, high eye must rest");
    expect(settle(far, held, nature, eye, 1.7, 300, whole), "The levels of far cubes never came back after the flight");
}

} // namespace

void testCubes(PlanetSession& session) {
    occlusionIsExact();
    chunksFit(session.game.nature.world, session.game.spawn);
    levelsFollowTheEye(session.game.nature, {session.game.spawn[0], session.game.spawn[1] + 1.7, session.game.spawn[2]});
}
