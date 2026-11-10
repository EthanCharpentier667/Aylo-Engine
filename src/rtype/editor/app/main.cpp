/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// aylo-editor: the editor of a game, in `game/` (or the <directory> given first). It runs both sides of the game in one
// world (no network) inside the editor's panels, always as a debug run (`engine.isDebug`); "Play online" instead runs a
// server as a child process and rebuilds the editor as a client of it. The game is shipped with aylo-runtime. Every
// flag goes to project.luau as `engine.args`, and `engine.args.editor` is always there.
//   aylo-editor [<directory>] [--<flag> <value>]...
//   aylo-editor --version

#include <algorithm>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/config/Settings.hpp"
#include "core/library/LibraryEntry.hpp"
#include "core/system/ExecutablePath.hpp"
#include "core/version/Version.hpp"
#include "editor/EditorPlugin.hpp"
#include "editor/EditorSession.hpp"
#include "editor/EditorSettings.hpp"
#include "editor/EditorState.hpp"
#include "editor/console/ScriptPrompt.hpp"
#include "editor/shipping/GameExport.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/engine/Engine.hpp"
#include "rtype/engine/exporting/GameExport.hpp"
#include "rtype/engine/exporting/LibrarySelection.hpp"

namespace {
constexpr int kErrorCode = 84;                          ///< What the program returns when it cannot run the game.
constexpr std::string_view kServerFlag = "--server";    ///< Refused: the editor runs both sides.
constexpr std::string_view kVersionFlag = "--version";  ///< Prints the version of the engine and stops.
constexpr std::string_view kEditorFlag = "--editor";    ///< Given to the project, so it can tell it runs in the editor.

// What the build says about itself: xmake gives the version and the commit (xmake/modules/engine_version.lua).
// NOLINTBEGIN(cppcoreguidelines-macro-usage): the build passes them as macros; these are the defaults without it.
#ifndef RTYPE_ENGINE_VERSION
#define RTYPE_ENGINE_VERSION "0.0.0"
#endif
#ifndef RTYPE_ENGINE_COMMIT
#define RTYPE_ENGINE_COMMIT ""
#endif
// NOLINTEND(cppcoreguidelines-macro-usage)

#ifdef NDEBUG
constexpr bool kIsRuntimeDebug = false;  ///< `engine.isDebug` of an export: the runtime is built as the editor is.
#else
constexpr bool kIsRuntimeDebug = true;  ///< `engine.isDebug` of an export: the runtime is built as the editor is.
#endif

/// @return The runtime an export ships: the release zip's `runtime/` next to the editor's folder, else the editor's own
/// folder (a build folder, where aylo-runtime sits next to it).
std::filesystem::path exportRuntime() {
    std::filesystem::path here = rtype::core::system::getExecutableDirectory();
    std::filesystem::path zip = here.parent_path() / "runtime";
    if (std::filesystem::is_regular_file(zip / rtype::engine::exporting::runtimeProgramName())) {
        return zip;
    }
    return here;
}

/// @brief Prints what `--version` says: the version, the commit, the toolchain and the library ABI version.
void printVersion() {
    const rtype::core::version::VersionInfo info{
        .version = RTYPE_ENGINE_VERSION,
        .commit = RTYPE_ENGINE_COMMIT,
        .toolchain = RTYPE_LIBRARY_TOOLCHAIN,
        .libraryAbiVersion = rtype::core::library::kLibraryAbiVersion,
    };

    std::cout << rtype::core::version::describe(info);
}
}  // namespace

int main(const int argc, char** argv) {
    try {
        const std::span rawArgs{argv, static_cast<std::size_t>(argc)};
        std::vector<std::string> args;
        if (!rawArgs.empty()) {  // Some platforms start a program with no argv[0].
            args.assign(rawArgs.begin() + 1, rawArgs.end());
        }
        if (std::ranges::find(args, kVersionFlag) != args.end()) {
            printVersion();
            return 0;
        }
        if (std::ranges::find(args, kServerFlag) != args.end()) {
            std::cerr << "Error: --server is not for the editor: it runs both sides\n";
            return kErrorCode;
        }
        if (std::ranges::find(args, kEditorFlag) == args.end()) {
            args.emplace_back(kEditorFlag);
        }
        // What outlives each App: the child server and the Console. Every build of the editor gets the same one.
        const auto session = std::make_shared<rtype::editor::EditorSession>(true);
        const std::vector<std::string> baseArgs = args;
        rtype::engine::Engine engine{{
            .mode = rtype::ecs::RunMode::kStandalone,
            .args = std::move(args),
            .addPlugins =
                [session](rtype::ecs::App& app, const rtype::core::config::Settings& settings) {
                    rtype::editor::EditorOptions options = rtype::editor::readEditorOptions(settings);
                    options.session = session;
                    app.addPlugin(rtype::editor::EditorPlugin{std::move(options)});
                },
            .debug = true,
        }};
        // The Console's input line runs Luau in the scripts of the App the engine has now (the editor knows no Luau).
        session->prompt.setBackend({
            .evaluate =
                [&engine](const std::string_view line, const rtype::editor::console::ScriptTarget& target) {
                    auto evaluation = engine.evaluateScript(line, {.self = target.self, .writable = target.writable});
                    return rtype::editor::console::ScriptOutcome{
                        .ok = evaluation.ok,
                        .text = std::move(evaluation.text),
                    };
                },
            .complete =
                [&engine](const std::string_view word, const rtype::editor::console::ScriptTarget& target) {
                    return engine.completeScript(word, {.self = target.self, .writable = target.writable});
                },
        });
        // File > Export... exports the game as aylo-export does, with the runtime next to the editor.
        session->exporter = [&engine](const rtype::editor::shipping::ExportChoice& choice) {
            if (engine.isPacked()) {
                throw std::runtime_error{"The editor edits a game folder: export it from its folder"};
            }
            const rtype::engine::exporting::ExportReport report = rtype::engine::exporting::exportGame({
                .game = engine.gameDirectory(),
                .runtime = exportRuntime(),
                .output = choice.output,
                .name = choice.name,
                .engineVersion = RTYPE_ENGINE_VERSION,
                .debug = kIsRuntimeDebug,
                .pack = choice.pack,
            });
            rtype::editor::shipping::ExportResult result{.ok = true,
                                                         .folder = report.folder.generic_string(),
                                                         .bytes = report.bytes,
                                                         .warnings = report.warnings};
            for (const rtype::engine::exporting::SelectedLibrary& library : report.libraries) {
                std::string reasons;
                for (const std::string& reason : library.reasons) {
                    reasons += std::format("{}{}", reasons.empty() ? "" : "; ", reason);
                }
                result.libraries.push_back(std::format("{}: {}", library.file.filename().generic_string(), reasons));
            }
            return result;
        };
        // Play online rebuilds the game as a client of the server, and Stop as standalone again.
        session->online.setRebuild(
            [&engine](const rtype::ecs::RunMode mode, std::vector<std::string> flags) {
                engine.requestReload(mode, std::move(flags));
            },
            baseArgs);

        return engine.run();
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return kErrorCode;
    }
}
