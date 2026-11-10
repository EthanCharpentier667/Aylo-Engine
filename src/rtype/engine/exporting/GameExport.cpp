/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GameExport
*/

#include "GameExport.hpp"

#include <sodium/core.h>
#include <sodium/crypto_hash_sha256.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "rtype/engine/Engine.hpp"
#include "rtype/engine/Exceptions.hpp"
#include "rtype/engine/exporting/LibrarySelection.hpp"
#include "rtype/engine/plugin/PluginSignature.hpp"
#include "rtype/engine/plugin/SharedLibrary.hpp"

namespace rtype::engine::exporting {
namespace {
#ifdef _WIN32
constexpr bool kWindows = true;
constexpr std::string_view kProgramExtension = ".exe";
#else
constexpr bool kWindows = false;
constexpr std::string_view kProgramExtension;
#endif

constexpr std::string_view kGameFolder = "game";          ///< The game, in the export.
constexpr std::string_view kLicensesFolder = "licenses";  ///< The notices, in the runtime and in the export.
constexpr std::string_view kNoticesFile = "THIRD_PARTY_NOTICES.md";
constexpr std::string_view kByLibraryFolder = "by-library";  ///< The notices of each binary, in the runtime.
constexpr std::string_view kExportFile = "export.json";

/// @return The name of the game when the request gives none: `MyGame` for `MyGame/game`, else the folder's own name.
std::string defaultName(const std::filesystem::path& game) {
    const std::filesystem::path folder = game.filename().empty() ? game.parent_path() : game;
    if (folder.filename() == kGameFolder && !folder.parent_path().filename().empty()) {
        return folder.parent_path().filename().string();
    }
    return folder.filename().string();
}

/// @brief Refuses a name that is not one file name: the export's folder and program are named after it.
void checkName(const std::string& name) {
    const bool bad = name.empty() || name == "." || name == ".." ||
                     name.find_first_of("/\\:*?\"<>|") != std::string::npos ||
                     std::ranges::any_of(name, [](const char c) { return static_cast<unsigned char>(c) < 0x20; });
    if (bad) {
        throw exceptions::RuntimeError{
            std::format("Invalid name '{}': it names a folder and a program, so it must be a file name", name)};
    }
}

/// @brief Copies @p from to @p to, making its folder, and keeps the execution bits of a program.
void copyFile(const std::filesystem::path& from, const std::filesystem::path& to) {
    std::filesystem::create_directories(to.parent_path());
    std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
    std::filesystem::permissions(to, std::filesystem::status(from).permissions(),
                                 std::filesystem::perm_options::replace);
}

/// @brief Copies the game folder @p from to @p to, without hidden files and with only the libraries of `plugins/`
/// and their signatures (not the sources a game builds them from).
void copyGame(const std::filesystem::path& from, const std::filesystem::path& to) {
    const std::filesystem::path plugins = from / Engine::kPluginsDirectory;
    for (auto entry = std::filesystem::recursive_directory_iterator{from};
         entry != std::filesystem::recursive_directory_iterator{}; ++entry) {
        const std::filesystem::path& path = entry->path();
        if (path.filename().string().starts_with('.')) {
            if (entry->is_directory()) {
                entry.disable_recursion_pending();
            }
            continue;
        }
        if (path.parent_path() == plugins) {
            const std::string name = path.filename().string();
            const bool library = path.extension() == plugin::SharedLibrary::getExtension();
            const bool signature =
                name.ends_with(std::format("{}{}", plugin::SharedLibrary::getExtension(), plugin::kSignatureExtension));
            if (entry->is_directory()) {
                entry.disable_recursion_pending();
            }
            if (!entry->is_regular_file() || (!library && !signature)) {
                continue;
            }
        }
        if (entry->is_regular_file()) {
            copyFile(path, to / path.lexically_relative(from));
        }
    }
}

/// @return The table rows of the notices of @p file (a binary of the runtime); none if the runtime has none for it.
std::vector<std::string> noticesOf(const std::filesystem::path& runtime, const std::filesystem::path& file,
                                   bool& found) {
    std::ifstream in{runtime / kLicensesFolder / kByLibraryFolder / std::format("{}.md", file.filename().string())};
    found = static_cast<bool>(in);
    std::vector<std::string> rows;
    for (std::string line; std::getline(in, line);) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.starts_with('|')) {
            rows.push_back(std::move(line));
        }
    }
    return rows;
}

/// @return @p text as a JSON string, quotes included.
std::string jsonString(const std::string_view text) {
    std::string out = "\"";
    for (const char c : text) {
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    out += std::format("\\u{:04x}", static_cast<unsigned>(static_cast<unsigned char>(c)));
                } else {
                    out += c;
                }
        }
    }
    return out + "\"";
}

