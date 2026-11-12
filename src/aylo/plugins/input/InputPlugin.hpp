/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** InputPlugin
*/

#pragma once

#include <string_view>

#include "aylo/ecs/app/Plugin.hpp"
#include "plugins/input/Export.hpp"

namespace aylo::ecs {
class App;
}  // namespace aylo::ecs

namespace aylo::plugins::input {
/// @brief Input for an App: the Input resource, kept up to date from the platform's events.
///
/// @details Inserts Input, registers the PlatformEvent ECS event and adds a PreUpdate client system,
/// "aylo.Input.Update", that feeds the frame's platform events to Input, in the order they happened, then calls
/// Input::update(). The events come from the GraphicsPlugin, which adds this plugin and runs before it; the InputPlugin
/// never talks to a window and knows nothing of the graphics plugin, so it also runs with events sent by hand (tests,
/// replays). Actions are declared on the resource, then read by the gameplay systems:
/// @code
/// app.addPlugin(aylo::plugins::input::InputPlugin{});
/// app.getWorld().getResources().requireMut<aylo::plugins::input::Input>()->addAction(
///     "fire", aylo::plugins::input::ActionType::kButton);
///
/// void fire(aylo::ecs::Res<aylo::plugins::input::Input> input) {
///     if (input->getAction("fire").isPressed()) { ... }
/// }
/// @endcode
class AYLO_PLUGIN_INPUT_API InputPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "aylo.Input";  ///< The plugin's name.
    /// @brief The PreUpdate system feeding the platform's events to Input. Producers of those events order their
    /// systems before it.
    static constexpr std::string_view kUpdateSystem = "aylo.Input.Update";

    InputPlugin() = default;
    ~InputPlugin() override = default;
    InputPlugin(const InputPlugin& other) = default;
    InputPlugin& operator=(const InputPlugin& other) = default;
    InputPlugin(InputPlugin&& other) noexcept = default;
    InputPlugin& operator=(InputPlugin&& other) noexcept = default;

    /// @return "aylo.Input".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @brief Inserts Input, registers PlatformEvent and adds the system that updates Input from it.
    void build(ecs::App& app) override;
};
}  // namespace aylo::plugins::input
