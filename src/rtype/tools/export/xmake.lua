-- aylo-export: exports a game with only the plugins and backends it needs (docs/release/export.md).
target("aylo-export")
    set_kind("binary")
    set_default(false)
    add_deps("engine")
    -- It loads the libraries of the runtime it is given, built with aylo-runtime. No build-order dependency on them
    -- here: with one (add_deps(..., {inherit = false})), xmake dropped the `@loader_path` search path of every library
    -- of the build.
    add_files("main.cpp")
    -- The version written in export.json (rtype/build/BuildInfo.hpp).
    add_deps("build-info")
    -- In the release zip it is in `tools/`, next to `runtime/`, which holds the libraries it links. On Windows the
    -- packaging copies the DLLs next to it, as there is no rpath there.
    if not is_plat("windows") then
        add_rpathdirs("@loader_path/../runtime/lib", "@loader_path/../runtime/plugins")
    end
