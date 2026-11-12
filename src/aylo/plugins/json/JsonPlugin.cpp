/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** JsonPlugin
*/

#include "plugins/json/JsonPlugin.hpp"

#include <span>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "plugins/json/JsonCalls.hpp"

namespace aylo::plugins::json {

std::string_view JsonPlugin::getName() const noexcept { return kName; }

std::span<const std::string_view> JsonPlugin::getDependencies() const noexcept { return {}; }

void JsonPlugin::build(ecs::App& app) { addJsonCalls(app); }

}  // namespace aylo::plugins::json
