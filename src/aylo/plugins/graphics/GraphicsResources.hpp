/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** GraphicsResources
*/

#pragma once

#include <memory>
#include <optional>

#include "aylo/contract/renderer/drawing/Color.hpp"
#include "aylo/contract/renderer/postprocess/PostProcess.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "core/config/Settings.hpp"
#include "plugins/graphics/BackendRegistry.hpp"

namespace aylo::plugins::graphics {
// The started platform and renderer, as a main-thread resource: GLFW and Vulkan must be used from the main thread.
AYLO_ECS_RESOURCE(Backend, "aylo.Backend");

/// @brief The colour each frame is cleared with.
struct ClearColor {
    contract::renderer::Color value{0.05F, 0.05F, 0.15F, 1.0F};  ///< RGBA, each in [0, 1].
};
AYLO_ECS_RESOURCE(ClearColor, "aylo.ClearColor");

/// @brief What happens to the frame between the scene and the window: bloom, exposure and tonemap. Scripts set it with
/// `aylo.backend.setPostProcess`; the GraphicsPlugin hands it to the renderer when it changes. The default changes
/// nothing of the picture.
struct PostProcess {
    contract::renderer::PostProcessSettings settings;                ///< What the game asks for.
    std::optional<contract::renderer::PostProcessSettings> applied;  ///< What the renderer was last given.
};
AYLO_ECS_RESOURCE(PostProcess, "aylo.PostProcess");

/// @brief What the Startup system needs to create the Backend: the registered platforms and renderers, and the
/// configuration naming the pair to start. Inserted by GraphicsPlugin::build(), which may not open a window itself.
struct BackendSetup {
    std::unique_ptr<plugins::graphics::BackendRegistry> registry;  ///< The platforms and renderers by name.
    core::config::Settings settings;                               ///< `platform`, `renderer`, `window`...
};
AYLO_ECS_RESOURCE(BackendSetup, "aylo.BackendSetup");
}  // namespace aylo::plugins::graphics
