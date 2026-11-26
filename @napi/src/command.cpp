#include "convert.hpp"
#include "planet/constant/Scale.hpp"

#include <cmath>

namespace derivean {
namespace {

constexpr double MaxSeed = 9007199254740991.0;   // largest integer a JavaScript number holds exactly
constexpr double MaxAge = 1e11;                  // yr
constexpr double MaxHeight = 1e6;                // m
constexpr double MaxTimeJump = 1000.0 * 365.0 * 86400.0;   // s: a thousand years in one command

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

bool absent(Napi::Value value) { return value.IsUndefined() || value.IsNull(); }

std::optional<Position> readTeleport(Napi::Value value) {
    if (value.IsNull()) return std::nullopt;
    if (!value.IsArray()) throw Napi::TypeError::New(value.Env(), "Expected a teleport position");
    const auto point = value.As<Napi::Array>();
    return Position{readNumber(point.Get(uint32_t(0)), -sim::PlanetSpan, sim::PlanetSpan),
                    readNumber(point.Get(uint32_t(1)), -MaxHeight, MaxHeight),
                    readNumber(point.Get(uint32_t(2)), -sim::PlanetSpan, sim::PlanetSpan)};
}

} // namespace

double readPlace(Napi::Value value) { return readNumber(value, -sim::PlanetSpan, sim::PlanetSpan); }

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
    // A client that does not steer the clock gets real time.
    const Napi::Value time = input.Get("time");
    if (!time.IsUndefined() && !time.IsObject()) throw Napi::TypeError::New(info.Env(), "Expected a calendar rate and jump");
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
        .fly = readFlag(input.Get("fly")),
        .boost = readFlag(input.Get("boost")),
        .ascend = float(readNumber(input.Get("ascend"), -1.0, 1.0)),
        .teleport = readTeleport(input.Get("teleport")),
        .timeRate = time.IsUndefined() ? 1.0 : readNumber(time.As<Napi::Object>().Get("rate"), 0.0, MaxTimeRate),
        .timeJump = time.IsUndefined() ? 0.0 : readNumber(time.As<Napi::Object>().Get("jump"), 0.0, MaxTimeJump),
    };
}

PlanetOptions parseWorld(Napi::Value value) {
    PlanetOptions options;
    if (absent(value)) return options;
    if (!value.IsObject()) throw Napi::TypeError::New(value.Env(), "Expected world options");
    const auto input = value.As<Napi::Object>();
    options.seed = uint64_t(readNumber(input.Get("seed"), 0.0, MaxSeed));
    options.age = readNumber(input.Get("age"), 0.0, MaxAge);
    if (const Napi::Value spawn = input.Get("spawn"); !absent(spawn)) {
        if (!spawn.IsArray()) throw Napi::TypeError::New(value.Env(), "Expected a spawn position");
        const auto point = spawn.As<Napi::Array>();
        options.spawn = {readNumber(point.Get(uint32_t(0)), -sim::PlanetSpan, sim::PlanetSpan),
                         readNumber(point.Get(uint32_t(1)), -sim::PlanetSpan, sim::PlanetSpan)};
    }
    if (const Napi::Value reach = input.Get("cubeReach"); !absent(reach)) options.cubeReach = int(readNumber(reach, 64, 256));
    return options;
}

} // namespace derivean
