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
    -- A thread that is terminated mid-call (a worker at app quit) can no longer take an exception: without
    -- the swallow, node-addon-api throws again out of its own catch and the process aborts.
    add_defines("NAPI_VERSION=8", "NAPI_CPP_EXCEPTIONS", "NODE_API_SWALLOW_UNTHROWABLE_EXCEPTIONS")
    -- Stamps the addon with the tree it was linked from, next to it as derivean.node.json, on every
    -- link: the build time, the HEAD commit, whether the sources it links (@simulation, @engine,
    -- @napi) differ from it, and a SHA-256 over their diff and their untracked files. From the root:
    --   { git diff HEAD --binary --no-ext-diff --no-textconv --no-color --no-renames --src-prefix=a/ --dst-prefix=b/ -- @simulation @engine @napi
    --     git -c core.quotePath=false ls-files --others --exclude-standard -- @simulation @engine @napi | while IFS= read -r f; do printf '\0untracked %s\0' "$f"; cat "$f"; done
    --   } | shasum -a 256
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
        local diff = git("diff", "HEAD", "--binary", "--no-ext-diff", "--no-textconv", "--no-color", "--no-renames", "--src-prefix=a/", "--dst-prefix=b/",
                         "--", table.unpack(sources))
        local changed = diff or ""
        for _, file in ipairs((git("-c", "core.quotePath=false", "ls-files", "--others", "--exclude-standard", "--", table.unpack(sources)) or ""):split("\n", {plain = true})) do
            if #file > 0 and os.isfile(path.join(root, file)) then
                changed = changed .. "\0untracked " .. file .. "\0" .. io.readfile(path.join(root, file), {encoding = "binary"})
            end
        end
        -- Without git (or a commit) nothing is known about the tree, which is not the same as clean.
        local known = commit ~= nil and diff ~= nil
        local dirty = json.null
        if known then dirty = #changed > 0 end
        local digest = json.null
        if known and #changed > 0 then
            local scratch = stamp .. ".diff"
            io.writefile(scratch, changed, {encoding = "binary"})
            digest = hash.sha256(scratch)
            os.rm(scratch)
        end
        io.writefile(stamp, json.encode({builtAt = os.date("!%Y-%m-%dT%H:%M:%SZ"), commit = commit and commit:trim() or json.null,
                                         dirty = dirty, diffHash = digest, sources = sources}) .. "\n")
    end)
