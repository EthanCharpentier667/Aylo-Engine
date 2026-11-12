/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PostProcessPass
*/

#include "PostProcessPass.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "BloomPlan.hpp"
#include "aylo/contract/renderer/postprocess/PostProcess.hpp"
#include "bloom_down.frag.h"
#include "bloom_up.frag.h"
#include "postprocess_composite.frag.h"
#include "postprocess_fullscreen.vert.h"
#include "render/vulkan/command/CommandLabel.hpp"
#include "render/vulkan/descriptor/DescriptorWriter.hpp"
#include "render/vulkan/rendering/DynamicRendering.hpp"

namespace aylo::render::vulkan::postprocess {

namespace {

/// @brief Binding of the image a pass reads, in each of its sets.
constexpr std::uint32_t kSamplerBinding = 0;

/// @return The layout of every pass: two sets of one sampler (the composite reads two images) and the constants.
pipeline::PipelineLayout makeLayout(const core::Device& device, const descriptor::DescriptorSetLayout& setLayout) {
    const std::array setLayouts{*setLayout.getLayout(), *setLayout.getLayout()};
    const std::array pushConstants{vk::PushConstantRange{vk::ShaderStageFlagBits::eFragment, 0, sizeof(PostConstants)}};
    return pipeline::PipelineLayout{device, setLayouts, pushConstants};
}

pipeline::GraphicsPipeline makePipeline(const core::Device& device, pipeline::ShaderCache& shaders,
                                        const pipeline::PipelineLayout& layout, vk::Format format,
                                        pipeline::GraphicsPipeline::BlendMode blend, std::string_view fragmentName,
                                        std::span<const std::uint32_t> fragment, std::string_view debugName,
                                        const vk::raii::PipelineCache* cache) {
    pipeline::GraphicsPipeline built =
        pipeline::GraphicsPipeline::Builder{}
            .setShaders(shaders.get("postprocess_fullscreen.vert", aylo_shader_postprocess_fullscreen_vert),
                        shaders.get(fragmentName, fragment))
            .setBlend(blend)
            .setColorFormat(format)
            .setLayout(layout)
            .build(device, cache);
    device.setName(*built.getPipeline(), debugName);
    return built;
}

/// @return A sampled image of @p extent that can be drawn into, named @p name.
memory::Image makeImage(const core::Device& device, const memory::Allocator& allocator, vk::Extent2D extent,
                        vk::Format format, const std::string& name) {
    memory::Image image{device, allocator, extent, format,
                        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled};
    device.setName(*image.getImage(), name);
    device.setName(*image.getImageView(), name + " view");
    return image;
}

/// @return A set that samples @p image.
vk::raii::DescriptorSet makeSet(const core::Device& device, descriptor::DescriptorAllocator& descriptors,
                                const descriptor::DescriptorSetLayout& layout, const memory::Image& image,
                                vk::Sampler sampler, const std::string& name) {
    vk::raii::DescriptorSet set = descriptors.allocate(layout);
    descriptor::DescriptorWriter{}
        .writeImage(kSamplerBinding, *image.getImageView(), sampler, vk::ImageLayout::eShaderReadOnlyOptimal)
        .update(device, *set);
    device.setName(*set, name);
    return set;
}

glm::uvec2 sizeOf(const memory::Image& image) {
    const vk::Extent2D extent = image.getExtent();
    return glm::uvec2{extent.width, extent.height};
}

}  // namespace

PostProcessPass::PostProcessPass(const core::Device& device, pipeline::ShaderCache& shaders,
                                 const descriptor::DescriptorSetLayout& textureSetLayout, vk::Sampler sampler,
                                 vk::Format sceneFormat, vk::Format outputFormat, const vk::raii::PipelineCache* cache)
    : _device{&device},
      _layout{makeLayout(device, textureSetLayout)},
      _down{makePipeline(device, shaders, _layout, sceneFormat, pipeline::GraphicsPipeline::BlendMode::kOpaque,
                         "bloom_down.frag", aylo_shader_bloom_down_frag, "Bloom downsample pipeline", cache)},
      _up{makePipeline(device, shaders, _layout, sceneFormat, pipeline::GraphicsPipeline::BlendMode::kAdditive,
                       "bloom_up.frag", aylo_shader_bloom_up_frag, "Bloom upsample pipeline", cache)},
      _composite{makePipeline(device, shaders, _layout, outputFormat, pipeline::GraphicsPipeline::BlendMode::kOpaque,
                              "postprocess_composite.frag", aylo_shader_postprocess_composite_frag,
                              "Post-process composite pipeline", cache)},
      _sampler{sampler},
      _sceneFormat{sceneFormat} {
    device.setName(*_layout.getLayout(), "Post-process pipeline layout");
}

std::unique_ptr<PostProcessPass::Targets> PostProcessPass::createTargets(
    const memory::Allocator& allocator, descriptor::DescriptorAllocator& descriptors,
    const descriptor::DescriptorSetLayout& textureSetLayout, vk::Extent2D extent) const {
    memory::Image scene = makeImage(*_device, allocator, extent, _sceneFormat, "HDR scene");
    vk::raii::DescriptorSet sceneSet =
        makeSet(*_device, descriptors, textureSetLayout, scene, _sampler, "HDR scene set");
    auto targets = std::make_unique<Targets>(Targets{.scene = std::move(scene), .sceneSet = std::move(sceneSet)});

    const glm::uvec2 sceneSize{extent.width, extent.height};
    const std::uint32_t levels = bloomLevelCount(sceneSize);
    targets->bloom.reserve(levels);
    targets->bloomSets.reserve(levels);
    for (std::uint32_t level = 0; level < levels; ++level) {
        const glm::uvec2 size = bloomLevelSize(sceneSize, level);
        memory::Image image =
            makeImage(*_device, allocator, vk::Extent2D{size.x, size.y}, _sceneFormat, std::format("Bloom {}", level));
        targets->bloomSets.push_back(
            makeSet(*_device, descriptors, textureSetLayout, image, _sampler, std::format("Bloom {} set", level)));
        targets->bloom.push_back(std::move(image));
    }
    return targets;
}

void PostProcessPass::drawInto(const vk::raii::CommandBuffer& commandBuffer, const pipeline::GraphicsPipeline& pipeline,
                               const memory::Image& target, bool keepContents, vk::DescriptorSet source,
                               const PostConstants& constants) const {
    rendering::transitionImage(
        commandBuffer, *target.getImage(),
        keepContents ? rendering::kShaderReadToColorAttachment : rendering::kSceneToColorAttachment);
    const vk::RenderingAttachmentInfo attachment =
        keepContents ? rendering::loadedColorAttachment(*target.getImageView())
                     : rendering::colorAttachment(*target.getImageView(), vk::ClearColorValue{});
    commandBuffer.beginRendering(rendering::renderingInfo(target.getExtent(), {&attachment, 1}));
    const vk::Extent2D extent = target.getExtent();
    commandBuffer.setViewport(
        0, vk::Viewport{0.0F, 0.0F, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0F, 1.0F});
    commandBuffer.setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, extent});
    pipeline.bind(commandBuffer);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *_layout.getLayout(), 0, source, {});
    commandBuffer.pushConstants<PostConstants>(*_layout.getLayout(), vk::ShaderStageFlagBits::eFragment, 0, constants);
    commandBuffer.draw(3, 1, 0, 0);
    commandBuffer.endRendering();
    rendering::transitionImage(commandBuffer, *target.getImage(), rendering::kSceneToShaderRead);
}

