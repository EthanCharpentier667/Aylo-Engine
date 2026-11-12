/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PostProcessPass
*/

#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "BloomPlan.hpp"
#include "aylo/contract/renderer/postprocess/PostProcess.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/descriptor/DescriptorAllocator.hpp"
#include "render/vulkan/descriptor/DescriptorSetLayout.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Image.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/ShaderCache.hpp"

namespace aylo::render::vulkan::postprocess {

/// @brief Brings the HDR scene to the window: bloom, then exposure and tonemap.
///
/// @details The scene is drawn into Targets::scene, a floating-point image, instead of the window. Each frame then
/// goes:
/// 1. recordBloom(), outside any rendering scope: a prefilter keeps what is brighter than the threshold while halving
///    the scene into the first bloom image, each next image is the previous one halved, then each is upsampled and
///    added to the larger one, so the glow of every size ends up in the first image;
/// 2. recordComposite(), inside the window's rendering scope: one fullscreen triangle that adds the glow to the scene,
///    applies the exposure and the tonemap.
/// The passes read their input through a combined image sampler (the renderer's texture set layout) and take their
/// parameters as push constants (PostConstants), so they allocate nothing per frame.
class AYLO_RENDER_VULKAN_API PostProcessPass {
  public:
    /// @brief The images of one window size: the scene and the bloom chain, with the sets that sample them. Destroy it
    /// with the deletion queue, once the frames that may read it are done.
    struct Targets {
        memory::Image scene;                             ///< The scene, drawn here, then read by the passes.
        vk::raii::DescriptorSet sceneSet;                ///< Samples scene.
        std::vector<memory::Image> bloom;                ///< bloomLevelCount() images, each half the previous one.
        std::vector<vk::raii::DescriptorSet> bloomSets;  ///< Samples bloom, by level.
    };

    /// @param textureSetLayout Set layout of one combined image sampler at binding 0: sets 0 and 1 of every pass.
    /// @param descriptors Where the sets of Targets come from; must outlive them.
    /// @param sampler Linear filtering, clamped to the edge: how every image is read.
    /// @param sceneFormat Format of the scene and the bloom images (floating point).
    /// @param outputFormat Format of the image recordComposite() draws into (the window's).
    /// @throws vk::SystemError If a pipeline cannot be created.
    PostProcessPass(const core::Device& device, pipeline::ShaderCache& shaders,
                    const descriptor::DescriptorSetLayout& textureSetLayout, vk::Sampler sampler,
                    vk::Format sceneFormat, vk::Format outputFormat, const vk::raii::PipelineCache* cache);
    ~PostProcessPass() = default;
    PostProcessPass(const PostProcessPass&) = delete;
    PostProcessPass& operator=(const PostProcessPass&) = delete;
    PostProcessPass(PostProcessPass&&) = delete;
    PostProcessPass& operator=(PostProcessPass&&) = delete;

    /// @return The images for a scene of @p extent pixels, in an undefined layout.
    /// @throws vk::SystemError If an image or its memory cannot be created.
    [[nodiscard]] std::unique_ptr<Targets> createTargets(const memory::Allocator& allocator,
                                                         descriptor::DescriptorAllocator& descriptors,
                                                         const descriptor::DescriptorSetLayout& textureSetLayout,
                                                         vk::Extent2D extent) const;

    /// @brief Records the bloom chain from the scene, which must be in SHADER_READ_ONLY_OPTIMAL. Leaves every bloom
    /// image in that layout. Does nothing if @p settings has bloom off. Outside a rendering scope.
    void recordBloom(const vk::raii::CommandBuffer& commandBuffer, const Targets& targets,
                     const contract::renderer::PostProcessSettings& settings) const;

    /// @brief Records the composite into the rendering scope already begun, over @p extent, from the scene and (when
    /// the bloom is on) the first bloom image, which recordBloom() left readable.
    void recordComposite(const vk::raii::CommandBuffer& commandBuffer, const Targets& targets,
                         const contract::renderer::PostProcessSettings& settings, vk::Extent2D extent) const;

  private:
    /// @brief Draws the fullscreen triangle of @p pipeline into @p target (the pass's rendering scope), reading
    /// @p source.
    void drawInto(const vk::raii::CommandBuffer& commandBuffer, const pipeline::GraphicsPipeline& pipeline,
                  const memory::Image& target, bool keepContents, vk::DescriptorSet source,
                  const PostConstants& constants) const;

    const core::Device* _device;            ///< Not owned.
    pipeline::PipelineLayout _layout;       ///< Two samplers (sets 0 and 1) and PostConstants.
    pipeline::GraphicsPipeline _down;       ///< bloom_down: replaces what the target holds.
    pipeline::GraphicsPipeline _up;         ///< bloom_up: adds to what the target holds.
    pipeline::GraphicsPipeline _composite;  ///< postprocess_composite: into the window's format.
    vk::Sampler _sampler;                   ///< Not owned.
    vk::Format _sceneFormat;                ///< Of the scene and the bloom images.
};

}  // namespace aylo::render::vulkan::postprocess
