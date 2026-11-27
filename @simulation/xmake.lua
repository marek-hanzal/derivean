-- Builds alone through this module's mise tasks and as a dependency of the engine through @napi.
if os.scriptdir() == os.projectdir() then
    set_xmakever("3.1.2")
    set_project("derivean-simulation")
    set_allowedplats("macosx", "windows")
    set_allowedarchs("windows|x64")
    set_languages("cxx20")
    set_exceptions("cxx")
    add_rules("mode.release", "mode.debug")
    if is_plat("windows") then set_toolchains("clang-cl") end
end

target("derivean-simulation")
    set_kind("static")
    add_files("src/**.cpp")
    add_includedirs("src", {public = true})
    -- Fused multiply-adds round alike on macOS and Windows (docs/windows.md#determinism; flags per
    -- docs/credits/libraries.md, platform documentation): clang on both, fusing a * b + c inside one
    -- expression and nowhere else, and on x86-64 with the FMA3 of AVX2 so that every fusion arm64
    -- makes is made there too. The platform libm still differs. Forced, because xmake drops a flag
    -- its check rejects, and public, so that the engine, the addon and the tests compile the
    -- simulation's inline code the same way.
    if is_plat("macosx") then add_cxflags("-ffp-contract=on", {public = true, force = true}) end
    if is_plat("windows") then
        add_cxflags("/clang:-ffp-contract=on", "/arch:AVX2", {public = true, force = true})
        -- M_PI and friends, <windows.h> without min/max macros, and the standard C library without
        -- the deprecation warnings that only the Microsoft runtime raises.
        add_defines("_USE_MATH_DEFINES", "NOMINMAX", "WIN32_LEAN_AND_MEAN", "_CRT_SECURE_NO_WARNINGS", {public = true})
        on_config(function (target)
            local _, compiler = target:tool("cxx")
            if compiler ~= "clang_cl" then
                raise("derivean-simulation is compiled with clang-cl on Windows, not " .. tostring(compiler) .. ": MSVC fuses and rounds differently from clang on macOS")
            end
        end)
    end

target("simulation-tests")
    set_kind("binary")
    set_default(false)
    add_files("test/**.cpp")
    add_deps("derivean-simulation")

-- Diagnostic command line: maps, probes and measurements straight from the simulation API.
target("terra")
    set_kind("binary")
    set_default(false)
    add_files("tool/**.cpp")
    add_deps("derivean-simulation")
