#pragma once

#include "game/type/Game.hpp"

#include <napi.h>

#include <cstring>

namespace derivean {

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

template<class T>
Napi::TypedArrayOf<T> packed(Napi::Env env, const std::vector<T>& values) {
    auto array = Napi::TypedArrayOf<T>::New(env, values.size());
    if (!values.empty()) std::memcpy(array.Data(), values.data(), values.size() * sizeof(T));
    return array;
}

template<class T>
Napi::Value optional(Napi::Env env, const std::optional<T>& value) {
    return value ? Napi::Number::New(env, double(*value)).template As<Napi::Value>() : env.Null();
}

Napi::Object renderFrame(Napi::Env env, const Frame& frame);
// Faces of a chunk as two words per quad, laid out as GameBridge.ts says. Only a chunk's faces fit: no coordinate above 32.
Napi::Value quads(Napi::Env env, const FaceMesh& faces);
Napi::Object farCubes(Napi::Env env, const Frame& frame);
Command parseCommand(const Napi::CallbackInfo& info);
PlanetOptions parseWorld(Napi::Value value);   // the default planet when no options are given
double readPlace(Napi::Value value);           // a horizontal world coordinate in metres

} // namespace derivean
