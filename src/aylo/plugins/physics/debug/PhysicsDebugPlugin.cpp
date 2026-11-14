/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PhysicsDebugPlugin
*/

#include "plugins/physics/debug/PhysicsDebugPlugin.hpp"

#include <array>
#include <span>
#include <string>
#include <string_view>

#include "aylo/contract/platform/event/Event.hpp"
#include "aylo/contract/platform/input/Key.hpp"
#include "aylo/contract/renderer/IRenderer.hpp"
#include "aylo/contract/renderer/drawing/Color.hpp"
#include "aylo/contract/renderer/drawing/Shape.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/event/Events.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/query/QueryTerms.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/system/EventReader.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "plugins/graphics/BackendRegistry.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/graphics/GraphicsResources.hpp"  // IWYU pragma: keep
#include "plugins/input/InputEvents.hpp"           // IWYU pragma: keep
#include "plugins/input/InputPlugin.hpp"
#include "plugins/physics/Collider.hpp"
#include "plugins/physics/ComponentOptions.hpp"
#include "plugins/physics/Velocity.hpp"
#include "plugins/physics/debug/PhysicsDebug.hpp"
#include "plugins/physics/debug/PhysicsDebugSettings.hpp"
#include "plugins/physics/debug/PhysicsDebugShapes.hpp"
#include "plugins/shape/ShapePlugin.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"

namespace aylo::plugins::physics::debug {
namespace {
/// @brief Update: flips PhysicsDebugSettings::enabled when the toggle key is pressed (not on auto-repeat).
void toggleDebug(ecs::ResMut<PhysicsDebugSettings> settings,
                 ecs::EventReader<contract::platform::event::KeyPressed> pressed) {
    const contract::platform::input::Key toggleKey = settings.get().toggleKey;
    for (const contract::platform::event::KeyPressed& event : pressed) {
        if (toggleKey != contract::platform::input::Key::kUnknown && event.key == toggleKey && !event.repeat) {
            settings->enabled = !settings.get().enabled;
        }
    }
}

/// @return The colour an entity is drawn in, or nothing when it is not drawn: PhysicsDebug chooses it, drawAll draws
/// the others in the settings' colour.
const contract::renderer::Color* colorOf(const PhysicsDebug* debug, bool wanted, const PhysicsDebugSettings& settings) {
    if (debug != nullptr) {
        return wanted || settings.drawAll ? &debug->color : nullptr;
    }
    return settings.drawAll ? &settings.color : nullptr;
}

/// @brief Render, after the sprites: draws the chosen entities' collider outlines and velocity arrows.
void drawDebug(
    ecs::NonSendMut<plugins::graphics::Backend> backend, ecs::Res<PhysicsDebugSettings> settings,
    ecs::Query<const Collider, const transform::GlobalTransform2D, ecs::Optional<const PhysicsDebug>> colliders,
    ecs::Query<const Velocity, const transform::GlobalTransform2D, ecs::Optional<const PhysicsDebug>> moving) {
    if (!settings->enabled || backend->renderer == nullptr) {
        return;
    }
    contract::renderer::IRenderer& renderer = *backend->renderer;
    for (auto [entity, collider, transform, debug] : colliders) {
        if (const contract::renderer::Color* color = colorOf(debug, debug != nullptr && debug->shape, *settings)) {
            renderer.draw(debugColliderShape(collider, transform, *color, settings->outlineThickness));
        }
    }
    for (auto [entity, velocity, transform, debug] : moving) {
        if (const contract::renderer::Color* color = colorOf(debug, debug != nullptr && debug->velocity, *settings)) {
            for (const contract::renderer::Shape& stroke :
                 debugVelocityArrow(transform.position, velocity.linear, settings->velocityScale, *color,
                                    settings->outlineThickness)) {
                renderer.draw(stroke);
            }
        }
    }
}
}  // namespace

std::string_view PhysicsDebugPlugin::getName() const noexcept { return kName; }

std::span<const std::string_view> PhysicsDebugPlugin::getDependencies() const noexcept {
    static constexpr std::array<std::string_view, 4> kDependencies{graphics::GraphicsPlugin::kName,
                                                                   input::InputPlugin::kName, shape::ShapePlugin::kName,
                                                                   transform::TransformPlugin::kName};
    return kDependencies;
}

PhysicsDebugPlugin::PhysicsDebugPlugin(PhysicsDebugSettings settings) : _settings{settings} {}

void PhysicsDebugPlugin::build(ecs::App& app) {
    static_cast<void>(app.getWorld().getComponents().registerType<Collider>(kColliderOptions));
    static_cast<void>(app.getWorld().getComponents().registerType<Velocity>());
    static_cast<void>(app.getWorld().getComponents().registerType<PhysicsDebug>());
    static_cast<void>(app.getWorld().getEvents().registerType<contract::platform::event::KeyPressed>());
    ecs::Resources& resources = app.getWorld().getResources();
    if (_settings) {
        resources.insert(*_settings);
    } else {
        resources.insertIfAbsent<PhysicsDebugSettings>();
    }
    app.addSystem(ecs::Phase::kUpdate, std::string{kToggleSystem}, &toggleDebug).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, std::string{kDrawSystem}, &drawDebug)
        .after(std::string{shape::ShapePlugin::kDrawSystem})
        .before(std::string{graphics::GraphicsPlugin::kEndFrameSystem})
        .setSide(ecs::Side::kClient);
}

}  // namespace aylo::plugins::physics::debug