void PostProcessPass::recordBloom(const vk::raii::CommandBuffer& commandBuffer, const Targets& targets,
                                  const contract::renderer::PostProcessSettings& settings) const {
    const std::size_t levels = targets.bloom.size();
    if (!settings.bloom.enabled || levels == 0) {
        return;
    }
    const command::CommandLabel label{*_device, commandBuffer, "Bloom"};
    // Down: the scene, filtered for what is bright, then each image halved.
    drawInto(commandBuffer, _down, targets.bloom.front(), false, *targets.sceneSet,
             makePrefilterConstants(sizeOf(targets.scene), settings.bloom));
    for (std::size_t level = 1; level < levels; ++level) {
        drawInto(commandBuffer, _down, targets.bloom.at(level), false, *targets.bloomSets.at(level - 1),
                 makeFilterConstants(sizeOf(targets.bloom.at(level - 1))));
    }
    // Up: each image added to the larger one, so the glow of every size ends up in the first.
    for (std::size_t level = levels - 1; level > 0; --level) {
        drawInto(commandBuffer, _up, targets.bloom.at(level - 1), true, *targets.bloomSets.at(level),
                 makeFilterConstants(sizeOf(targets.bloom.at(level))));
    }
}

void PostProcessPass::recordComposite(const vk::raii::CommandBuffer& commandBuffer, const Targets& targets,
                                      const contract::renderer::PostProcessSettings& settings,
                                      vk::Extent2D extent) const {
    const command::CommandLabel label{*_device, commandBuffer, "Composite"};
    const bool glow = settings.bloom.enabled && !targets.bloomSets.empty();
    // Without the glow the second sampler is not read (intensity 0), but it must still be a valid set.
    const std::array sets{*targets.sceneSet, glow ? *targets.bloomSets.front() : *targets.sceneSet};
    commandBuffer.setViewport(
        0, vk::Viewport{0.0F, 0.0F, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0F, 1.0F});
    commandBuffer.setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, extent});
    _composite.bind(commandBuffer);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *_layout.getLayout(), 0, sets, {});
    PostConstants constants = makeCompositeConstants(settings);
    if (!glow) {
        constants.intensity = 0.0F;
    }
    commandBuffer.pushConstants<PostConstants>(*_layout.getLayout(), vk::ShaderStageFlagBits::eFragment, 0, constants);
    commandBuffer.draw(3, 1, 0, 0);
}

}  // namespace aylo::render::vulkan::postprocess
