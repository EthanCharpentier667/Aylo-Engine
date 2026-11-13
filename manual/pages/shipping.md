# Shipping your game

You make your game with the engine release, then export it: `aylo-export` puts your game and the part of the engine it
uses in one folder. The player installs nothing: no compiler, no library, no engine. This page covers the download, what
is in it, how to export, and what your players will see the first time they run it.

## Download the engine

Releases are on [GitHub](https://github.com/EthanCharpentier667/Aylo-Engine/releases). Each one holds one `.zip` per platform:

```
aylo-engine-<version>-<os>-<arch>.zip
```

For example `aylo-engine-0.3.0-linux-x86_64.zip`, `aylo-engine-0.3.0-windows-x86_64.zip` or
`aylo-engine-0.3.0-macos-arm64.zip`. Pick the one that matches the machine you develop on.

| Kind | Where | For |
|---|---|---|
| **Stable** | A release named `v<version>` | What you ship. Tested, and it stays available. |
| **Nightly** | The one prerelease named `nightly` | Trying what is coming. It is rebuilt after every change to the engine, it is not tested like a release, and it is replaced each time: never ship it. |

Unzip it anywhere. There is no installer: the folder is the engine.

## What is in the zip

```
aylo-engine-<version>-<os>-<arch>/
  runtime/                what your game ships
    aylo-runtime          the program (aylo-runtime.exe on Windows)
    (libraries)           what the program needs, next to it
    lib/                  the engine's libraries (on Windows they sit next to the program)
    plugins/              the engine's plugins, and plugins/backends/ for the window, the renderer and the sound
      backends/           the window, renderer and audio backends
    licenses/             the notices of the libraries inside
  editor/                 planned: the editor (aylo-editor). Never shipped.
  tools/aylo-export       exports your game: what you ship (aylo-export.exe on Windows)
  tools/aylo-sign         makes the signatures of native plugins
  templates/game/         a small game to start from
  manual/                 this manual, to read offline
  README.md  LICENSE  VERSION
  manifest.json           the version, and the checksum of every file
```

Nothing of the zip goes to your players as it is: `aylo-export` takes what your game needs from `runtime/`.

## Make a game with it

Copy `templates/game/` next to the zip's folder, rename it, and run it with the program in `runtime/`:

```sh
cp -r aylo-engine-0.3.0-linux-x86_64/templates/game my_game/game

# the server: no window, no graphics
./aylo-engine-0.3.0-linux-x86_64/runtime/aylo-runtime --server my_game/game
# a player
./aylo-engine-0.3.0-linux-x86_64/runtime/aylo-runtime my_game/game
```

The first argument is the folder with your `project.luau`, `scripts/` and `assets/`. [Getting started](getting-started.md)
explains the command line, and [Project settings](project-settings.md) the file. With
[hot reload](hot-reload.md) you can change scripts while the game runs.

## Ship your game

Export your game with `tools/aylo-export`: the folder of your game first, then where the export goes.

```sh
./aylo-engine-0.3.0-linux-x86_64/tools/aylo-export my_game/game exports
```

```
Exported to exports/my_game (10.4 MB)
  libaylo-plugin-graphics.so: named in plugins (client)
  libaylo-plugin-input.so: linked by libaylo-plugin-graphics.so; required by aylo.graphics
  libaylo-plugin-transform.so: every game; linked by libaylo-plugin-graphics.so
  libaylo-platform-glfw.so: backend 'glfw' (platform)
  libaylo-render-vulkan.so: backend 'vulkan' (renderer)
```

It reads your `project.luau` the way the engine does, once for the server and once for a player, and keeps only the
plugins and backends it names, the ones those plugins need, and `aylo.Transform`, which every game has.
Each line says why a library is kept. The template game above ships five of the engine's libraries instead of all of
them.

The export is a folder you zip and publish:

```
exports/my_game/
  my_game               the program (my_game.exe on Windows)
  game.pak              your game in one file: project.luau, scripts/ (compiled), assets/
  lib/                  what every plugin needs (on Windows, next to the program)
  plugins/              only the plugins your game uses
    backends/           only the window, renderer and audio backends it names
  game/
    plugins/            your native plugins and their signatures, if you have some
  licenses/
    THIRD_PARTY_NOTICES.md   the notices of the libraries kept
  play.sh               or play.bat on Windows
  export.json           the engine version, the libraries kept and why, the SHA-256 of every file
```

`game.pak` holds your `project.luau`, your `assets/` and your `scripts/`, compiled: you do not hand out the source of
your scripts, and the export stops on a script that does not compile, before it writes anything. The program finds
`game.pak` next to itself, so a player can also start it without `play.sh`. A game run from its pack never reloads its
files, whatever `hotReload` says. Your [native plugins](native-plugins.md) stay files, in `game/plugins/`: a library is
not loaded from a pack.

| Option | Default | What it does |
|---|---|---|
| `--name <name>` | the name of your game's folder (`my_game` for `my_game/game`) | the name of the export folder and of the program |
| `--runtime <folder>` | the zip's `runtime/`, next to `tools/` | the runtime the export takes the program and the libraries from |
| `--no-pack` | off | ships your game as its folder, `game/`, with the scripts as you wrote them, instead of `game.pak` |

Your players run `play.sh` (`play.bat` on Windows), and a dedicated server runs `./play.sh --server`. Every other flag
reaches your `project.luau` as `engine.args`, as with `aylo-runtime`. Check the export on a machine that has never seen
the engine, or in a clean virtual machine, before you publish it.

!!! warning
    The export reads `project.luau` with no flags, as a player starts the game. If a flag changes the plugins or the
    backends you name (`engine.args.headless` in the template), name in the default branch everything the other
    branches need, or the export leaves it out and that branch stops with `unknown plugin`.

What the export leaves out of your game: the files and folders whose name starts with a dot (`.luaurc` is for your
editor), and in `plugins/` everything but the libraries and their `.sig` files: the sources of your
[native plugins](native-plugins.md) stay with you.

### When the export stops

The export checks everything before it writes anything, and leaves nothing behind when it stops:

| Message | Cause |
|---|---|
| `Invalid setting 'plugins' (client): unknown plugin 'game.Bos' (known: ...)` | `project.luau` names a plugin no library brings: a typo, or a native plugin you did not build |
| `Plugin 'aylo.Sprite' requires 'aylo.graphics', which no library registers (known: ...)` | the runtime is missing a library: export with the whole `runtime/` of a release |
| `Invalid setting 'platform': no backend library provides 'glfx' (found in '...': 'glfw', 'raylib', ...)` | a backend name with a typo (`platform`, `renderer`, `audio.backend`) |
| `Cannot export: scripts/player.luau does not compile: ...` | a script with a syntax error: the message gives its line |
| `'exports/my_game' already exists and is not empty: remove it, or export elsewhere` | an export is never written over another one |
| `Invalid name 'a/b': it names a folder and a program, so it must be a file name` | `--name` with a `/`, `\` or `:` in it |

A warning, not an error: `The runtime has no notices for ...`. You exported with a runtime that is not the release's (a
build of the engine): the export has no third-party notices. Export with the release zip to ship them.

### What the player needs

Nothing to install. The runtime brings its own libraries. A player needs only the operating system and a graphics driver
with Vulkan support, which comes with the driver of any recent graphics card.

## Your players' first run

The runtime is not signed by a company the operating system knows, so the first start can show a warning. Tell your
players, or sign your own build (next section).

**macOS.** A zip downloaded in a browser is marked as coming from the internet, and macOS refuses to open its programs.
This command clears the mark on the unzipped folder:

```sh
xattr -dr com.apple.quarantine MyGame/
```

A download made with `curl` or `gh` is not marked.

**Windows.** SmartScreen shows "Windows protected your PC". Click *More info*, then *Run anyway*.

**Linux.** There is no prompt. If the zip tool dropped the right to run, restore it with
`chmod +x MyGame play.sh`.

## Sign your own build

The engine's zip carries free, basic protection (see below). A shipped game looks more trustworthy, and gives your
players no prompt, when you sign it yourself. This is done with the usual tools of each platform, on your files, after
you renamed the program:

- **Windows:** sign `MyGame.exe` and the `.dll` files with a code-signing certificate and `signtool`. Certificates are
  sold by certificate authorities. Some programs sign open-source projects for free.
- **macOS:** sign the program and every library with `codesign`, then have Apple notarize the folder. This needs an
  Apple Developer account. Without it, the quarantine command above is the way.
- **Linux:** nothing is needed.

## Check what you downloaded

Every release comes with `SHA256SUMS`. Download it next to the zips and compare:

```sh
sha256sum --check --ignore-missing SHA256SUMS      # Linux
shasum -a 256 --check --ignore-missing SHA256SUMS  # macOS
```

```powershell
Get-FileHash aylo-engine-0.3.0-windows-x86_64.zip  # Windows: compare with the line of SHA256SUMS
```

Every zip also has a GitHub **attestation**: a statement signed by GitHub that the zip was built by the project's own
workflow from a given commit. With the [GitHub CLI](https://cli.github.com):

```sh
gh attestation verify aylo-engine-0.3.0-linux-x86_64.zip --repo EthanCharpentier667/Aylo-Engine
```

It prints the commit and the workflow, or fails if the file is not what the project built.

## Native plugins

A game written in Luau needs nothing more. If your game ships a [native plugin](native-plugins.md), the release build of
the engine opens it only if it comes with a signature, made with `tools/aylo-sign`, from a developer the project vouches
for. Ask the project for a certificate for your key, and sign each build of your library. See
[Native plugins](native-plugins.md#what-the-engine-checks).

## Update the engine

Engine versions follow [semantic versioning](https://semver.org): `major.minor.patch`.

| Version | Meaning |
|---|---|
| `0.x.y` | The script API is still settling. **A minor version can break your scripts.** Read the notes of a release before you update, and ship the version you tested. |
| `1.0.0` and later | A patch fixes bugs, a minor version adds features and keeps your scripts working, and only a major version can break them. |

To update, unzip the new release and export your game again with its `tools/aylo-export`, into a new folder. Do not mix
files from two exports: the libraries and plugins of a release are made for each other. Then run your game and play it
once through before you ship.

## See also

- [Getting started](getting-started.md): the command line and the folder of a game.
- [Project settings](project-settings.md): the plugins and backends your game chooses, which the export keeps.
- [Native plugins](native-plugins.md): the C++ plugins a game can ship, and their signatures.
