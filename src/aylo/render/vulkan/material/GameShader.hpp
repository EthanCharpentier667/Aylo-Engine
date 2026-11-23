/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** GameShader
*/

#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <string>
#include <utility>
#include <vulkan/vulkan_raii.hpp>

#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"

namespace aylo::render::vulkan::material {

/// @brief A game shader, behind a contract::renderer::ShaderId: the modules of each target it serves, and the
/// pipeline each one is drawn with, built the first time it is drawn.
///
/// @details A pipeline is built for one colour format: drawn into another one (a swapchain recreated in another
/// format), it is built again. The format only changes when the swapchain is recreated, after vkDeviceWaitIdle, so the
/// old pipeline is no longer in use and is destroyed at once.
/// @warning Destroy it before the Device.
class AYLO_RENDER_VULKAN_API GameShader {
  public:
    /// @brief Builds the pipeline of a target: its modules, whether they read instance values, the colour format, the
    /// layout and cache to use, and the render state (kMesh only; the 2D targets ignore it).
    using PipelineBuilder = pipeline::GraphicsPipeline (*)(const core::Device& device,
                                                           const vk::raii::ShaderModule& vertex,
                                                           const vk::raii::ShaderModule& fragment, bool instanceValues,
                                                           const pipeline::PipelineLayout& layout,
                                                           vk::Format colorFormat, const vk::raii::PipelineCache* cache,
                                                           const contract::renderer::RenderState& state);

    /// @brief Creates the modules of every variant of @p shader.
    /// @throws std::invalid_argument If @p shader is invalid (contract::renderer::validate()).
    /// @throws exceptions::ShaderException If a stage is not SPIR-V.
    /// @throws vk::SystemError If a module cannot be created.
    GameShader(const core::Device& device, const contract::renderer::CompiledShader& shader);
    ~GameShader() = default;
    GameShader(const GameShader&) = delete;
    GameShader& operator=(const GameShader&) = delete;
    GameShader(GameShader&&) noexcept = default;
    GameShader& operator=(GameShader&&) noexcept = default;

    /// @return The shader's name, for messages.
    [[nodiscard]] const std::string& getName() const noexcept { return _name; }
    /// @return The bytes of its material block (0: none).
    [[nodiscard]] std::uint32_t getMaterialSize() const noexcept { return _materialSize; }
    /// @return Whether it declares instance values: only then do its sprites carry them.
    [[nodiscard]] bool hasInstanceValues() const noexcept { return _instanceValues; }
    /// @return Whether its screen effect reads the 3D depth.
    [[nodiscard]] bool readsDepth() const noexcept { return _readsDepth; }

    /// @return Whether it serves @p target.
    [[nodiscard]] bool serves(contract::renderer::ShaderTarget target) const noexcept;

    /// @brief Builds a shadow map pipeline of the kMesh variant: its vertex module, its fragment module (used when the
    /// shader cuts out), whether it cuts out, and the faces culled.
    using ShadowPipelineBuilder = std::function<pipeline::GraphicsPipeline(
        const vk::raii::ShaderModule& vertex, const vk::raii::ShaderModule& fragment, bool cutsOut,
        contract::renderer::CullMode cull)>;

    /// @return The pipeline that draws its kMesh variant into a shadow map culling @p cull, built by @p build the first
    /// time; nullptr if it draws no mesh or says `#pragma shadow off`.
    /// @throws vk::SystemError If the pipeline cannot be built.
    [[nodiscard]] const pipeline::GraphicsPipeline* getShadowPipeline(const core::Device& device,
                                                                      contract::renderer::CullMode cull,
                                                                      const ShadowPipelineBuilder& build);

    /// @return The shader's own render state (its pragmas).
    [[nodiscard]] const contract::renderer::RenderState& getRenderState() const noexcept { return _renderState; }

    /// @return The pipeline drawing @p target into @p colorFormat with @p state, built by @p build the first time (or
    /// after a format change), or nullptr if the shader does not serve @p target. A target keeps one pipeline per
    /// render state its materials ask for (a 2D target only ever has the default one).
    /// @throws vk::SystemError If the pipeline cannot be built.
    [[nodiscard]] const pipeline::GraphicsPipeline* getPipeline(
        contract::renderer::ShaderTarget target, const core::Device& device, const pipeline::PipelineLayout& layout,
        vk::Format colorFormat, const vk::raii::PipelineCache* cache, PipelineBuilder build,
        const contract::renderer::RenderState& state = {});

  private:
    /// @brief What the shader draws one target with.
    struct Variant {
        vk::raii::ShaderModule vertex{nullptr};    ///< Null if the shader does not serve the target.
        vk::raii::ShaderModule fragment{nullptr};  ///< Null if the shader does not serve the target.
        /// @brief The pipelines built for `format`, by the first draw with each render state. A deque: building one
        /// keeps the others where they are, as the frame's draws point to them.
        std::deque<std::pair<contract::renderer::RenderState, pipeline::GraphicsPipeline>> built;
        vk::Format format{vk::Format::eUndefined};  ///< The colour format `built` draws into.
        /// The shadow map pipelines of a kMesh variant, by the faces they cull.
        std::deque<std::pair<contract::renderer::CullMode, pipeline::GraphicsPipeline>> shadows;
    };

    std::string _name;                                                      ///< For messages.
    std::uint32_t _materialSize{0};                                         ///< Its material block.
    bool _instanceValues{false};                                            ///< It has `instance uniform`s.
    contract::renderer::RenderState _renderState;                           ///< Its pragmas.
    bool _castsShadows{true};                                               ///< No `#pragma shadow off`.
    bool _cutsOut{false};                                                   ///< It writes ALPHA_SCISSOR.
    bool _readsDepth{false};                                                ///< Its screen effect reads depth.
    std::array<Variant, contract::renderer::kShaderTargetCount> _variants;  ///< By ShaderTarget.
};

}  // namespace aylo::render::vulkan::material