/// @return The SHA-256 of the file @p path, in lowercase hexadecimal.
std::string sha256(const std::filesystem::path& path) {
    std::ifstream in{path, std::ios::binary};
    crypto_hash_sha256_state state{};
    crypto_hash_sha256_init(&state);
    std::array<char, 1 << 16> buffer{};
    while (in) {
        in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto count = static_cast<std::size_t>(in.gcount());
        if (count > 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): libsodium reads bytes.
            crypto_hash_sha256_update(&state, reinterpret_cast<const unsigned char*>(buffer.data()), count);
        }
    }
    std::array<unsigned char, crypto_hash_sha256_BYTES> digest{};
    crypto_hash_sha256_final(&state, digest.data());
    std::string hex;
    for (const unsigned char byte : digest) {
        hex += std::format("{:02x}", byte);
    }
    return hex;
}

/// @return The folder of @p library in the export, relative to it.
std::filesystem::path destinationOf(const SelectedLibrary& library) {
    const std::filesystem::path plugins{Engine::kPluginsDirectory};
    switch (library.kind) {
        case LibraryKind::kPlugin:
            return plugins / library.file.filename();
        case LibraryKind::kBackend:
            return plugins / Engine::kBackendsDirectory / library.file.filename();
        case LibraryKind::kGame:
            return std::filesystem::path{kGameFolder} / Engine::kPluginsDirectory / library.file.filename();
    }
    return library.file.filename();
}

/// @brief Fills an export folder; removed by exportGame() if this throws.
class Exporter {
  public:
    Exporter(const ExportRequest& request, std::string name, std::filesystem::path folder, ExportReport& report)
        : _request{request}, _name{std::move(name)}, _folder{std::move(folder)}, _report{report} {}

    void run() {
        copyProgram();
        copyLib();
        for (const SelectedLibrary& library : _report.libraries) {
            if (library.kind != LibraryKind::kGame) {
                copyFile(library.file, _folder / destinationOf(library));
                _binaries.push_back({library.file, true});
            }
        }
        copyGame(_request.game, _folder / kGameFolder);
        writeNotices();
        writeLauncher();
        writeManifest();
    }

  private:
    /// @brief Copies the runtime's program under the game's name.
    void copyProgram() {
        const std::filesystem::path program = _request.runtime / runtimeProgramName();
        copyFile(program, _folder / std::format("{}{}", _name, kProgramExtension));
        _binaries.push_back({program, true});
    }

    /// @brief Copies what any plugin needs, whole: `lib/` (on Windows, the libraries next to the program) and `vulkan/`
    /// (the Vulkan driver's manifest the macOS runtime carries). A library made for the tests is left out.
    void copyLib() {
        const auto isTestLibrary = [](const std::filesystem::path& file) {
            return file.filename().string().find("test") != std::string::npos;
        };
        if (kWindows) {
            for (const std::filesystem::directory_entry& entry :
                 std::filesystem::directory_iterator{_request.runtime}) {
                if (entry.is_regular_file() && entry.path().extension() == plugin::SharedLibrary::getExtension() &&
                    !isTestLibrary(entry.path())) {
                    copyFile(entry.path(), _folder / entry.path().filename());
                    _binaries.push_back({entry.path(), false});
                }
            }
        }
        for (const std::string_view folder : {std::string_view{"lib"}, std::string_view{"vulkan"}}) {
            const std::filesystem::path from = _request.runtime / folder;
            std::error_code error;
            for (auto entry = std::filesystem::recursive_directory_iterator{from, error};
                 entry != std::filesystem::recursive_directory_iterator{}; entry.increment(error)) {
                if (entry->is_regular_file() && !isTestLibrary(entry->path())) {
                    copyFile(entry->path(), _folder / entry->path().lexically_relative(_request.runtime));
                    _binaries.push_back({entry->path(), false});
                }
            }
        }
    }

    /// @brief Writes the notices of the binaries kept, from those of the runtime.
    void writeNotices() {
        std::set<std::string> rows;
        std::vector<std::string> missing;
        for (const auto& [binary, required] : _binaries) {
            bool found = false;
            for (std::string& row : noticesOf(_request.runtime, binary, found)) {
                rows.insert(std::move(row));
            }
            if (!found && required) {
                missing.push_back(binary.filename().string());
            }
        }
        if (!missing.empty()) {
            std::string names;
            for (const std::string& name : missing) {
                names += std::format("{}{}", names.empty() ? "" : ", ", name);
            }
            _report.warnings.push_back(std::format(
                "The runtime has no notices for {} (licenses/by-library/): export from the engine's release zip to "
                "ship them",
                names));
        }
        const std::filesystem::path file = _folder / kLicensesFolder / kNoticesFile;
        std::filesystem::create_directories(file.parent_path());
        std::ofstream out{file};
        out << "# Third-party software in this game\n\n"
               "The game runs on the AYLO engine, which contains the libraries below. Each is under its own license; "
               "the license\ntexts are in the projects named here.\n\n"
               "| Library | Used for | License | Project |\n|---|---|---|---|\n";
        for (const std::string& row : rows) {
            out << row << '\n';
        }
    }

