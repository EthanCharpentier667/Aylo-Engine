/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScreenPass
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vulkan/vulkan_raii.hpp>

#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
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

/// @brief The set of the 3D depth (DEPTH_TEXTURE) in the screen effects' layout.
inline constexpr std::uint32_t kScreenDepthSet = 3;

/// @brief Runs the screen effects (game shaders of ShaderTarget::kScreen) over the floating-point scene, then copies
/// the result to where it is shown.
///
/// @details The scene is drawn into the first image of Targets instead of the window. Each frame then goes:
/// 1. recordPass() for each pass, outside any rendering scope: each effect draws one fullscreen triangle into its
/// target,
///    reading the one the previous effect wrote (the scene, for the first) as its TEXTURE (set 1), so the two images
///    take turns;
/// 2. the renderer draws the overlay over the image the last effect wrote, so the effects leave it alone;
/// 3. recordCopy(), inside the rendering scope of the window or of a tool's scene texture: one fullscreen triangle that
///    copies that image there.
/// The effects are drawn with their material's pipeline (createMaterialPipeline()) and sets, so the pass allocates
/// nothing per frame; the copy reads its input through a combined image sampler (the renderer's texture set layout).
class AYLO_RENDER_VULKAN_API ScreenPass {
  public:
    /// @brief The images of one size the effects draw the chain on, with the sets that sample them: the first holds the
    /// scene and is never written by an effect (SCENE_TEXTURE), the two others take turns. Destroy it with the deletion
    /// queue, once the frames that may read it are done.
    struct Targets {
        std::array<memory::Image, 3> images;          ///< Floating point, of the picture's size.
        std::array<vk::raii::DescriptorSet, 3> sets;  ///< Samples images, by index.
    };

    /// @brief One pass to record: what it draws with, into and from.
    struct Pass {
        const pipeline::GraphicsPipeline* pipeline = nullptr;  ///< Its material's kScreen pipeline, for its format.
        vk::Image image;                                       ///< The target it writes.
        vk::ImageView view;                                    ///< Its view.
        vk::Extent2D extent;                                   ///< Its size.
        vk::DescriptorSet input;                               ///< Set 1: what it reads as TEXTURE.
        vk::DescriptorSet material;                            ///< Set 2: its material (or a copy with targets).
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
        const vk::raii::PipelineCache* cache, const contract::renderer::RenderState& state);

    /// @return The images for a scene of @p extent pixels, in an undefined layout.
    /// @param descriptors Where the sets come from; must outlive them.
    /// @throws vk::SystemError If an image or its memory cannot be created.
    [[nodiscard]] std::unique_ptr<Targets> createTargets(const memory::Allocator& allocator,
                                                         descriptor::DescriptorAllocator& descriptors,
                                                         const descriptor::DescriptorSetLayout& textureSetLayout,
                                                         vk::Extent2D extent) const;

    /// @brief Records @p pass outside a rendering scope: its target, made a colour attachment, is drawn over by one
    /// fullscreen triangle, then left in SHADER_READ_ONLY_OPTIMAL for the passes after it.
    /// @param globals The frame's uniforms (set 0), for @p frameIndex: the 2D view and the 3D camera.
    /// @param screenSet Set 3: DEPTH_TEXTURE, SCENE_TEXTURE and PREVIOUS_FRAME.
    void recordPass(const vk::raii::CommandBuffer& commandBuffer, const Pass& pass, const frame::FrameGlobals& globals,
                    std::size_t frameIndex, vk::DescriptorSet screenSet) const;

    /// @return An image of @p extent and @p format, drawn into then sampled, named @p name, and @p set (allocated
    /// from @p descriptors with @p textureSetLayout) pointing at it: a named target of the effects.
    [[nodiscard]] memory::Image createImage(const memory::Allocator& allocator, vk::Extent2D extent, vk::Format format,
                                            const std::string& name) const;

    /// @brief Points @p set (of the texture set layout) at @p image, read through the pass's sampler.
    void writeSet(vk::DescriptorSet set, const memory::Image& image) const;

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
