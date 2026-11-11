/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** UiPlugin
*/

#include "plugins/ui/UiPlugin.hpp"

#include <array>
#include <span>
#include <string>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/event/Events.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/text/TextPlugin.hpp"
#include "plugins/ui/UiComponents.hpp"
#include "plugins/ui/UiEvents.hpp"
#include "plugins/ui/UiNode.hpp"
#include "plugins/ui/UiResources.hpp"
#include "plugins/ui/UiWidgets.hpp"
#include "plugins/ui/detail/UiSystems.hpp"

namespace aylo::plugins::ui {

void addUiForms(ecs::PluginRegistry& registry) {
    static constexpr auto kForms = std::to_array<char>({
#include "UiForms.luau.h"
    });
    // bin2c ends the bytes with a null terminator, which is not part of the source.
    registry.addScriptForms("addUiForms", std::string_view{kForms.data(), kForms.size() - 1});
}

std::string_view UiPlugin::getName() const noexcept { return kName; }

std::span<const std::string_view> UiPlugin::getDependencies() const noexcept {
    static constexpr std::array<std::string_view, 3> kDependencies{graphics::GraphicsPlugin::kName,
                                                                   input::InputPlugin::kName, text::TextPlugin::kName};
    return kDependencies;
}

void UiPlugin::build(ecs::App& app) {
    ecs::Components& components = app.getWorld().getComponents();
    static_cast<void>(components.registerType<UiNode>());
    static_cast<void>(components.registerType<UiComputed>());
    static_cast<void>(components.registerType<UiBackground>());
    static_cast<void>(components.registerType<UiImage>());
    static_cast<void>(components.registerType<UiText>());
    static_cast<void>(components.registerType<UiInteraction>());
    static_cast<void>(components.registerType<UiButton>());
    static_cast<void>(components.registerType<UiToggle>());
    static_cast<void>(components.registerType<UiSlider>());
    static_cast<void>(components.registerType<UiScroll>());
    static_cast<void>(components.registerType<UiTextInput>());
    ecs::Events& events = app.getWorld().getEvents();
    static_cast<void>(events.registerType<UiClicked>());
    static_cast<void>(events.registerType<UiValueChanged>());
    static_cast<void>(events.registerType<UiTextChanged>());
    static_cast<void>(events.registerType<UiTextSubmitted>());
    ecs::Resources& resources = app.getWorld().getResources();
    resources.insertIfAbsent<UiPointer>();
    resources.insertIfAbsent<UiFocus>();
    addUiForms(app.getPluginRegistry());

    const auto preUpdate = [&app](const std::string_view name, auto system, const std::string_view after) {
        app.addSystem(ecs::Phase::kPreUpdate, std::string{name}, system)
            .after(std::string{after})
            .setSide(ecs::Side::kClient);
    };
    preUpdate("aylo.ui.pick", &detail::pickNodes, input::InputPlugin::kUpdateSystem);
    preUpdate("aylo.ui.navigate", &detail::navigateFocus, "aylo.ui.pick");
    preUpdate("aylo.ui.interact", &detail::applyInteraction, "aylo.ui.navigate");
    preUpdate("aylo.ui.buttons", &detail::updateButtons, "aylo.ui.interact");
    preUpdate("aylo.ui.toggles", &detail::updateToggles, "aylo.ui.buttons");
    preUpdate("aylo.ui.sliders", &detail::updateSliders, "aylo.ui.toggles");
    preUpdate("aylo.ui.scrolls", &detail::updateScrolls, "aylo.ui.sliders");
    preUpdate("aylo.ui.inputs", &detail::updateTextInputs, "aylo.ui.scrolls");
    app.addSystem(ecs::Phase::kRender, std::string{kLayoutSystem}, &detail::layoutUi)
        .after(std::string{graphics::GraphicsPlugin::kCameraSystem})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, std::string{kDrawSystem}, &detail::drawUi)
        .after(std::string{kLayoutSystem})
        .before(std::string{graphics::GraphicsPlugin::kEndFrameSystem})
        .setSide(ecs::Side::kClient);
}

}  // namespace aylo::plugins::ui
