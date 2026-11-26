#include "convert.hpp"

namespace derivean {

Napi::Value quads(Napi::Env env, const FaceMesh& faces) {
    const size_t count = faces.size() / FaceWords;
    auto array = Napi::Uint32Array::New(env, count * 2);
    for (size_t face = 0; face < count; ++face) {
        const int16_t* word = faces.data() + face * FaceWords;
        array[face * 2] = uint32_t(word[2]) | uint32_t(word[3]) << 6 | uint32_t(word[4]) << 12 | uint32_t(word[5] - 1) << 18 |
                          uint32_t(word[6] - 1) << 23 | uint32_t(word[0]) << 28 | uint32_t(word[1] > 0) << 30;
        array[face * 2 + 1] = uint32_t(word[7]) | uint32_t(word[8]) << 8;
    }
    return array;
}

namespace {

Napi::Value levelBox(Napi::Env env, const std::optional<LevelBox>& value) {
    if (!value) return env.Null();
    auto box = Napi::Object::New(env);
    box["min"] = numbers(env, value->min);
    box["max"] = numbers(env, value->max);
    return box;
}

Napi::Object cubeChunk(Napi::Env env, const CubeChunk& value) {
    auto chunk = Napi::Object::New(env);
    const double edge = edgeOf(value.level), size = CubeSide * edge;
    chunk["id"] = value.id;
    chunk["level"] = value.level;
    chunk["edge"] = edge;
    chunk["origin"] = numbers(env, Position{value.at.x * size, value.at.y * size, value.at.z * size});
    chunk["quads"] = quads(env, value.faces);
    chunk["water"] = project(env, value.water, [](Napi::Env env, const WaterSheet& value) {
        auto sheet = Napi::Object::New(env);
        sheet["level"] = value.level;
        sheet["quads"] = quads(env, value.faces);
        return sheet;
    });
    chunk["dataLevel"] = value.dataLevel;
    chunk["wantedLevel"] = uint8_t(cubeSource(value.level));
    return chunk;
}

} // namespace

Napi::Object farCubes(Napi::Env env, const Frame& frame) {
    auto cubes = Napi::Object::New(env);
    cubes["added"] = project(env, frame.cubes.added, cubeChunk);
    cubes["removed"] = numbers(env, frame.cubes.removed);
    cubes["data"] = numbers(env, frame.farData);
    cubes["levels"] = project(env, frame.cubeLevels, [](Napi::Env env, const CubeLevelView& value) {
        auto level = Napi::Object::New(env);
        level["level"] = value.level;
        level["edge"] = edgeOf(value.level);
        level["wanted"] = double(value.wanted);
        level["shown"] = double(value.shown);
        level["settled"] = double(value.settled);
        level["box"] = levelBox(env, value.box);
        return level;
    });
    return cubes;
}

} // namespace derivean
