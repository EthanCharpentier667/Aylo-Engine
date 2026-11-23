/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ShaderIncludes
*/

#include "plugins/shader/detail/ShaderIncludes.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "plugins/shader/BuiltinShaders.hpp"
#include "plugins/shader/exceptions/ShaderExceptions.hpp"

namespace aylo::plugins::shader::detail {

namespace {

/// @return @p line without its leading and trailing spaces.
std::string_view trim(std::string_view line) {
    const std::size_t first = line.find_first_not_of(" \t\r");
    if (first == std::string_view::npos) {
        return {};
    }
    return line.substr(first, line.find_last_not_of(" \t\r") - first + 1);
}

/// @brief Expands one file after another into an ExpandedShader.
class Expander {
  public:
    Expander(ExpandedShader& expanded, const IncludeReader& read) : _expanded{&expanded}, _read{&read} {}

    /// @brief Appends @p source, the shader, expanding its includes: an included file is read to its end, then the file
    /// that included it goes on, as a stack of files being read rather than a recursion.
    void expand(std::string_view source) {
        _frames.push_back(Frame{.source = std::string{source}});
        while (!_frames.empty()) {
            Frame& frame = _frames.back();
            if (frame.position > frame.source.size()) {
                _frames.pop_back();
                continue;
            }
            const std::size_t end = std::min(frame.source.find('\n', frame.position), frame.source.size());
            const std::string line = frame.source.substr(frame.position, end - frame.position);
            frame.position = end + 1;
            const std::uint32_t file = frame.file;
            const std::uint32_t lineNumber = ++frame.line;
            const std::string_view trimmed = trim(line);
            if (trimmed.starts_with("#include")) {
                append({}, file, lineNumber);
                include(trimmed, file, lineNumber);  // may push a frame: `frame` is not used after it
            } else {
                append(line, file, lineNumber);
            }
        }
    }

  private:
    /// @brief Appends @p line, written at line @p lineNumber of @p file.
    void append(std::string_view line, std::uint32_t file, std::uint32_t lineNumber) {
        _expanded->text.append(line);
        _expanded->text.push_back('\n');
        _expanded->lines.push_back(ExpandedShader::Origin{.file = file, .line = lineNumber});
    }

    [[noreturn]] void fail(std::uint32_t file, std::uint32_t lineNumber, std::string_view problem) const {
        const std::string where = file == 0 ? std::format("line {}", lineNumber)
                                            : std::format("line {} of '{}'", lineNumber, _expanded->files.at(file));
        throw exceptions::ShaderException{std::format("Shader '{}', {}: {}", _expanded->files.front(), where, problem)};
    }

    /// @brief Expands the `#include` @p directive of @p file at @p lineNumber.
    void include(std::string_view directive, std::uint32_t file, std::uint32_t lineNumber) {
        const std::string_view rest = trim(directive.substr(std::string_view{"#include"}.size()));
        if (rest.size() < 2 || rest.front() != '"' || rest.find('"', 1) != rest.size() - 1) {
            fail(file, lineNumber, "expected #include \"path\"");
        }
        const std::string path{rest.substr(1, rest.size() - 2)};
        auto [name, source] = find(path, _expanded->files.at(file), file, lineNumber);
        const auto known = std::ranges::find(_expanded->files, name);
        if (known != _expanded->files.end()) {
            const auto index = static_cast<std::uint32_t>(known - _expanded->files.begin());
            if (std::ranges::any_of(_frames, [index](const Frame& open) { return open.file == index; })) {
                fail(file, lineNumber, std::format("'{}' includes itself", name));
            }
            return;  // each file once
        }
        if (_frames.size() >= kMaxIncludeDepth) {
            fail(file, lineNumber, std::format("more than {} levels of #include", kMaxIncludeDepth));
        }
        _expanded->files.push_back(name);
        _frames.push_back(
            Frame{.source = std::move(source), .file = static_cast<std::uint32_t>(_expanded->files.size() - 1)});
    }

    /// @return The name and text of the file @p path names, included from @p from.
    [[nodiscard]] std::pair<std::string, std::string> find(const std::string& path, const std::string& from,
                                                           std::uint32_t file, std::uint32_t lineNumber) const {
        if (path.starts_with(kBuiltinShaderPrefix)) {
            if (const std::optional<std::string_view> library = findBuiltinLibrary(path)) {
                return {path, std::string{*library}};
            }
            fail(file, lineNumber, std::format("the engine has no library '{}' ({})", path, kBuiltinLibraryNames));
        }
        if (from.starts_with(kBuiltinShaderPrefix)) {
            fail(file, lineNumber,
                 std::format("an engine library includes only libraries of the engine, not '{}'", path));
        }
        std::vector<std::string> candidates;
        const std::filesystem::path folder = std::filesystem::path{from}.parent_path();
        if (!folder.empty()) {
            candidates.push_back((folder / path).lexically_normal().generic_string());
        }
        candidates.push_back(std::filesystem::path{path}.lexically_normal().generic_string());
        if (*_read) {
            for (const std::string& candidate : candidates) {
                if (std::optional<std::string> text = (*_read)(candidate)) {
                    return {candidate, std::move(*text)};
                }
            }
        }
        std::string looked;
        for (const std::string& candidate : candidates) {
            looked += std::format("{}'{}'", looked.empty() ? "" : ", ", candidate);
        }
        fail(file, lineNumber, std::format("cannot find \"{}\" (looked for {})", path, looked));
    }

    /// @brief A file being read.
    struct Frame {
        std::string source;       ///< Its text.
        std::uint32_t file{0};    ///< Its index in ExpandedShader::files.
        std::size_t position{0};  ///< Where its next line starts.
        std::uint32_t line{0};    ///< The number of its last line read.
    };

    ExpandedShader* _expanded;   ///< What is being built.
    const IncludeReader* _read;  ///< Reads the game's files.
    std::vector<Frame> _frames;  ///< The files being read, the shader first, the one read now last.
};

}  // namespace

std::string ExpandedShader::locate(std::uint32_t line) const {
    if (line == 0 || line > lines.size()) {
        return std::format("line {}", line);
    }
    const Origin& origin = lines.at(line - 1);
    return origin.file == 0 ? std::format("line {}", origin.line)
                            : std::format("line {} of '{}'", origin.line, files.at(origin.file));
}

std::string ExpandedShader::locateLines(std::string_view message) const {
    std::string located;
    constexpr std::string_view kLine = "line ";
    for (std::size_t start = 0; start < message.size();) {
        const std::size_t found = message.find(kLine, start);
        if (found == std::string_view::npos) {
            located.append(message.substr(start));
            break;
        }
        located.append(message.substr(start, found - start));
        std::size_t digits = found + kLine.size();
        std::uint32_t number = 0;
        while (digits < message.size() && message.at(digits) >= '0' && message.at(digits) <= '9') {
            number = (number * 10) + static_cast<std::uint32_t>(message.at(digits) - '0');
            ++digits;
        }
        if (digits > found + kLine.size() && digits < message.size() && message.at(digits) == ':') {
            located.append(locate(number));
            start = digits;
        } else {
            located.append(kLine);
            start = found + kLine.size();
        }
    }
    return located;
}

ExpandedShader expandIncludes(std::string_view source, std::string_view name, const IncludeReader& read) {
    ExpandedShader expanded;
    expanded.files.emplace_back(name);
    Expander{expanded, read}.expand(source);
    if (!expanded.text.empty()) {
        expanded.text.pop_back();  // the last line ends as the source did
    }
    return expanded;
}

}  // namespace aylo::plugins::shader::detail
