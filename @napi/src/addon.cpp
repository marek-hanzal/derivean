#include "chart.hpp"
#include "convert.hpp"
#include "atmosphere/constant/Snow.hpp"
#include "planet/constant/Scale.hpp"
#include "threads/type/Threads.hpp"

namespace derivean {
namespace {

class Session final : public Napi::ObjectWrap<Session> {
public:
    explicit Session(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Session>(info) { start(info); }
    static Napi::Function define(Napi::Env env) {
        return DefineClass(env, "Session", {
            InstanceMethod("advance", &Session::frame),
            InstanceMethod("restart", &Session::restart),
            InstanceMethod("info", &Session::describe),
            InstanceMethod("inspect", &Session::inspect),
            InstanceMethod("chart", &Session::chart),
            InstanceMethod("abandon", &Session::abandon),
        });
    }

private:
    std::unique_ptr<Game> game_;
    PlanetOptions planet_;
    ChartGate charts_;   // declared after game_: destroyed first, which waits for charts still reading that world

    // A planet takes seconds to minutes to evolve and blocks the calling thread for that long.
    void start(const Napi::CallbackInfo& info) {
        try {
            planet_ = parseWorld(info[0]);
            favourCallingThread();   // this thread advances the game from now on
            charts_.wait();
            game_.reset();
            game_ = std::make_unique<Game>(planet_);
        } catch (const Napi::Error&) {
            throw;
        } catch (const std::exception& error) {
            throw Napi::Error::New(info.Env(), error.what());
        }
    }

    void restart(const Napi::CallbackInfo& info) { start(info); }

    // Lets go of the world without taking it down, for a process that is about to exit. Taking a
    // world down waits for every tile build in flight, which can be most of a minute; an exiting
    // process has no use for that. The world and its threads live on until the process ends, so
    // the addon must stay loaded until then.
    void abandon(const Napi::CallbackInfo&) {
        charts_.wait();
        (void)game_.release();
    }

    // A failed start leaves no world behind; every other call needs one.
    Game& game(Napi::Env env) {
        if (!game_) throw Napi::Error::New(env, "No world has been started");
        return *game_;
    }

    Napi::Value describe(const Napi::CallbackInfo& info) {
        const Game& world = game(info.Env());
        auto output = Napi::Object::New(info.Env());
        output["seed"] = double(planet_.seed);
        output["age"] = planet_.age;
        output["seaLevel"] = world.nature.world.planet.seaLevel;
        output["worldFloor"] = sim::WorldFloor;
        output["planetSpan"] = sim::PlanetSpan;
        output["faceWords"] = world.cubes ? FaceWords : FaceWords - 1;
        if (world.cubes) {
            auto cubes = Napi::Object::New(info.Env());
            cubes["reach"] = world.cubes->reach;
            cubes["side"] = CubeSide;
            cubes["band"] = CubeBand;
            cubes["finestLevel"] = FinestCubes;
            cubes["coarsestLevel"] = CoarsestCubes;
            cubes["viewDistance"] = CubeView;
            output["cubes"] = cubes;
        } else {
            output["cubes"] = info.Env().Null();
        }
        return output;
    }

    Napi::Value chart(const Napi::CallbackInfo& info) { return requestChart(info, game(info.Env()).nature.world, charts_); }

    Napi::Value inspect(const Napi::CallbackInfo& info) {
        const Napi::Env env = info.Env();
        try {
            auto output = Napi::Object::New(env);
            for (const auto& [key, value] : derivean::inspect(game(env), readPlace(info[0]), readPlace(info[1]))) {
                if (const double* number = std::get_if<double>(&value)) output.Set(key, *number);
                else output.Set(key, std::get<std::string>(value));
            }
            return output;
        } catch (const Napi::Error&) {
            throw;
        } catch (const std::exception& error) {
            throw Napi::Error::New(env, error.what());
        }
    }

    Napi::Value frame(const Napi::CallbackInfo& info) {
        try {
            return renderFrame(info.Env(), advance(game(info.Env()), parseCommand(info)));
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
    auto far = Napi::Object::New(env);
    far["cells"] = PatchCells;
    far["splitCells"] = SplitCells;
    far["morphStart"] = MorphStart;
    far["viewDistance"] = ViewDistance;
    world["far"] = far;
    world["snowLadder"] = numbers(env, sim::SnowLadder);
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
