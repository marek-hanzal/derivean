#include "chart.hpp"

#include "chart/fn/Chart.hpp"
#include "planet/constant/Scale.hpp"

#include <array>
#include <cmath>
#include <cstring>
#include <string>

namespace derivean {
namespace {

constexpr double MaxPosition = 1e12;                // m; site tile indices still fit 32 bits there, and the chart reads no ground tile so far out
constexpr double MaxSpan = 64 * sim::PlanetSpan;    // m
constexpr double MinSpan = 1.0;                     // m
constexpr int MaxSize = 1024;                       // pixels per side
// Rasters in the order they lie in the chart's one buffer: the 32-bit ones first, which keeps them aligned.
constexpr std::array FloatRasters{"ground", "water", "discharge", "ice", "warmth", "cover"};
constexpr std::array ByteRasters{"bedrock", "waters", "level"};

double readNumber(Napi::Object input, const char* name, double min, double max) {
    const Napi::Value value = input.Get(name);
    if (!value.IsNumber()) throw Napi::TypeError::New(input.Env(), std::string("Chart request needs a numeric ") + name);
    const double number = value.As<Napi::Number>().DoubleValue();
    if (!std::isfinite(number) || number < min || number > max)
        throw Napi::RangeError::New(input.Env(), std::string("Chart request ") + name + " is out of range");
    return number;
}

void leave(ChartGate::Readers& readers) {
    std::lock_guard lock(readers.mutex);
    --readers.count;
    readers.idle.notify_all();
}

struct Leaving {
    std::shared_ptr<ChartGate::Readers> readers;
    ~Leaving() { leave(*readers); }
};

// One chart: the result object and its buffer are made on the session thread, filled on a pool
// thread, and handed to the promise back on the session thread, which never reads or copies a raster.
class ChartWork final : public Napi::AsyncWorker {
public:
    ChartWork(Napi::Env env, const sim::World& world, sim::ChartRequest request, std::shared_ptr<ChartGate::Readers> readers)
        : Napi::AsyncWorker(env), deferred_(Napi::Promise::Deferred::New(env)), world_(world), request_(request), readers_(std::move(readers)) {
        const size_t pixels = size_t(request.size) * size_t(request.size);
        auto buffer = Napi::ArrayBuffer::New(env, pixels * (FloatRasters.size() * sizeof(float) + ByteRasters.size()));
        auto output = Napi::Object::New(env);
        auto origin = Napi::Array::New(env, 2);
        origin[uint32_t(0)] = request.centre.x - 0.5 * request.span;
        origin[uint32_t(1)] = request.centre.z - 0.5 * request.span;
        output["origin"] = origin;
        output["pitch"] = request.span / request.size;
        output["size"] = request.size;
        size_t offset = 0;
        for (const char* name : FloatRasters) {
            output[name] = Napi::Float32Array::New(env, pixels, buffer, offset);
            offset += pixels * sizeof(float);
        }
        for (const char* name : ByteRasters) {
            output[name] = Napi::Uint8Array::New(env, pixels, buffer, offset);
            offset += pixels;
        }
        memory_ = static_cast<uint8_t*>(buffer.Data());
        output_ = Napi::Persistent(output);
        std::lock_guard lock(readers_->mutex);
        ++readers_->count;
    }

    // Work that was never run still has to leave.
    ~ChartWork() override {
        if (readers_) leave(*readers_);
    }

    Napi::Promise promise() const { return deferred_.Promise(); }

protected:
    // On a pool thread. The buffer belongs to an object nobody else holds until the promise resolves.
    void Execute() override {
        const Leaving leaving{std::move(readers_)};
        const sim::Chart chart = sim::chartOf(world_, request_);
        tiles_ = chart.tiles;
        finest_ = chart.finest;
        uint8_t* to = memory_;
        for (const std::vector<float>* raster : {&chart.ground, &chart.water, &chart.discharge, &chart.ice, &chart.warmth, &chart.cover}) {
            std::memcpy(to, raster->data(), raster->size() * sizeof(float));
            to += raster->size() * sizeof(float);
        }
        for (const std::vector<uint8_t>* raster : {&chart.bedrock, &chart.waters, &chart.level}) {
            std::memcpy(to, raster->data(), raster->size());
            to += raster->size();
        }
    }

    // A session thread that is terminated with a chart in flight still gets here, in an environment
    // that no longer runs JavaScript. Node-API then refuses to settle the promise, and nobody is
    // left to hear it; an exception let out of here would end the process.
    void OnOK() override try {
        Napi::Object output = output_.Value();
        output["tiles"] = double(tiles_);
        output["finest"] = finest_;
        deferred_.Resolve(output);
    } catch (const Napi::Error&) {
    }

    void OnError(const Napi::Error& error) override try {
        deferred_.Reject(error.Value());
    } catch (const Napi::Error&) {
    }

private:
    Napi::Promise::Deferred deferred_;
    Napi::ObjectReference output_;
    const sim::World& world_;
    sim::ChartRequest request_;
    std::shared_ptr<ChartGate::Readers> readers_;
    uint8_t* memory_{};
    size_t tiles_{};
    uint8_t finest_{};
};

} // namespace

ChartGate::ChartGate() : readers_(std::make_shared<Readers>()) {}

ChartGate::~ChartGate() { wait(); }

void ChartGate::wait() {
    std::unique_lock lock(readers_->mutex);
    readers_->idle.wait(lock, [&] { return readers_->count == 0; });
}

Napi::Value requestChart(const Napi::CallbackInfo& info, const sim::World& world, ChartGate& gate) try {
    const Napi::Env env = info.Env();
    if (!info[0].IsObject()) throw Napi::TypeError::New(env, "Missing chart request");
    const auto input = info[0].As<Napi::Object>();
    const sim::ChartRequest request{{readNumber(input, "x", -MaxPosition, MaxPosition), readNumber(input, "z", -MaxPosition, MaxPosition)},
                                    readNumber(input, "span", MinSpan, MaxSpan), int(std::floor(readNumber(input, "size", 1, MaxSize)))};

    // Nothing has been built since the caller's chart: there is no sharper one to read.
    if (const Napi::Value unless = input.Get("unless"); unless.IsNumber() && unless.As<Napi::Number>().DoubleValue() == double(world.atlas.finished())) {
        auto same = Napi::Promise::Deferred::New(env);
        same.Resolve(env.Null());
        return same.Promise();
    }

    auto work = std::make_unique<ChartWork>(env, world, request, gate.readers());
    const Napi::Promise promise = work->promise();
    work->Queue();
    work.release();   // queued work deletes itself once it has settled its promise
    return promise;
} catch (const Napi::Error&) {
    throw;
} catch (const std::exception& error) {
    throw Napi::Error::New(info.Env(), error.what());
}

} // namespace derivean
