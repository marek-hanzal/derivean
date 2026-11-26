add_requires("joltphysics 5.7.0", {configs = {
    double_precision = true,
    object_stream = false
}})

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
