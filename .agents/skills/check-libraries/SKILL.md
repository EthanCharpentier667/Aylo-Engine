---
name: check-libraries
description: Check a third-party library added to or changed in the AYLO Engine build - its notice for the release, its license, its pinned version, that it stays private to one module and builds on every platform. Use whenever a change touches an `xmake.lua` (or anything under `xmake/`) with `add_requires` or `add_packages`, when asked to "add a library", "add a package", "add a dependency", "use <lib>", "bump <lib>", or when `xmake package-release` or the Release workflow fails on a package without a notice.
---

# Check a third-party library

Every binary the release ships says which third-party code it contains (`runtime/licenses/`, made from
`xmake/modules/third_party_notices.lua`). A package the table does not know stops `xmake package-release`, and so the
Release workflow on `main`, on every platform. CI runs the same check (`xmake check-notices`, job *lint*); this skill
makes sure the change is right before it gets there.

## When

Run it on every change whose diff adds or changes an `add_requires` or an `add_packages`:

```sh
git diff <base> -- '*xmake.lua' 'xmake/**' | grep -E '^[+-].*(add_requires|add_packages)'
```

Nothing printed: nothing to do. Otherwise, go through the checklist for each package added, bumped or newly used by a
shipped target (a library or program of `src/`; the tests and examples are never in the zip).

## Checklist

1. **The notice.** The package's xmake name (the alias given to `add_requires`, else its name) has a row in the
   `packages` table of `xmake/modules/third_party_notices.lua`: `name` (as its project spells it), `use` (what the
   engine uses it for, a few words), `license` (SPDX id, `A or B` for a choice), `project` (its home page). A package
   that brings another library into the binary lists it in `with`, with a row in `extras`. A library the release copies
   from the system goes in `bundled`, by file name.
2. **The license.** Read it in the package's repository, do not guess. Permissive licenses (MIT, BSD, Zlib, Apache-2.0,
   BSL-1.0, ISC, public domain) are fine. GPL, LGPL (the engine links statically or ships libraries a game re-ships),
   or no license: stop and ask before adding it.
3. **The version** is pinned in `add_requires` (`"cgltf v1.15"`, `"joltphysics 5.6.x"`), never left open, so every
   platform and the release build the same code.
4. **Private to one module** (root `AGENTS.md`, rule 7): `add_packages` only in the module that wraps it, no public
   header includes it, and the module's line in `docs/Modules.md` says it is a private package. A package several
   modules use is required once in the root `xmake.lua`.
5. **Portable.** The package exists in xmake-repo for Linux, macOS and Windows; CI builds the three. If it needs a
   system tool or library, add it to the setup steps of `.github/workflows/ci.yml` and `release.yml`.
6. **Run the check:**

   ```sh
   xmake check-notices
   ```

   It prints `the N shipped binaries are covered`, or names each binary and the package without a notice. It loads
   the targets of the current configuration: configure with what the change builds (`xmake f -y --Tests=y`).

## Report

Say which packages were added or changed, their license, and the result of `xmake check-notices`. Then go on with
`verify`.
