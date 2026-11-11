/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** InputPlugin
*/

#include "InputPlugin.hpp"

#include <string>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/event/Events.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/system/EventReader.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "plugins/input/Input.hpp"
#include "plugins/input/InputCalls.hpp"
#include "plugins/input/PlatformEvent.hpp"

namespace aylo::plugins::input {
namespace {
/// @brief Feeds the platform's events of the frame to Input, in the order they happened, then updates its actions.
void updateInput(ecs::ResMut<Input> input, ecs::EventReader<input::PlatformEvent> events) {
    Input& state = *input;
    for (const input::PlatformEvent& platformEvent : events) {
        state.handleEvent(platformEvent.event);
    }
    state.update();
}
}  // namespace

std::string_view InputPlugin::getName() const noexcept { return kName; }

void InputPlugin::build(ecs::App& app) {
    static_cast<void>(app.getWorld().getEvents().registerType<input::PlatformEvent>());
    app.insertResource(Input{});
    addInputCalls(app);
    app.addSystem(ecs::Phase::kPreUpdate, std::string{kUpdateSystem}, &updateInput).setSide(ecs::Side::kClient);
}
}  // namespace aylo::plugins::input
