-- The notices of the third-party code each binary of the runtime contains, for `xmake package-release`
-- (docs/release/export.md, EXP-02). A binary contains the packages of its target and those of the static modules it
-- links (`engine`, `luau`, `network`...). The release writes one file per binary in `runtime/licenses/by-library/`,
-- and the game export merges those of the binaries it keeps into the THIRD_PARTY_NOTICES.md it ships.
import("core.project.project")

-- One row of the notices per package, by its xmake name. A package a binary uses and this table does not know stops the
-- release: add it here. `with` names the rows of what the package brings in with it.
local packages = {
    asio = {name = "Asio", use = "networking", license = "BSL-1.0", project = "https://think-async.com"},
    glfw = {name = "GLFW", use = "windows and input", license = "Zlib", project = "https://www.glfw.org"},
    glm = {name = "GLM", use = "mathematics", license = "MIT", project = "https://github.com/g-truc/glm"},
    glslang = {name = "glslang", use = "shader compiler", license = "BSD-3-Clause, Apache-2.0",
               project = "https://github.com/KhronosGroup/glslang"},
    imgui = {name = "Dear ImGui", use = "the editor's interface", license = "MIT",
             project = "https://github.com/ocornut/imgui"},
    joltphysics = {name = "Jolt Physics", use = "physics", license = "MIT",
                   project = "https://github.com/jrouwe/JoltPhysics"},
    libcurl = {name = "curl", use = "HTTP requests", license = "curl",
               project = "https://curl.se", with = {"openssl"}},
    libsodium = {name = "libsodium", use = "signatures of the game plugins", license = "ISC",
                 project = "https://libsodium.org"},
    luau = {name = "Luau", use = "the scripting language", license = "MIT", project = "https://github.com/luau-lang/luau"},
    miniaudio = {name = "miniaudio", use = "audio", license = "MIT-0 or public domain", project = "https://miniaud.io"},
    nlohmann_json = {name = "nlohmann/json", use = "data files", license = "MIT",
                     project = "https://github.com/nlohmann/json"},
    raylib = {name = "raylib", use = "windows, drawing and sound (raylib backend)", license = "Zlib",
              project = "https://www.raylib.com"},
    sdl3 = {name = "SDL", use = "windows and input (SDL platform)", license = "Zlib", project = "https://www.libsdl.org"},
    stb = {name = "stb", use = "image decoding", license = "MIT or public domain",
           project = "https://github.com/nothings/stb"},
    tl_expected = {name = "tl::expected", use = "results", license = "CC0-1.0",
                   project = "https://github.com/TartanLlama/expected"},
    ["vulkan-headers"] = {name = "Vulkan headers", use = "graphics API", license = "Apache-2.0",
                          project = "https://github.com/KhronosGroup/Vulkan-Headers"},
    ["vulkan-memory-allocator"] = {name = "Vulkan Memory Allocator", use = "GPU memory", license = "MIT",
                                   project = "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator"},
    ["vulkan-memory-allocator-hpp"] = {name = "Vulkan Memory Allocator C++ bindings", use = "GPU memory",
                                       license = "CC0-1.0",
                                       project = "https://github.com/YaaZ/VulkanMemoryAllocator-Hpp"},
    yoga = {name = "Yoga", use = "user interface layout", license = "MIT", project = "https://github.com/facebook/yoga"},
}

-- What a package brings in with it, by the name `with` gives.
local extras = {
    openssl = {name = "OpenSSL", use = "HTTPS (not on Windows, which uses Schannel)", license = "Apache-2.0",
               project = "https://www.openssl.org"},
}

-- Libraries the release copies into `lib/` from the system, by file name.
local bundled = {
    ["libvulkan.1.dylib"] = {name = "Vulkan loader", use = "graphics API", license = "Apache-2.0",
                             project = "https://github.com/KhronosGroup/Vulkan-Loader"},
    ["libMoltenVK.dylib"] = {name = "MoltenVK", use = "Vulkan on Metal", license = "Apache-2.0",
                             project = "https://github.com/KhronosGroup/MoltenVK"},
    ["libstdc++.so.6"] = {name = "GCC C++ runtime", use = "the C++ standard library",
                          license = "GPL-3.0 with the GCC Runtime Library Exception", project = "https://gcc.gnu.org"},
    ["libgcc_s.so.1"] = {name = "GCC runtime", use = "the C++ standard library",
                         license = "GPL-3.0 with the GCC Runtime Library Exception", project = "https://gcc.gnu.org"},
}

-- The header of the notices table.
local header = {
    "| Library | Used for | License | Project |",
    "|---|---|---|---|",
}

-- @return The table row of `entry`.
local function row(entry)
    return format("| %s | %s | %s | %s |", entry.name, entry.use, entry.license, entry.project)
end

-- Adds to `rows` the rows of the package `name`, and of what it brings in; raises if the table does not know it.
local function addPackage(rows, name, binary)
    if name == "gtest" then
        return
    end
    local entry = packages[name]
    if not entry then
        raise("%s uses the package '%s', which xmake/modules/third_party_notices.lua does not know: add its notice", binary,
              name)
    end
    rows[row(entry)] = true
    for _, extra in ipairs(entry.with or {}) do
        rows[row(extras[extra])] = true
    end
end

-- @return The sorted rows of a set.
local function sorted(set)
    local list = table.keys(set)
    table.sort(list)
    return list
end

-- @return The rows of the third-party code the target `name` contains: its packages and those of the static modules
-- it links.
function rowsOfTarget(name)
    project.load_targets()
    local target = project.target(name)
    if not target then
        raise("no target %s", name)
    end
    local rows = {}
    local function addTarget(t)
        for _, pkg in ipairs(t:orderpkgs() or {}) do
            addPackage(rows, pkg:name(), name)
        end
    end
    addTarget(target)
    for _, dep in ipairs(target:orderdeps()) do
        if dep:kind() == "static" then
            addTarget(dep)
        end
    end
    return sorted(rows)
end

-- @return The rows of a library the release bundles from the system (its file name), or nil if it is not one.
function rowsOfBundled(filename)
    local entry = bundled[filename]
    return entry and {row(entry)} or nil
end

-- @return The target that builds the file `filename` (a library or an executable), or nil.
function targetOf(filename)
    project.load_targets()
    for _, target in pairs(project.targets()) do
        local kind = target:kind()
        if (kind == "shared" or kind == "binary") and path.filename(target:targetfile()) == filename then
            return target:name()
        end
    end
    return nil
end

-- Writes `<dir>/<filename>.md`: the rows of the binary `filename`, one per line.
function writeRows(dir, filename, rows)
    io.writefile(path.join(dir, filename .. ".md"), #rows > 0 and (table.concat(rows, "\n") .. "\n") or "")
end

-- Writes the whole notices file `file`: the text that opens it, then the table of `rows` (any order, made unique).
function writeNotices(file, rows)
    local set = {}
    for _, line in ipairs(rows) do
        set[line] = true
    end
    local lines = {
        "# Third-party software in the runtime",
        "",
        "The runtime contains the libraries below. Each is under its own license; the license texts are in the projects",
        "named here. `by-library/` says which file of the runtime contains which.",
        "",
    }
    table.join2(lines, header)
    table.join2(lines, sorted(set))
    io.writefile(file, table.concat(lines, "\n") .. "\n")
end
