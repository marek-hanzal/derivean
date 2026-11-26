set_xmakever("3.1.2")
set_project("derivean")
set_allowedplats("macosx", "linux")
set_languages("cxx20")
set_exceptions("cxx")
add_rules("mode.release", "mode.debug")
includes("../@simulation")
includes("../@engine")

target("derivean")
    add_rules("nodejs.module")
    set_targetdir("../.out/addon")
    add_deps("derivean-engine")
    add_files("src/*.cpp")
    add_includedirs("node_modules/node-addon-api", "node_modules/node-api-headers/include")
    add_defines("NAPI_VERSION=8", "NAPI_CPP_EXCEPTIONS")
    -- Stamps the addon with the tree it was linked from, next to it as derivean.node.json, on every
    -- link: the build time, the HEAD commit, whether the sources it links (@simulation, @engine,
    -- @napi) differ from it, and a SHA-256 over their diff and their untracked files.
    after_link(function (target)
        import("core.base.json")
        local stamp = target:targetfile() .. ".json"
        local root = path.directory(os.scriptdir())
        local sources = {"@simulation", "@engine", "@napi"}
        local function git(...)
            local args = table.join({"-C", root}, {...})
            return try { function () return os.iorunv("git", args) end }
        end
        local commit = git("rev-parse", "HEAD")
        local changed = git("diff", "HEAD", "--binary", "--", table.unpack(sources)) or ""
        for _, file in ipairs((git("ls-files", "--others", "--exclude-standard", "-z", "--", table.unpack(sources)) or ""):split("\0")) do
            if #file > 0 and os.isfile(path.join(root, file)) then
                changed = changed .. "\0untracked " .. file .. "\0" .. io.readfile(path.join(root, file), {encoding = "binary"})
            end
        end
        local digest = json.null
        if #changed > 0 then
            local scratch = stamp .. ".diff"
            io.writefile(scratch, changed, {encoding = "binary"})
            digest = hash.sha256(scratch)
            os.rm(scratch)
        end
        io.writefile(stamp, json.encode({builtAt = os.date("!%Y-%m-%dT%H:%M:%SZ"), commit = commit and commit:trim() or json.null,
                                         dirty = #changed > 0, diffHash = digest, sources = sources}) .. "\n")
    end)
