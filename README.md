# AYLO Engine

[![CI](https://github.com/EthanCharpentier667/Aylo-Engine/actions/workflows/ci.yml/badge.svg)](https://github.com/EthanCharpentier667/Aylo-Engine/actions/workflows/ci.yml)
[![Manual](https://img.shields.io/badge/manual-online-blue)](https://ethancharpentier667.github.io/Aylo-Engine/)
[![Developer docs](https://img.shields.io/badge/developer%20docs-online-blue)](https://ethancharpentier667.github.io/Aylo-Engine/engine/)

**AYLO** (*Another Yet Lightweight Optimized Engine*) is a C++23 game engine made at Epitech (Tek3):
a Vulkan renderer, Luau scripting, physics, audio and an authoritative client/server network model over UDP. It does
not contain a game: it runs one. A game is a folder of Luau scripts and assets (see `examples/games/`).

It is made for every kind of game, not one genre: 2D today, 3D next.

## Features

- **Entity Component System** on paged sparse sets, with change detection, events, hierarchy and scenes (capsules).
- **Plugins**: every feature (sprites, text, tilemaps, UI, particles, audio, physics, saves, HTTP...) is a shared
  library a game lists by name in `project.luau`.
- **Luau scripting** with hot reload: components, systems, inputs and network packets are declared in scripts.
- **Networking**: a binary protocol over UDP with its own reliability layer, snapshots, client prediction and
  reconnection ([protocol](docs/network/protocol.md)).
- **Vulkan renderer** with GPU particles and user shaders; **Jolt** physics; **miniaudio** sound.
- **Editor** (`aylo-editor`): scenes in docked panels, with the game playing inside.

## Requirements

| | Linux | Windows | macOS |
|---|---|---|---|
| Compiler | Clang (C++23) | Clang or MSVC (Visual Studio 2022) | Apple Clang (Xcode) |
| Build | [xmake](https://xmake.io) | xmake | xmake |
| System libraries | Vulkan loader, X11 and xkbcommon headers (GLFW builds its X11 and Wayland support with them): `vulkan-tools libvulkan-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxkbcommon-dev` (and `libgl-dev` for the raylib backend) | A Vulkan driver | `brew install molten-vk vulkan-loader` |

Every other dependency (GLFW, GLM, Luau, Asio, Jolt, miniaudio, Yoga, ImGui, libcurl...) is downloaded and built by
xmake's package manager into a per-user cache: nothing else is installed on the system.

## Build and run

```sh
xmake f -y                 # configure (add -m release for a release build)
xmake build -y             # build: copies aylo-runtime and aylo-editor to the repository root

./aylo-runtime --server    # the server (on game/)
./aylo-runtime             # a client (on game/)
./aylo-runtime <game_dir>  # any game: a folder with a project.luau
./aylo-editor <game_dir>   # the editor
```

The server listens on UDP port 4242. Other flags (`--port <port>`, `--host <address>`...) reach the game's
`project.luau` as `engine.args`, which decides what to do with them.

Example games are in [`examples/games/`](examples/games/): `aylo_duel` is a complete two-player game.

```sh
./aylo-runtime --server examples/games/aylo_duel/game
./aylo-runtime examples/games/aylo_duel/game
```

### Starting example: Aylo-Example-Circle

[**Aylo-Example-Circle**](https://github.com/EthanCharpentier667/Aylo-Example-Circle) is a small public game, written
only in Luau (a window with a circle in the middle, Vulkan and GLFW), that is kept up to date with the engine: it is
the reference example of each version of the engine. It is built against an engine release, with no compiler needed,
and its [releases](https://github.com/EthanCharpentier667/Aylo-Example-Circle/releases) hold the game zipped for Linux,
macOS and Windows.

Tests: `xmake f -y --Tests=y && xmake build -y && xmake test -v`.

## Documentation

| | For | Link |
|---|---|---|
| **Game developer manual** | Making a game in Luau | <https://ethancharpentier667.github.io/Aylo-Engine/> |
| **Developer documentation** | Working on the engine: architecture, modules, network protocol, technical study | <https://ethancharpentier667.github.io/Aylo-Engine/engine/> ([`docs/`](docs/index.md)) |
| **Network protocol** | Writing another client | [`docs/network/protocol.md`](docs/network/protocol.md) |
| **Contributing** | Workflow, conventions, definition of done | [`CONTRIBUTING.md`](CONTRIBUTING.md) |

## Authors

- Charles Mahoudeau
- Ethan Charpentier
- Noé Caillaud
- Jhong Amos Almacin

Questions and bugs: [GitHub issues](https://github.com/EthanCharpentier667/Aylo-Engine/issues).

## License

[MIT](LICENSE).
