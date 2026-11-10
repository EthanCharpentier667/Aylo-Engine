# Game export: plan

Goal: **a game developer exports their game, and the player gets only what the game needs: one program, the plugin and
backend libraries `project.luau` names, and the game packed in one file.** Development does not change: the editor,
hot reload and `aylo-runtime <dir>` keep reading the game folder and every plugin of the engine.

Based on the code of 2026-11-10 (the release zip of [plan.md](plan.md), the runtime layout of
[`../Plugins.md`](../Plugins.md#loading)).

## 1. Where we are

A shipped game today is the engine's `runtime/` folder as it came, next to the game's `game/` folder
([Shipping your game](../../manual/pages/shipping.md)):

- `runtime/plugins/` holds every plugin and `plugins/backends/` every backend (Vulkan, raylib, SDL, GLFW, miniaudio...),
  whatever the game names. The manual tells the developer to remove a file only if they checked that the game does not
  name it.
- `game/` is shipped as source: `project.luau`, the `.luau` scripts and the assets, file by file.
- Everything the engine reads comes from the file system: the scripts (`scripting/modules/ScriptResolver`), the assets
  (`core::asset::resolveAssetPath()` and `readTextFile()`, used by the asset stores of the plugins), the capsule files
  (`plugins/capsule/CapsuleFiles`), and the sounds, opened by the audio backend from a path
  (`IAudioDevice::createSound(path)`).

## 2. Decisions

| #  | Decision | Why |
|----|----------|-----|
| E1 | **The model is the one of Godot and Unity: a runtime built by the engine's CI, the game as data next to it.** No game is compiled. An export copies and packs; it never needs a compiler or an `xmake.lua`. | A game is Luau only (`AGENTS.md`, rule 1 and 2). A binary per game (the Unreal model) would ask every game developer for a C++ toolchain. |
| E2 | **One export code, two front ends**: the tool `aylo-export` (in the release zip, next to `aylo-sign`) and an *Export* menu of the editor, which calls the same code. | A game's CI uses the tool, a developer the editor; both give the same result. |
| E3 | **The export keeps only the plugins and backends the game needs**: those `project.luau` names (`plugins`, `platform`, `renderer`, `audio`...), the plugins they require (`Plugin::getDependencies()`, transitively), and the ones every game has (`rtype.Transform`, `MinimalPlugins`). `lib/` is copied whole. | The plugins and backends are most of the size; `lib/` (`ecs`, the contracts, the C++ runtime) is small and needed by any plugin. |
| E4 | **Both sides are resolved**: the client's and the server's plugins (`project.luau` read once per side). An export makes one folder that runs as a client and with `--server` as a server, like `aylo-runtime`. | The same download is a player's game and a dedicated server. A server-only export can come later. |
| E5 | **The game is one pack file, `game.pak`**: `project.luau`, the scripts compiled to Luau bytecode, the assets. The engine reads through a **file source** (`core::vfs`): a folder in development, the pack in an exported game. | One file to ship, no script source handed out, and the path checks of the assets become checks on a closed list of entries. |
| E6 | **Hot reload is off in a pack.** A packed game has no files to watch; `hotReload = true` is ignored with a log line. | A pack is a shipped build. |
| E7 | **A pack is data from outside**: a damaged or hostile pack never crashes, throws or asserts past its reader; it is refused with its path and the reason. | `AGENTS.md`, rule 6 and 9. |
| E8 | **Game plugins are copied as built**, with their `.sig`, from `<game>/plugins/`. The export compiles nothing and signs nothing. | Signing stays the developer's step (`aylo-sign`, [`../Plugins.md`](../Plugins.md#signatures)). |
| E9 | **The licenses follow the libraries**: each binary of the runtime has its notices in `licenses/by-library/<file>.md`, and the export ships those of the binaries it keeps. | A trimmed runtime must still carry every notice it needs, and nothing more. |
| E10 | **Later, not in this plan**: the pack inside the executable (one file), and a static runtime with every plugin linked in. Both are listed in [Later](#5-later) with what they need. | They are not needed to ship only what a game uses, and each has a cost of its own (signing on macOS, an executable per set of plugins). |

## 3. What an export makes

```
aylo-export <game> <out> [--name MyGame] [--runtime <dir>]
```

```
<out>/MyGame/
  MyGame(.exe)            the runtime program, renamed
  lib/                    copied whole (on Windows, next to the program)
  plugins/                only the plugins the game needs
    backends/             only the backends it names
  game.pak                the game: project.luau, scripts as bytecode, assets (until EXP-08: game/, as a folder)
  game/plugins/           the game's own plugin libraries and their .sig, where the Engine looks for them
  vulkan/                 as in the runtime, when it has one (the MoltenVK manifest on macOS)
  licenses/               THIRD_PARTY_NOTICES.md: the notices of the binaries kept
  play.sh / play.bat      the launcher (the program finds game.pak next to it, so it can also be started alone)
  export.json             the engine version, the plugins and backends kept and why, the sha256 of every file
```

`export.json` says, for each plugin kept, what asked for it (`project.luau`, `required by rtype.Sprite`, `every game`),
so a developer can see why a library is in their game.

The program starts the pack next to it when no game folder is given, so the launcher becomes optional.

## 4. Tasks

| Id     | Task | Blocked by | Priority | Size |
|--------|------|------------|----------|------|
| EXP-01 | Plugin resolution: what a game needs, from its settings and the libraries | none | P1 | M |
| EXP-02 | Licenses per library in `manifest.json` | none | P2 | S |
| EXP-03 | `aylo-export` tool: the trimmed folder, with the game folder as it is | EXP-01, EXP-02 | P1 | M |
| EXP-04 | File source (`core::vfs`): a folder or a pack, and every reader moved to it | none | P1 | L |
| EXP-05 | Sounds from bytes: a streamed sound from memory | none | P1 | S |
| EXP-06 | Pack format: writer, reader, scripts as bytecode | EXP-04 | P1 | M |
| EXP-07 | The engine runs a pack (`game.pak` next to the program, or given) | EXP-04, EXP-05, EXP-06 | P1 | M |
| EXP-08 | `aylo-export` packs the game | EXP-03, EXP-07 | P1 | S |
| EXP-09 | Export menu of the editor | EXP-08 | P2 | M |
| EXP-10 | CI: export the template game and run it on the three platforms | EXP-08 | P1 | M |
| EXP-11 | Docs: shipping a game with an export, the engine side | EXP-08 | P1 | S |

```
EXP-01 --\
          +-> EXP-03 ----------------\
EXP-02 --/                            +-> EXP-08 -> EXP-09
EXP-04 -> EXP-06 --\                  |         \-> EXP-10
EXP-05 ------------+-> EXP-07 --------/          \-> EXP-11
```

EXP-07 is also blocked by EXP-04 directly: the engine reads through the file source.

EXP-03 alone already answers the size of a shipped game: the trimmed folder, with `game/` copied as it is. The pack
(EXP-04 to EXP-08) comes on top.

### EXP-01: plugin resolution

`exporting::selectLibraries()` in `engine` (`engine/exporting/`; `export` is a keyword) takes a game folder and the
runtime's folder and returns the libraries the game needs, each with the reasons it is kept
([`../engine/design.md`](../engine/design.md#what-a-game-needs)):

- read `project.luau` once per side, as the `Engine` does (`scripting::loadProjectSettings()`, shared with it);
- load the libraries of `plugins/`, `plugins/backends/` and `<game>/plugins/` (`plugin::PluginLoader`), and register
  them one at a time, comparing the registries before and after each, to know which library registers which plugin
  and which backend. A backend registers into a registry typed by its contract, which `engine` cannot name: the plugin
  that offers the registry also offers its names (`LibraryRegistrar::provideNames()`, `kLibraryAbiVersion` 2);
- from the plugins named, the backends named and the plugins every game has, follow `getDependencies()` until nothing
  is added; a name no library registers is an error naming it and the names that exist;
- add the plugin libraries a kept library links: the system loads them with it, even when its plugins do not require
  theirs. The build writes them next to each library, `<library>.links` ([`../Plugins.md`](../Plugins.md#what-a-library-links)).

To read `getDependencies()`, the plugin is created from its factory with the side's settings and never built. The
engine's plugins do no work in their constructor (the tests create the graphics, shader and physics plugins this way).

Tests (`tests/engine/LibrarySelectionTests.cpp`): an empty game keeps the transform plugin alone; a game naming one
plugin keeps it and what it requires and nothing else; the server and client sides are joined; a backend not named is
left out; a game plugin and what it links are kept; an unknown plugin or backend is an error naming it.

### EXP-02: licenses per library

`xmake/package-release.lua` writes, for each binary of `runtime/`, the notices of the third-party code it contains, in
`licenses/by-library/<file>.md` (one table row per package), and `THIRD_PARTY_NOTICES.md` from all of them
([`package.md`](package.md)). The rows come from a table in `xmake/modules/third_party_notices.lua`, by xmake package:
a binary holds the packages of its target and of the static modules it links, and a package the table does not know
stops the release. This replaces the hand-made `xmake/package/THIRD_PARTY_NOTICES.md`, which had fallen behind (no SDL,
raylib, curl or OpenSSL). The smoke test checks that the program and every plugin and backend have their file.

### EXP-03: `aylo-export`, the trimmed folder

`exporting::exportGame()` in `engine` (`engine/exporting/GameExport`, so the editor can call it, EXP-09) and the
program `aylo-export` (`src/rtype/tools/export/`, linking `engine`: it is an executable) that runs it. It makes the
layout of [section 3](#3-what-an-export-makes) with `game/` copied as a folder instead of `game.pak`: the program
renamed, `lib/` and `vulkan/` whole, the libraries of selectLibraries(), the game without its hidden files and without
the sources of its plugins (only their libraries and `.sig` files), the notices merged from `licenses/by-library/`
(EXP-02; a build folder has none, which is a warning), the launcher of the platform and `export.json`. Everything is
checked before a file is written, an output folder that is not empty is refused, and a failure removes what was
written. `engine.isDebug` is the build of the tool, which is the build of the runtime of its zip, so there is no
`--debug`. The runtime is the zip's `runtime/` next to `tools/` by default, or `--runtime`. The release zip ships it in
`tools/`. Tests: `tests/engine/GameExportTests.cpp`; the manual: `manual/pages/shipping.md`.

`project.luau` is read with no flags, as a player starts the game: a game whose plugins depend on a flag names, in its
default branch, what the others need (the manual says so). Passing the flags of the export to `engine.args` is left for
later.

### EXP-04: file source

`core::vfs::FileSource` (`core/vfs/`), an interface with `read(path)` (the bytes), `exists(path)`, `list(folder)` and
`directory()` (the folder on disk, if the files are files on disk), and `core::vfs::DirectorySource`, a folder with the
checks of `resolveAssetPath()` (`..`, absolute paths, links leaving the folder). `core::vfs::readBytes()` gives the bytes
of an image or a sound. `core` stays without state: a source is an object the world holds.

The `AssetDirectory` resource gets a `source` (the Engine puts a `DirectorySource` on `<game>/assets/` in it), and
`plugins::assetSourceOf()` gives it to the plugins. Every reader moved to it:

- `AssetStore` (atlases, fonts, tilemaps, particle effects) and the external tilesets of a Tiled map;
- `TextureAssets`, `ShaderAssets`, and the images the atlas packer packs (`packImages()` takes a reader);
- `AudioAssets`: a sound is opened by its path when the source has a folder (a streamed music keeps reading it), from
  its bytes otherwise (EXP-05 streams them);
- `CapsuleFiles`, which lists the capsule files with `list()`.

`setDirectory(path)` stays on each store, a `DirectorySource` on that folder, for the programs and tests without an
Engine. Hot reload follows the assets only when `AssetDirectory` has a folder (`path`). The scripts and `project.luau`
are read by the scripting module and the settings loader, which move to a source with EXP-07, where the engine runs a
pack. Tests: `tests/core/DirectorySourceTests.cpp`, and a store and the capsule files reading a source held in memory
(`AssetStoreTests`, `CapsuleFilesTests`).

### EXP-05: sounds from bytes

The contract already had `IAudioDevice::createSound(encoded)`, and the audio plugin gives it the bytes from the file
source when the assets have no folder (EXP-04). What did not work was a streamed sound given as bytes: miniaudio's
resource manager streams files only, by their path, so a streamed music from a pack could not play. `MiniaudioDevice`
now keeps the bytes of such a sound, and each voice decodes them while it plays (`core::Sound` from memory: a
`ma_decoder` on the bytes, `ma_sound_init_from_data_source`). No contract changed, and miniaudio is the only backend
with an audio device. Tests: `tests/audio/MiniaudioDeviceTests.cpp` (a new `audio-tests` module), which plays a sound
and streams one given as bytes, on miniaudio's null device where the machine has no sound card.

### EXP-06: pack format

A small format, written and read by `core::vfs`: a header (magic, version), a table of entries (normalized path,
offset, size, sha256), the data, no compression at first. The reader checks every offset and size against the file,
refuses a path that is absolute or contains `..`, and reads an entry only when asked. The scripts are stored as Luau
bytecode (`luau::Bytecode`), and the script loader takes bytecode as well as source.

Tests (`tests/core/`): a written pack reads back the same bytes; a truncated pack, an offset past the end, a duplicate
path and a path leaving the pack are each refused with a message, never a crash.

### EXP-07: the engine runs a pack

`aylo-runtime` (and the renamed program) starts `game.pak` next to itself when no folder is given, or a `.pak` given as
the first argument. The `Engine` takes a file source instead of a folder; a pack turns hot reload off (E6). The
editor keeps folders only.

### EXP-08: `aylo-export` packs the game

`aylo-export` writes `game.pak` from the game folder, compiling each script (a script that does not compile stops the
export with its file and line). `--no-pack` keeps the folder of EXP-03.

### EXP-09: export menu of the editor

*File > Export...*: the output folder, the name, then the same function as the tool, with its result (the libraries
kept and why, the size) in a panel. `docs/Editor.md` and `manual/pages/editor.md` are updated.

### EXP-10: CI

The smoke test (`.github/scripts/smoke-test.sh` and `.ps1`) exports `templates/game` with the packaged zip, runs the
export in a clean folder as a server and as a headless client (`--server`, `--headless`, as it runs `runtime/` today),
and checks that `plugins/` holds only what `export.json` lists.

### EXP-11: docs

`manual/pages/shipping.md` rewritten around `aylo-export` (with the `write-manual` skill), `docs/release/package.md`
for `tools/aylo-export`, `docs/engine/design.md` for the file source and the pack.

## 5. Later

- **One file.** The pack appended to the program, found by a footer (magic, offset) at the end of the executable.
  Windows and Linux accept it; on macOS the signature covers the whole file, so the pack goes into an `.app` bundle
  (`Contents/Resources/game.pak`) and the bundle is signed after. The plugins stay files.
- **A static runtime.** A program with every plugin and backend linked in, built by the engine's CI: one file, no
  library to load, but every game carries every plugin again, and game plugins cannot be loaded into it. It reverses
  several rules of `AGENTS.md` (the executables link no plugin, `ecs` and the contracts are shared), so it would be a
  decision of its own.
- **A server-only export**: the server's plugins only, no window or renderer backend.
- **Compression** of the pack's entries.
