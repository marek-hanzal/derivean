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

-- Polygon union of crust footprints in the planet's boundary band (docs/credits/libraries.md).
add_requires("clipper2 1.5.4")

-- Correctly rounded maths (CORE-MATH, docs/credits/libraries.md) behind core/fn/Exact.hpp: one
-- result for every argument on every platform, where the platform libm differs by a few ulp. Taken
-- from git at a fixed commit, whose hash pins the source. Only its `cr_` names exist, so no compiler
-- folds or merges them as builtins. The flags written below are not part of the package's hash:
-- a change to them needs a new version here, or both machines keep the old build. Should CORE-MATH
-- ever meet an argument its proof missed, pow prints it and exits the process (upstream's choice).
package("core-math")
    set_homepage("https://core-math.gitlabpages.inria.fr/")
    set_description("Correctly rounded mathematical functions")
    set_license("MIT")
    set_urls("https://gitlab.inria.fr/core-math/core-math.git")
    add_versions("2026.11.27", "b464d7d0280f4c5091b8d2fff80b6cb03ab1d37f")
    on_install("macosx", "windows|x64", function (package)
        local functions = {"sin", "cos", "sincos", "tan", "asin", "acos", "atan", "atan2", "sinh", "cosh", "tanh", "atanh",
            "exp", "exp2", "exp10", "expm1", "log", "log2", "log10", "log1p", "pow", "cbrt", "hypot", "erf", "erfc"}
        local files = {}
        for _, name in ipairs(functions) do
            table.insert(files, format("%q", "src/binary64/" .. name .. "/" .. name .. ".c"))
            table.insert(files, format("%q", "src/binary32/" .. name .. "/" .. name .. "f.c"))
        end
        io.writefile("xmake.lua", [[
add_rules("mode.release")
target("core-math")
    set_kind("static")
    set_warnings("none")
    add_files(]] .. table.concat(files, ", ") .. [[)
    -- Correct rounding gives the same result with fusion or without; upstream's README asks for
    -- contraction off or on, never fast. Clang on both platforms, and on x86-64 the FMA3 of AVX2.
    if is_plat("macosx") then add_cflags("-O3", "-ffp-contract=on", {force = true}) end
    if is_plat("windows") then
        set_toolchains("clang-cl")
        add_cflags("/O2", "/arch:AVX2", "/clang:-O3", "/clang:-ffp-contract=on", {force = true})
    end
]])
        import("package.tools.xmake").install(package)
    end)
    on_test(function (package)
        assert(package:check_csnippets({test = "double cr_pow(double, double); void test(void) { volatile double r = cr_pow(2.0, 0.5); (void)r; }"}))
    end)
package_end()
add_requires("core-math 2026.11.27")

target("derivean-simulation")
    set_kind("static")
    add_files("src/**.cpp")
    add_includedirs("src", {public = true})
    add_packages("clipper2", "core-math", {public = true})
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
