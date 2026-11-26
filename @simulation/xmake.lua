-- Builds alone through this module's mise tasks and as a dependency of the engine through @napi.
if os.scriptdir() == os.projectdir() then
    set_xmakever("3.1.2")
    set_project("derivean-simulation")
    set_allowedplats("macosx", "linux")
    set_languages("cxx20")
    set_exceptions("cxx")
    add_rules("mode.release", "mode.debug")
end

target("derivean-simulation")
    set_kind("static")
    add_files("src/**.cpp")
    add_includedirs("src", {public = true})
    if is_plat("linux") then
        add_syslinks("pthread", {public = true})
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
