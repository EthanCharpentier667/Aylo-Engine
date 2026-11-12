/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScreenPass
*/

#include "ScreenPass.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/command/CommandLabel.hpp"
#include "render/vulkan/descriptor/DescriptorWriter.hpp"
#include "render/vulkan/rendering/DynamicRendering.hpp"
#include "screen_copy.frag.h"
#include "screen_fullscreen.vert.h"

namespace aylo::render::vulkan::postprocess {

namespace {

/// @brief Binding of the image the copy reads, in its set.
constexpr std::uint32_t kSamplerBinding = 0;
/// @brief The set of a game shader that holds the image it reads: its drawing's texture.
constexpr std::uint32_t kTextureSet = 1;
/// @brief The set of a game shader that holds its material.
constexpr std::uint32_t kMaterialSet = 2;

/// @return The copy's pipeline into @p format, named @p debugName.
pipeline::GraphicsPipeline makeCopyPipeline(const core::Device& device, pipeline::ShaderCache& shaders,
                                            const pipeline::PipelineLayout& layout, vk::Format format,
                                            std::string_view debugName, const vk::raii::PipelineCache* cache) {
    pipeline::GraphicsPipeline built =
        pipeline::GraphicsPipeline::Builder{}
            .setShaders(shaders.get("screen_fullscreen.vert", aylo_shader_screen_fullscreen_vert),
                        shaders.get("screen_copy.frag", aylo_shader_screen_copy_frag))
            .setBlend(pipeline::GraphicsPipeline::BlendMode::kOpaque)
            .setColorFormat(format)
            .setLayout(layout)
            .build(device, cache);
    device.setName(*built.getPipeline(), debugName);
    return built;
}

/// @brief Sets the viewport and the scissor to the whole of @p extent.
void coverAll(const vk::raii::CommandBuffer& commandBuffer, vk::Extent2D extent) {
    commandBuffer.setViewport(
        0, vk::Viewport{0.0F, 0.0F, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0F, 1.0F});
    commandBuffer.setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, extent});
}

}  // namespace

ScreenPass::ScreenPass(const core::Device& device, pipeline::ShaderCache& shaders,
                       const descriptor::DescriptorSetLayout& textureSetLayout, vk::Sampler sampler,
                       vk::Format sceneFormat, vk::Format windowFormat, const vk::raii::PipelineCache* cache)
    : _device{&device},
      _layout{device, std::array{*textureSetLayout.getLayout()}, {}},
      _toWindow{makeCopyPipeline(device, shaders, _layout, windowFormat, "Screen copy to window pipeline", cache)},
      _toScene{makeCopyPipeline(device, shaders, _layout, sceneFormat, "Screen copy to scene pipeline", cache)},
      _sampler{sampler},
      _sceneFormat{sceneFormat} {
    device.setName(*_layout.getLayout(), "Screen copy pipeline layout");
}

pipeline::GraphicsPipeline ScreenPass::createMaterialPipeline(
    const core::Device& device, const vk::raii::ShaderModule& vertex, const vk::raii::ShaderModule& fragment,
    bool /*instanceValues*/, const pipeline::PipelineLayout& layout, vk::Format colorFormat,
    const vk::raii::PipelineCache* cache) {
    // The instance values of a screen effect are constants of its code: nothing to read.
    return pipeline::GraphicsPipeline::Builder{}
        .setShaders(vertex, fragment)
        .setBlend(pipeline::GraphicsPipeline::BlendMode::kOpaque)
        .setColorFormat(colorFormat)
        .setLayout(layout)
        .build(device, cache);
}

std::unique_ptr<ScreenPass::Targets> ScreenPass::createTargets(const memory::Allocator& allocator,
                                                               descriptor::DescriptorAllocator& descriptors,
                                                               const descriptor::DescriptorSetLayout& textureSetLayout,
                                                               vk::Extent2D extent) const {
    const auto makeImage = [&](std::size_t index) {
        memory::Image image{*_device, allocator, extent, _sceneFormat,
                            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled};
        const std::string name = std::format("Screen {}", index);
        _device->setName(*image.getImage(), name);
        _device->setName(*image.getImageView(), name + " view");
        return image;
    };
    const auto makeSet = [&](const memory::Image& image, std::size_t index) {
        vk::raii::DescriptorSet set = descriptors.allocate(textureSetLayout);
        descriptor::DescriptorWriter{}
            .writeImage(kSamplerBinding, *image.getImageView(), _sampler, vk::ImageLayout::eShaderReadOnlyOptimal)
            .update(*_device, *set);
        _device->setName(*set, std::format("Screen {} set", index));
        return set;
    };
    std::array images{makeImage(0), makeImage(1)};
    std::array sets{makeSet(images.at(0), 0), makeSet(images.at(1), 1)};
    return std::make_unique<Targets>(Targets{.images = std::move(images), .sets = std::move(sets)});
}

std::size_t ScreenPass::recordEffects(const vk::raii::CommandBuffer& commandBuffer, const Targets& targets,
                                      std::span<const rendering::MaterialBinding> effects,
                                      const frame::FrameGlobals& globals, std::size_t frameIndex) const {
    std::size_t current = 0;
    for (const rendering::MaterialBinding& effect : effects) {
        if (effect.pipeline == nullptr) {
            continue;
        }
        const command::CommandLabel label{*_device, commandBuffer, "Screen effect"};
        const std::size_t next = 1 - current;
        const memory::Image& target = targets.images.at(next);
        rendering::transitionImage(commandBuffer, *target.getImage(), rendering::kSceneToColorAttachment);
        const vk::RenderingAttachmentInfo attachment =
            rendering::colorAttachment(*target.getImageView(), vk::ClearColorValue{});
        commandBuffer.beginRendering(rendering::renderingInfo(target.getExtent(), {&attachment, 1}));
        coverAll(commandBuffer, target.getExtent());
        effect.pipeline->bind(commandBuffer);
        const pipeline::PipelineLayout& layout = effect.pipeline->getLayout();
        globals.bind(commandBuffer, layout, frameIndex);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *layout.getLayout(), kTextureSet,
                                         *targets.sets.at(current), {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *layout.getLayout(), kMaterialSet,
                                         effect.set, {});
        commandBuffer.draw(3, 1, 0, 0);
        commandBuffer.endRendering();
        rendering::transitionImage(commandBuffer, *target.getImage(), rendering::kSceneToShaderRead);
        current = next;
    }
    return current;
}

void ScreenPass::recordCopy(const vk::raii::CommandBuffer& commandBuffer, vk::DescriptorSet source, vk::Extent2D extent,
                            bool toWindow) const {
    const command::CommandLabel label{*_device, commandBuffer, "Screen copy"};
    coverAll(commandBuffer, extent);
    const pipeline::GraphicsPipeline& copy = toWindow ? _toWindow : _toScene;
    copy.bind(commandBuffer);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *_layout.getLayout(), 0, source, {});
    commandBuffer.draw(3, 1, 0, 0);
}

}  // namespace aylo::render::vulkan::postprocess
