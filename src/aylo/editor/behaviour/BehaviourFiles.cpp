/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** BehaviourFiles
*/

#include "BehaviourFiles.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/component/ComponentRegistry.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/schedule/Schedule.hpp"
#include "aylo/ecs/schedule/Schedules.hpp"

namespace aylo::editor::behaviour {
namespace {
/// @brief A hook of a Behaviour and the phase of its system.
struct Hook {
    std::string_view name;  ///< `onUpdate`...
    ecs::Phase phase;       ///< Where its system is.
};

constexpr std::array kHooks{
    Hook{.name = "onStart", .phase = ecs::Phase::kPreUpdate},
    Hook{.name = "onEnable", .phase = ecs::Phase::kPreUpdate},
    Hook{.name = "onDisable", .phase = ecs::Phase::kPreUpdate},
    Hook{.name = "onUpdate", .phase = ecs::Phase::kUpdate},
    Hook{.name = "onFixedUpdate", .phase = ecs::Phase::kFixedUpdate},
    Hook{.name = "onDestroy", .phase = ecs::Phase::kPostUpdate},
};

constexpr std::array<std::string_view, 2> kTags{".Started", ".Active"};

constexpr std::string_view kExtension = ".luau";

/// @brief The source of a new Behaviour, `BehaviourTemplate.luau` embedded as a byte array: a Luau file, edited as one.
constexpr auto kTemplateBytes = std::to_array<char>({
#include "BehaviourTemplate.luau.h"
});
constexpr std::string_view kTemplate{kTemplateBytes.data(), kTemplateBytes.size() - 1};

/// @brief What the template writes for the name of the Behaviour.
constexpr std::string_view kPlaceholder = "@Name@";

/// @return Whether @p name is a Luau name a Behaviour can have: a letter or `_`, then letters, digits and `_`.
[[nodiscard]] bool isName(std::string_view name) {
    const auto isStart = [](char letter) {
        return (letter >= 'a' && letter <= 'z') || (letter >= 'A' && letter <= 'Z') || letter == '_';
    };
    const auto isRest = [&isStart](char letter) { return isStart(letter) || (letter >= '0' && letter <= '9'); };
    return !name.empty() && isStart(name.front()) && std::ranges::all_of(name.substr(1), isRest);
}

/// @return The content of @p file; empty if it cannot be read.
[[nodiscard]] std::string readFile(const std::filesystem::path& file) {
    const std::ifstream stream{file, std::ios::binary};
    std::ostringstream content;
    content << stream.rdbuf();
    return std::move(content).str();
}
}  // namespace

std::filesystem::path behavioursFolder(const std::filesystem::path& assets) {
    return assets.parent_path() / "scripts" / "behaviours";
}

std::string makeTemplate(std::string_view name) {
    std::string source{kTemplate};
    for (std::size_t at = source.find(kPlaceholder); at != std::string::npos;
         at = source.find(kPlaceholder, at + name.size())) {
        source.replace(at, kPlaceholder.size(), name);
    }
    return source;
}

std::string createBehaviour(const std::filesystem::path& folder, std::string_view name, std::filesystem::path& file) {
    if (name.ends_with(kExtension)) {
        name.remove_suffix(kExtension.size());
    }
    if (!isName(name)) {
        return std::format("'{}' cannot name a Behaviour: use letters, digits and '_', not starting with a digit",
                           name);
    }
    file = folder / std::format("{}{}", name, kExtension);
    std::error_code error;
    if (std::filesystem::exists(file, error)) {
        return std::format("{} exists already", file.generic_string());
    }
    std::filesystem::create_directories(folder, error);
    std::ofstream stream{file, std::ios::binary};
    stream << makeTemplate(name);
    if (!stream) {
        return std::format("Could not write {}", file.generic_string());
    }
    return {};
}

std::optional<std::filesystem::path> findBehaviourFile(const std::filesystem::path& folder,
                                                       std::string_view component) {
    std::error_code error;
    std::vector<std::filesystem::path> files;
    for (auto entry = std::filesystem::recursive_directory_iterator{folder, error};
         !error && entry != std::filesystem::recursive_directory_iterator{}; entry.increment(error)) {
        if (entry->is_regular_file(error) && entry->path().extension() == kExtension) {
            files.push_back(entry->path());
        }
    }
    std::ranges::sort(files);
    const std::string wanted = std::format("engine.behaviour(\"{}\"", component);
    const auto found =
        std::ranges::find_if(files, [&wanted](const auto& file) { return readFile(file).contains(wanted); });
    return found == files.end() ? std::nullopt : std::optional{*found};
}

bool isBehaviour(const ecs::App& app, std::string_view component) {
    return std::ranges::any_of(kHooks, [&app, component](const Hook& hook) {
        return app.getSchedules().getSchedule(hook.phase).contains(std::format("{}.{}", component, hook.name));
    });
}

bool isBehaviourTag(const ecs::ComponentRegistry& registry, std::string_view component) {
    return std::ranges::any_of(kTags, [&registry, component](std::string_view tag) {
        return component.ends_with(tag) &&
               registry.findByName(component.substr(0, component.size() - tag.size())) != nullptr;
    });
}
}  // namespace aylo::editor::behaviour
