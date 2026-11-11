/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** JsonCalls
*/

#include "plugins/json/JsonCalls.hpp"

#include <array>
#include <string>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "plugins/json/detail/JsonCodec.hpp"

namespace aylo::plugins::json {

void addJsonForms(ecs::PluginRegistry& registry) {
    static constexpr auto kForms = std::to_array<char>({
#include "JsonForms.luau.h"
    });
    // bin2c ends the bytes with a null terminator, which is not part of the source.
    registry.addScriptForms("addJsonForms", std::string_view{kForms.data(), kForms.size() - 1});
}

void addJsonCalls(ecs::App& app) {
    addJsonForms(app.getPluginRegistry());
    app.addCall(std::string{kEncodeCall}, &detail::encode);
    app.addCall(std::string{kDecodeCall}, &detail::decode);
}

}  // namespace aylo::plugins::json
