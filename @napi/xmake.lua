set_xmakever("3.1.2")
set_project("derivean")
set_allowedplats("macosx", "windows")
set_allowedarchs("windows|x64")
set_languages("cxx20")
set_exceptions("cxx")
add_rules("mode.release", "mode.debug")
if is_plat("windows") then set_toolchains("clang-cl") end
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
    if is_plat("windows") then
        -- A Windows DLL resolves every import at link time, so Node-API comes from an import library
        -- built from node-api-headers' export list. It names node.exe; the import is delayed and
        -- src/windows/HostImports.cpp binds it to the executable that loads the addon.
        add_files("src/windows/*.cpp")
        add_shflags("/DELAYLOAD:node.exe", {force = true})
        add_syslinks("delayimp")
        on_config(function (target)
            target:add("linkdirs", target:autogendir())
            target:add("links", "node")
        end)
        before_link(function (target)
            local lib = path.join(target:autogendir(), "node.lib")
            local def = path.join(os.scriptdir(), "node_modules/node-api-headers/def/node_api.def")
            -- The toolchain's librarian: link.exe -lib by default; with LLVM's tools xmake picks
            -- llvm-ar, which takes no .def, so its sibling llvm-lib is used.
            local program, name = target:tool("ar")
            local argv = {"/nologo", "/machine:x64", "/def:" .. def, "/out:" .. lib}
            if name == "link" then table.insert(argv, 1, "-lib") end
            if name == "llvm_ar" or name == "ar" then program = program:gsub("llvm%-ar", "llvm-lib") end
            local envs = {}
            for _, toolchain in ipairs(target:toolchains()) do envs = os.joinenvs(envs, toolchain:runenvs()) end
            os.mkdir(path.directory(lib))
            vprint("import library %s (%s)", program, tostring(name))
            os.vrunv(program, argv, {envs = envs})
        end)
    end
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