    /// @brief Writes the script that runs the game from its folder.
    void writeLauncher() const {
        if (kWindows) {
            std::ofstream{_folder / "play.bat"}
                << std::format("@echo off\r\ncd /d \"%~dp0\"\r\n\"{}.exe\" {} %*\r\n", _name, kGameFolder);
            return;
        }
        const std::filesystem::path script = _folder / "play.sh";
        std::ofstream{script} << std::format(
            "#!/bin/sh\n# Runs {0}; `./play.sh --server` runs its server.\n"
            "cd \"$(dirname \"$0\")\" || exit 1\nexec \"./{0}\" {1} \"$@\"\n",
            _name, kGameFolder);
        std::filesystem::permissions(script,
                                     std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec |
                                         std::filesystem::perms::others_exec,
                                     std::filesystem::perm_options::add);
    }

    /// @brief Writes `export.json`: the version, the libraries and why, and the hash of every other file.
    void writeManifest() {
        std::vector<std::filesystem::path> files;
        for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator{_folder}) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path());
            }
        }
        std::ranges::sort(files);
        std::string json = std::format("{{\n  \"name\": {},\n  \"engine\": {},\n  \"libraries\": [", jsonString(_name),
                                       jsonString(_request.engineVersion));
        for (std::size_t i = 0; i < _report.libraries.size(); ++i) {
            const SelectedLibrary& library = _report.libraries[i];
            json += std::format("{}\n    {{\"file\": {}, \"reasons\": [", i == 0 ? "" : ",",
                                jsonString(destinationOf(library).generic_string()));
            for (std::size_t r = 0; r < library.reasons.size(); ++r) {
                json += std::format("{}{}", r == 0 ? "" : ", ", jsonString(library.reasons[r]));
            }
            json += "]}";
        }
        json += "\n  ],\n  \"sha256\": {";
        for (std::size_t i = 0; i < files.size(); ++i) {
            _report.bytes += std::filesystem::file_size(files[i]);
            json += std::format("{}\n    {}: {}", i == 0 ? "" : ",",
                                jsonString(files[i].lexically_relative(_folder).generic_string()),
                                jsonString(sha256(files[i])));
        }
        json += "\n  }\n}\n";
        std::ofstream out{_folder / kExportFile};
        out << json;
        out.close();
        _report.bytes += std::filesystem::file_size(_folder / kExportFile);
    }

    const ExportRequest& _request;  ///< What to export.
    std::string _name;              ///< The game's name.
    std::filesystem::path _folder;  ///< The export.
    ExportReport& _report;          ///< What the export made.
    std::vector<std::pair<std::filesystem::path, bool>>
        _binaries;  ///< The files copied, for their notices; true: they must have some.
};
}  // namespace

std::string runtimeProgramName() { return std::format("aylo-runtime{}", kProgramExtension); }

ExportReport exportGame(const ExportRequest& request) {
    ExportRequest absolute = request;
    absolute.game = std::filesystem::absolute(request.game).lexically_normal();
    absolute.runtime = std::filesystem::absolute(request.runtime).lexically_normal();
    absolute.output = std::filesystem::absolute(request.output).lexically_normal();
    if (!absolute.game.has_filename()) {
        absolute.game = absolute.game.parent_path();
    }
    const std::string name = request.name.empty() ? defaultName(absolute.game) : request.name;
    checkName(name);
    if (!std::filesystem::is_regular_file(absolute.runtime / runtimeProgramName())) {
        throw exceptions::RuntimeError{
            std::format("No runtime in '{}': it has no {}", absolute.runtime.generic_string(), runtimeProgramName())};
    }
    if (sodium_init() < 0) {
        throw exceptions::RuntimeError{"Cannot start the hashes of the export"};
    }

    ExportReport report;
    report.folder = absolute.output / name;
    std::error_code error;
    if (std::filesystem::exists(report.folder, error) && !std::filesystem::is_empty(report.folder, error)) {
        throw exceptions::RuntimeError{std::format(
            "'{}' already exists and is not empty: remove it, or export elsewhere", report.folder.generic_string())};
    }
    report.libraries = selectLibraries({.game = absolute.game,
                                        .runtime = absolute.runtime,
                                        .debug = request.debug,
                                        .verifyGamePlugin = plugin::gamePluginVerifier()});
    std::filesystem::create_directories(report.folder);
    try {
        Exporter{absolute, name, report.folder, report}.run();
    } catch (...) {
        std::filesystem::remove_all(report.folder, error);
        throw;
    }
    return report;
}
}  // namespace rtype::engine::exporting
