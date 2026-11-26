#include "convert.hpp"

namespace derivean {
namespace {

Napi::Array coordinates(Napi::Env env, Cell cell) {
    return numbers(env, std::array{cell.x, cell.y, cell.z});
}

Napi::Object chunkMesh(Napi::Env env, const ChunkMesh& value) {
    auto chunk = Napi::Object::New(env);
    chunk["key"] = coordinates(env, value.key);
    chunk["origin"] = numbers(env, value.origin);
    return chunk;
}

Napi::Value clouds(Napi::Env env, const Frame& frame) {
    if (!frame.clouds) return env.Null();
    const sim::CloudField& field = *frame.clouds;
    auto output = Napi::Object::New(env);
    output["origin"] = numbers(env, std::array{0.0, 0.0});
    output["cell"] = field.cell;
    output["cells"] = field.size;
    output["time"] = frame.cloudsTime;
    output["bands"] = project(env, field.band, [](Napi::Env env, const sim::CloudField::BandChannels& value) {
        auto band = Napi::Object::New(env);
        band["cover"] = packed(env, value.cover);
        band["base"] = packed(env, value.base);
        band["top"] = packed(env, value.top);
        band["depth"] = packed(env, value.opticalDepth);
        band["ice"] = packed(env, value.iceShare);
        band["windX"] = packed(env, value.windX);
        band["windZ"] = packed(env, value.windZ);
        band["genus"] = packed(env, value.genus);
        return band;
    });
    output["heaping"] = packed(env, field.heaping);
    output["rain"] = packed(env, field.rain);
    output["snow"] = packed(env, field.snow);
    const auto& lying = frame.cloudsAir->snow.cells;
    output["snowpack"] = packed(env, std::vector<float>(lying.begin(), lying.end()));
    output["snowLadder"] = packed(env, field.snowLadder);
    output["snowLine"] = packed(env, field.snowLine);
    output["seaLevel"] = field.seaLevel;
    output["aerosolDepth"] = packed(env, field.aerosolDepth);
    output["hazeDepth"] = packed(env, field.hazeDepth);
    output["hazeTop"] = packed(env, field.hazeTop);
    output["hazeThickness"] = field.hazeThickness;
    output["ashAloft"] = packed(env, field.ashAloft);
    output["ashLevel"] = packed(env, field.ashLevel);
    return output;
}

void sky(Napi::Env env, const Frame& frame, Napi::Object output) {
    auto calendar = Napi::Object::New(env);
    calendar["era"] = frame.calendar.era;
    calendar["seconds"] = frame.calendar.seconds;
    calendar["dayOfYear"] = frame.dayOfYear;
    calendar["hour"] = frame.hour;
    output["calendar"] = calendar;
    output["sun"] = numbers(env, frame.sun);
    auto weather = Napi::Object::New(env);
    weather["temperature"] = frame.weather.temperature;
    weather["rain"] = frame.weather.rain;
    weather["snow"] = frame.weather.snow;
    weather["wind"] = numbers(env, std::array{frame.weather.windX, frame.weather.windZ});
    weather["cloudCover"] = frame.weather.cloudCover;
    weather["humidity"] = frame.weather.humidity;
    weather["pressure"] = frame.weather.pressure;
    weather["snowpack"] = frame.weather.snowpack;
    weather["lag"] = frame.weather.lag;
    output["weather"] = weather;
    output["clouds"] = clouds(env, frame);
}

Napi::Object natural(Napi::Env env, const Frame& frame, Napi::Object output) {
    sky(env, frame, output);
    output["flying"] = frame.flying;
    // Always empty; kept because the renderer's capture stall rule reads it.
    auto far = Napi::Object::New(env);
    far["added"] = Napi::Array::New(env);
    far["removed"] = Napi::Array::New(env);
    output["far"] = far;
    output["cubes"] = farCubes(env, frame);
    if (frame.voxelBox) {
        auto box = Napi::Object::New(env);
        box["min"] = numbers(env, frame.voxelBox->min);
        box["max"] = numbers(env, frame.voxelBox->max);
        output["voxelBox"] = box;
    } else {
        output["voxelBox"] = env.Null();
    }
    output["ground"] = optional(env, frame.ground);
    output["waterLevel"] = optional(env, frame.waterLevel);
    auto ready = Napi::Object::New(env);
    ready["site"] = frame.siteReady;
    ready["local"] = frame.localReady;
    ready["region"] = frame.regionReady;
    ready["far"] = frame.farReady;
    output["ready"] = ready;
    auto tiles = Napi::Object::New(env);
    constexpr std::array names{"region", "local", "site", "ground"};
    for (size_t level = 1; level < frame.tilesWanted.size(); ++level) {
        auto progress = Napi::Object::New(env);
        progress["wanted"] = frame.tilesWanted[level];
        progress["finished"] = frame.tilesFinished[level];
        tiles[names[level - 1]] = progress;
    }
    output["tiles"] = tiles;
    auto pending = Napi::Object::New(env);
    pending["chunks"] = double(frame.pendingChunks);
    pending["patches"] = double(frame.pendingCubes);
    output["pending"] = pending;
    output["generationMs"] = frame.generationMs;
    output["streamingMs"] = frame.streamingMs;
    return output;
}

} // namespace

Napi::Object renderFrame(Napi::Env env, const Frame& frame) {
    auto output = Napi::Object::New(env);
    output["eye"] = numbers(env, frame.eye);
    if (frame.target) {
        auto target = Napi::Object::New(env);
        target["voxel"] = coordinates(env, frame.target->voxel);
        target["material"] = uint8_t(frame.target->material);
        output["target"] = target;
    } else {
        output["target"] = env.Null();
    }
    const auto mesh = [](Napi::Env env, const ChunkMesh& value) {
        auto chunk = chunkMesh(env, value);
        chunk["faces"] = packed(env, value.faces);
        chunk["quads"] = env.Null();
        return chunk;
    };
    // The ground comes as packed quads, like the far chunks.
    output["chunks"] = project(env, frame.chunks, [](Napi::Env env, const ChunkMesh& value) {
        auto chunk = chunkMesh(env, value);
        chunk["faces"] = Napi::Int16Array::New(env, 0);
        chunk["quads"] = quads(env, value.faces);
        return chunk;
    });
    output["waterMeshes"] = project(env, frame.waterMeshes, mesh);
    output["unloaded"] = project(env, frame.unloaded, coordinates);
    output["spawned"] = project(env, frame.spawned, [](Napi::Env env, const DebrisMesh& value) {
        auto body = Napi::Object::New(env);
        body["id"] = value.id;
        body["faces"] = packed(env, value.faces);
        return body;
    });
    output["debris"] = project(env, frame.debris, [](Napi::Env env, const DebrisPose& value) {
        auto body = Napi::Object::New(env);
        body["id"] = value.id;
        body["position"] = numbers(env, value.pose.position);
        body["rotation"] = numbers(env, value.pose.rotation);
        return body;
    });
    output["editedTiles"] = double(frame.editedTiles);
    output["visibleChunks"] = double(frame.visibleChunks);
    output["bodies"] = double(frame.bodies);
    output["nativeMs"] = frame.nativeMs;
    output["particles"] = packed(env, frame.particles);
    output["particleOrigin"] = numbers(env, frame.particleOrigin);
    output["retired"] = numbers(env, frame.retired);
    output["streamingReady"] = frame.streamingReady;
    output["buildStock"] = frame.buildStock;
    output["sandAirborne"] = double(frame.sandAirborne);
    output["sandDeposited"] = double(frame.sandDeposited);
    output["sandEscaped"] = double(frame.sandEscaped);
    output["debrisEscapedKg"] = frame.debrisEscapedKg;
    output["excavatedKg"] = frame.excavatedKg;
    auto water = Napi::Object::New(env);
    water["initial"] = double(frame.water.initial);
    water["bulk"] = double(frame.water.bulk);
    water["airborne"] = double(frame.water.airborne);
    water["escaped"] = double(frame.water.escaped);
    output["water"] = water;
    output["waterDepth"] = optional(env, frame.waterDepth);
    return natural(env, frame, output);
}

} // namespace derivean
