-- The names of the folders and files the build, the release and the engine agree on, as the build writes them. The
-- engine writes the same names in C++, in src/aylo/core/layout/Layout.hpp: change both together (the release's smoke
-- test checks the layout end to end).

-- @return The names, as a table: `local layout = import("layout").names()`.
function names()
    return {
        -- A game.
        gameFolder = "game",
        pluginsFolder = "plugins",
        packFile = "game.pak",

        -- The runtime: what a game ships (`runtime/` in the release zip, the build folder before).
        runtimeFolder = "runtime",
        toolsFolder = "tools",
        runtimeProgram = "aylo-runtime",
        libFolder = "lib",
        backendsFolder = "backends",
        vulkanFolder = "vulkan",
        licensesFolder = "licenses",
        noticesFile = "THIRD_PARTY_NOTICES.md",
        noticesByLibraryFolder = "by-library",

        -- Next to a plugin library.
        linksExtension = ".links",
        signatureExtension = ".sig",
    }
end
