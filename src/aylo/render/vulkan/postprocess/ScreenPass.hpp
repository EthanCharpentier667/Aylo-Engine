/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScreenPass
*/

#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/descriptor/DescriptorAllocator.hpp"
#include "render/vulkan/descriptor/DescriptorSetLayout.hpp"
#include "render/vulkan/frame/FrameGlobals.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Image.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/ShaderCache.hpp"
#include "render/vulkan/rendering/MaterialBinding.hpp"

namespace aylo::render::vulkan::postprocess {

/// @brief Runs the screen effects (game shaders of ShaderTarget::kScreen) over the floating-point scene, then copies
/// the result to where it is shown.
///
/// @details The scene is drawn into the first image of Targets instead of the window. Each frame then goes:
/// 1. recordEffects(), outside any rendering scope: each effect draws one fullscreen triangle into the other image,
///    reading the one the previous effect wrote (the scene, for the first) as its TEXTURE (set 1), so the two images
///    take turns;
/// 2. the renderer draws the overlay over the image the last effect wrote, so the effects leave it alone;
/// 3. recordCopy(), inside the rendering scope of the window or of a tool's scene texture: one fullscreen triangle that
///    copies that image there.
/// The effects are drawn with their material's pipeline (createMaterialPipeline()) and sets, so the pass allocates
/// nothing per frame; the copy reads its input through a combined image sampler (the renderer's texture set layout).
class AYLO_RENDER_VULKAN_API ScreenPass {
  public:
    /// @brief The two images of one size the effects take turns on, with the sets that sample them. Destroy it with the
    /// deletion queue, once the frames that may read it are done.
    struct Targets {
        std::array<memory::Image, 2> images;          ///< The first holds the scene; floating point.
        std::array<vk::raii::DescriptorSet, 2> sets;  ///< Samples images, by index.
    };

    /// @param textureSetLayout Set layout of one combined image sampler at binding 0: the copy's set 0.
    /// @param sampler Linear filtering, clamped to the edge: how the copy reads.
    /// @param sceneFormat Format of the images of Targets (floating point), and of a tool's scene texture.
    /// @param windowFormat Format of the window, the other image recordCopy() draws into.
    /// @throws vk::SystemError If a pipeline cannot be created.
    ScreenPass(const core::Device& device, pipeline::ShaderCache& shaders,
               const descriptor::DescriptorSetLayout& textureSetLayout, vk::Sampler sampler, vk::Format sceneFormat,
               vk::Format windowFormat, const vk::raii::PipelineCache* cache);
    ~ScreenPass() = default;
    ScreenPass(const ScreenPass&) = delete;
    ScreenPass& operator=(const ScreenPass&) = delete;
    ScreenPass(ScreenPass&&) = delete;
    ScreenPass& operator=(ScreenPass&&) = delete;

    /// @brief Builds the pipeline of a screen effect (a game shader's kScreen variant): no vertex buffer, the result
    /// replaces what the target holds (no blending). A material::GameShader::PipelineBuilder.
    [[nodiscard]] static pipeline::GraphicsPipeline createMaterialPipeline(
        const core::Device& device, const vk::raii::ShaderModule& vertex, const vk::raii::ShaderModule& fragment,
        bool instanceValues, const pipeline::PipelineLayout& layout, vk::Format colorFormat,
        const vk::raii::PipelineCache* cache);

    /// @return The images for a scene of @p extent pixels, in an undefined layout.
    /// @param descriptors Where the sets come from; must outlive them.
    /// @throws vk::SystemError If an image or its memory cannot be created.
    [[nodiscard]] std::unique_ptr<Targets> createTargets(const memory::Allocator& allocator,
                                                         descriptor::DescriptorAllocator& descriptors,
                                                         const descriptor::DescriptorSetLayout& textureSetLayout,
                                                         vk::Extent2D extent) const;

    /// @brief Records @p effects in their order, starting from the scene in the first image, which must be in
    /// SHADER_READ_ONLY_OPTIMAL. An effect with no pipeline is skipped. Outside a rendering scope.
    /// @param globals The frame's uniforms (set 0 of every effect), for @p frameIndex.
    /// @return The index of the image holding the result, left in SHADER_READ_ONLY_OPTIMAL: 0 with no effect.
    [[nodiscard]] std::size_t recordEffects(const vk::raii::CommandBuffer& commandBuffer, const Targets& targets,
                                            std::span<const rendering::MaterialBinding> effects,
                                            const frame::FrameGlobals& globals, std::size_t frameIndex) const;

    /// @brief Records the copy of @p source (a set of Targets) into the rendering scope already begun, over
    /// @p extent: into the window's format, or the scene's if @p toWindow is false.
    void recordCopy(const vk::raii::CommandBuffer& commandBuffer, vk::DescriptorSet source, vk::Extent2D extent,
                    bool toWindow) const;

  private:
    const core::Device* _device;           ///< Not owned.
    pipeline::PipelineLayout _layout;      ///< The copy's: one sampler (set 0).
    pipeline::GraphicsPipeline _toWindow;  ///< screen_copy, into the window's format.
    pipeline::GraphicsPipeline _toScene;   ///< screen_copy, into the scene's format.
    vk::Sampler _sampler;                  ///< Not owned.
    vk::Format _sceneFormat;               ///< Of the images of Targets.
};

}  // namespace aylo::render::vulkan::postprocess
