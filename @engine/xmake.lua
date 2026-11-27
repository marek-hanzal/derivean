local jolt = {double_precision = true, object_stream = false}
if is_plat("windows") then
    -- The engine compiles Jolt's headers with the simulation's /arch:AVX2, and Jolt passes DVec3 by
    -- value only with AVX, so the library is built for the same instruction set.
    for _, set in ipairs({"sse4_1", "sse4_2", "avx", "avx2", "f16c", "fmadd", "lzcnt", "tzcnt"}) do jolt["inst_" .. set] = true end
end
add_requires("joltphysics 5.7.0", {configs = jolt})

target("derivean-engine")
    set_kind("static")
    add_files("src/**.cpp")
    add_includedirs("src", {public = true})
    add_deps("derivean-simulation")
    add_packages("joltphysics", {public = true})

target("engine-tests")
    set_kind("binary")
    set_default(false)
    add_files("test/**.cpp")
    add_deps("derivean-engine")

target("engine-bench")
    set_kind("binary")
    set_default(false)
    add_files("bench/**.cpp")
    add_deps("derivean-engine")
