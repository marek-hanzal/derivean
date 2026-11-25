#include "simulation/type/Simulation.hpp"

#include <napi.h>

#include <cmath>
#include <cstring>
#include <optional>

namespace derivean {
namespace {

template<class Range>
Napi::Array numbers(Napi::Env env, const Range& range) {
    auto array = Napi::Array::New(env, range.size());
    uint32_t index = 0;
    for (auto value : range) array[index++] = Napi::Number::New(env, double(value));
    return array;
}

template<class Range, class Project>
Napi::Array project(Napi::Env env, const Range& range, Project convert) {
    auto array = Napi::Array::New(env, range.size());
    uint32_t index = 0;
    for (const auto& value : range) array[index++] = convert(env, value);
    return array;
}

Napi::Array coordinates(Napi::Env env, Cell cell) {
    return numbers(env, std::array{cell.x, cell.y, cell.z});
}

template<class T>
Napi::TypedArrayOf<T> packed(Napi::Env env, const std::vector<T>& values) {
    auto array = Napi::TypedArrayOf<T>::New(env, values.size());
    if (!values.empty()) std::memcpy(array.Data(), values.data(), values.size() * sizeof(T));
    return array;
}

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
    const auto chunkMesh = [](Napi::Env env, const ChunkMesh& value) {
        auto chunk = Napi::Object::New(env);
        chunk["key"] = coordinates(env, value.key);
        chunk["origin"] = numbers(env, value.origin);
        chunk["faces"] = packed(env, value.faces);
        return chunk;
    };
    output["chunks"] = project(env, frame.chunks, chunkMesh);
    output["waterMeshes"] = project(env, frame.waterMeshes, chunkMesh);
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
    output["waterDepth"] = frame.waterDepth ? Napi::Number::New(env, *frame.waterDepth).As<Napi::Value>() : env.Null();
    return output;
}

double readNumber(Napi::Value value, double min, double max) {
    if (!value.IsNumber()) throw Napi::TypeError::New(value.Env(), "Expected a numeric input");
    const double number = value.As<Napi::Number>().DoubleValue();
    if (!std::isfinite(number) || number < min || number > max)
        throw Napi::RangeError::New(value.Env(), "Engine input out of range");
    return number;
}

bool readFlag(Napi::Value value) {
    if (!value.IsBoolean()) throw Napi::TypeError::New(value.Env(), "Expected a boolean input");
    return value.As<Napi::Boolean>().Value();
}

Command parseCommand(const Napi::CallbackInfo& info) {
    if (!info[0].IsObject()) throw Napi::TypeError::New(info.Env(), "Missing engine input");
    const auto input = info[0].As<Napi::Object>();
    const Napi::Value aim = input.Get("direction");
    if (!aim.IsArray()) throw Napi::TypeError::New(info.Env(), "Missing direction vector");
    const auto direction = aim.As<Napi::Array>();
    const Napi::Value shape = input.Get("shape");
    if (!shape.IsString()) throw Napi::TypeError::New(info.Env(), "Missing brush shape");
    const auto name = shape.As<Napi::String>().Utf8Value();
    if (name != "sphere" && name != "cube") throw Napi::TypeError::New(info.Env(), "Unknown brush shape");
    return {
        .elapsed = readNumber(input.Get("elapsed"), 0.0, 0.1),
        .x = float(readNumber(input.Get("x"), -1.0, 1.0)),
        .z = float(readNumber(input.Get("z"), -1.0, 1.0)),
        .jump = readFlag(input.Get("jump")),
        .sprint = readFlag(input.Get("sprint")),
        .active = readFlag(input.Get("active")),
        .direction = {readNumber(direction.Get(uint32_t(0)), -1.0, 1.0),
                      readNumber(direction.Get(uint32_t(1)), -1.0, 1.0),
                      readNumber(direction.Get(uint32_t(2)), -1.0, 1.0)},
        .radius = int(readNumber(input.Get("radius"), 1, MaxBrushRadius)),
        .shape = name == "sphere" ? BrushShape::Sphere : BrushShape::Cube,
        .dig = readFlag(input.Get("dig")),
        .place = readFlag(input.Get("place")),
    };
}

class Session final : public Napi::ObjectWrap<Session> {
public:
    explicit Session(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Session>(info), game_(std::in_place) {}
    static Napi::Function define(Napi::Env env) {
        return DefineClass(env, "Session", {
            InstanceMethod("advance", &Session::frame),
            InstanceMethod("restart", &Session::restart),
        });
    }

private:
    std::optional<Simulation> game_;

    void restart(const Napi::CallbackInfo&) {
        game_.emplace();
    }

    Napi::Value frame(const Napi::CallbackInfo& info) {
        try {
            return renderFrame(info.Env(), advance(*game_, parseCommand(info)));
        } catch (const Napi::Error&) {
            throw;
        } catch (const std::exception& error) {
            throw Napi::Error::New(info.Env(), error.what());
        }
    }
};

Napi::Object initialize(Napi::Env env, Napi::Object exports) {
    exports["Session"] = Session::define(env);
    exports["world"] = Napi::Object::New(env);
    auto world = exports.Get("world").As<Napi::Object>();
    world["voxelSize"] = VoxelSize;
    world["chunkMeters"] = ChunkSide * VoxelSize;
    world["defaultBrushRadius"] = DefaultBrushRadius;
    world["maxBrushRadius"] = MaxBrushRadius;
    world["waterUnitsPerVoxel"] = WaterUnits;
    world["materials"] = project(env, Materials, [](Napi::Env env, const MaterialInfo& value) {
        auto material = Napi::Object::New(env);
        material["name"] = Napi::String::New(env, value.name.data(), value.name.size());
        material["color"] = value.color;
        material["roughness"] = value.roughness;
        material["density"] = value.density;
        material["support"] = value.support;
        constexpr std::array phases{"gas", "solid", "granular", "liquid"};
        material["matter"] = Napi::String::New(env, phases[size_t(value.matter)]);
        material["friction"] = value.friction;
        material["angleOfRepose"] = value.angleOfRepose;
        material["dynamicViscosity"] = value.dynamicViscosity;
        material["surfaceTension"] = value.surfaceTension;
        return material;
    });
    return exports;
}

} // namespace
} // namespace derivean

Napi::Object init(Napi::Env env, Napi::Object exports) { return derivean::initialize(env, exports); }
NODE_API_MODULE(derivean, init)
