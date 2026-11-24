/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EnvironmentMaps
*/

#include "EnvironmentMaps.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <format>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float4.hpp>
#include <memory>
#include <span>
#include <utility>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "aylo/contract/renderer/drawing/Light3D.hpp"
#include "environment_irradiance.frag.h"
#include "environment_prefilter.frag.h"
#include "environment_source.frag.h"
#include "render/vulkan/command/CommandLabel.hpp"
#include "render/vulkan/descriptor/DescriptorWriter.hpp"
#include "render/vulkan/rendering/DynamicRendering.hpp"
#include "screen_fullscreen.vert.h"
#include "sky.frag.h"

namespace aylo::render::vulkan::rendering {

namespace {

/// @brief The maps' texel format: half floats, the sky may be brighter than white.
constexpr vk::Format kMapFormat = vk::Format::eR16G16B16A16Sfloat;

/// @brief AyloEnvironmentPass of shaders/include/EnvironmentPass.glsl.
struct EnvironmentPassConstants {
    glm::vec4 top{0.0F};      ///< rgb: the gradient's top; w: 1 to read the source image.
    glm::vec4 horizon{0.0F};  ///< rgb: the gradient's horizon; w: the rotation.
    glm::vec4 bottom{0.0F};   ///< rgb: the gradient's bottom; w: the roughness, or the level to integrate.
    glm::vec4 source{0.0F};   ///< xy: the size of the image read; z: its levels.
};
static_assert(sizeof(EnvironmentPassConstants) == 64, "EnvironmentPassConstants must match AyloEnvironmentPass");

/// @brief AyloSky of shaders/sky.frag.
struct SkyConstants {
    glm::mat4 inverseViewProjection{1.0F};  ///< Clip space to the world.
    glm::vec4 values{0.0F};                 ///< x: intensity.
};
static_assert(sizeof(SkyConstants) == 80, "SkyConstants must match AyloSky");

/// @return The levels of an image of @p extent down to one texel on its longer side.
std::uint32_t levelsOf(vk::Extent2D extent) {
    return static_cast<std::uint32_t>(std::bit_width(std::max(extent.width, extent.height)));
}

/// @return @p extent's size at mip @p level, at least one texel a side.
vk::Extent2D levelExtent(vk::Extent2D extent, std::uint32_t level) {
    return vk::Extent2D{std::max(1U, extent.width >> level), std::max(1U, extent.height >> level)};
}

/// @brief Moves mips [@p first, @p first + @p count) of @p image from @p from to @p to, after @p after's accesses.
void transition(const vk::raii::CommandBuffer& commandBuffer, vk::Image image, std::uint32_t first, std::uint32_t count,
                vk::ImageLayout from, vk::ImageLayout to) {
    const vk::ImageMemoryBarrier2 barrier =
        vk::ImageMemoryBarrier2{}
            .setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands)
            .setSrcAccessMask(vk::AccessFlagBits2::eMemoryWrite)
            .setDstStageMask(vk::PipelineStageFlagBits2::eAllCommands)
            .setDstAccessMask(vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite)
            .setOldLayout(from)
            .setNewLayout(to)
            .setImage(image)
            .setSubresourceRange(vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, first, count, 0, 1});
    commandBuffer.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));
}

/// @return A view of mip @p level of @p image.
vk::raii::ImageView levelView(const core::Device& device, const memory::Image& image, std::uint32_t level) {
    return vk::raii::ImageView{device.getDevice(), vk::ImageViewCreateInfo{}
                                                       .setImage(*image.getImage())
                                                       .setViewType(vk::ImageViewType::e2D)
                                                       .setFormat(image.getFormat())
                                                       .setSubresourceRange(vk::ImageSubresourceRange{
                                                           vk::ImageAspectFlagBits::eColor, level, 1, 0, 1})};
}

/// @return A fullscreen pipeline of @p fragment into @p format, with @p layout.
pipeline::GraphicsPipeline makePass(const core::Device& device, pipeline::ShaderCache& shaders,
                                    const pipeline::PipelineLayout& layout, const char* name,
                                    std::span<const std::uint32_t> fragment, vk::Format format,
                                    const vk::raii::PipelineCache* cache) {
    pipeline::GraphicsPipeline built =
        pipeline::GraphicsPipeline::Builder{}
            .setShaders(shaders.get("screen_fullscreen.vert", aylo_shader_screen_fullscreen_vert),
                        shaders.get(name, fragment))
            .setBlend(pipeline::GraphicsPipeline::BlendMode::kOpaque)
            .setColorFormat(format)
            .setLayout(layout)
            .build(device, cache);
    device.setName(*built.getPipeline(), std::format("Environment {} pipeline", name));
    return built;
}

/// @return The sky's pipeline: the 3D pass's attachments, its depth neither tested nor written.
pipeline::GraphicsPipeline makeSky(const core::Device& device, pipeline::ShaderCache& shaders,
                                   const pipeline::PipelineLayout& layout, vk::Format color, vk::Format depth,
                                   const vk::raii::PipelineCache* cache) {
    pipeline::GraphicsPipeline built =
        pipeline::GraphicsPipeline::Builder{}
            .setShaders(shaders.get("screen_fullscreen.vert", aylo_shader_screen_fullscreen_vert),
                        shaders.get("sky.frag", aylo_shader_sky_frag))
            .setBlend(pipeline::GraphicsPipeline::BlendMode::kOpaque)
            .setColorFormat(color)
            .setDepth(depth, false)
            .setDepthTest(false)
            .setLayout(layout)
            .build(device, cache);
    device.setName(*built.getPipeline(), "Sky pipeline");
    return built;
}

/// @brief Records one fullscreen pass of @p pipeline into @p view, of @p extent, reading @p set.
void recordPass(const vk::raii::CommandBuffer& commandBuffer, const pipeline::GraphicsPipeline& pipeline,
                vk::ImageView view, vk::Extent2D extent, vk::DescriptorSet set,
                const EnvironmentPassConstants& constants) {
    const vk::RenderingAttachmentInfo attachment = colorAttachment(view, vk::ClearColorValue{});
    commandBuffer.beginRendering(renderingInfo(extent, {&attachment, 1}));
    commandBuffer.setViewport(
        0, vk::Viewport{0.0F, 0.0F, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0F, 1.0F});
    commandBuffer.setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, extent});
    pipeline.bind(commandBuffer);
    const vk::PipelineLayout layout = *pipeline.getLayout().getLayout();
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0, set, {});
    commandBuffer.pushConstants<EnvironmentPassConstants>(layout, vk::ShaderStageFlagBits::eFragment, 0, constants);
    commandBuffer.draw(3, 1, 0, 0);
    commandBuffer.endRendering();
}

}  // namespace

EnvironmentMaps::EnvironmentMaps(const core::Device& device, const memory::Allocator& allocator,
                                 pipeline::ShaderCache& shaders, vk::Format sceneFormat, vk::Format depthFormat,
                                 const vk::raii::PipelineCache* cache, std::size_t framesInFlight)
    : _device{&device},
      _allocator{&allocator},
      _sampler{device.getDevice(), vk::SamplerCreateInfo{}
                                       .setMagFilter(vk::Filter::eLinear)
                                       .setMinFilter(vk::Filter::eLinear)
                                       .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                                       .setAddressModeU(vk::SamplerAddressMode::eRepeat)
                                       .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
                                       .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
                                       .setMinLod(0.0F)
                                       .setMaxLod(vk::LodClampNone)},
      _setLayout{std::make_unique<descriptor::DescriptorSetLayout>(
          descriptor::DescriptorSetLayout::Builder{}
              .addBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment)
              .build(device))},
      _descriptors{std::make_unique<descriptor::DescriptorAllocator>(
          device, static_cast<std::uint32_t>(framesInFlight + 1),
          std::array{vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler,
                                            static_cast<std::uint32_t>(framesInFlight + 1)}},
          "Environment sets")},
      _passLayout{std::make_unique<pipeline::PipelineLayout>(
          device, std::array{*_setLayout->getLayout()},
          std::array{vk::PushConstantRange{vk::ShaderStageFlagBits::eFragment, 0, sizeof(EnvironmentPassConstants)}})},
      _skyLayout{std::make_unique<pipeline::PipelineLayout>(
          device, std::array{*_setLayout->getLayout()},
          std::array{vk::PushConstantRange{vk::ShaderStageFlagBits::eFragment, 0, sizeof(SkyConstants)}})},
      _source{makePass(device, shaders, *_passLayout, "environment_source.frag", aylo_shader_environment_source_frag,
                       kMapFormat, cache)},
      _prefilter{makePass(device, shaders, *_passLayout, "environment_prefilter.frag",
                          aylo_shader_environment_prefilter_frag, kMapFormat, cache)},
      _irradiance{makePass(device, shaders, *_passLayout, "environment_irradiance.frag",
                           aylo_shader_environment_irradiance_frag, kMapFormat, cache)},
      _sky{makeSky(device, shaders, *_skyLayout, sceneFormat, depthFormat, cache)} {
    device.setName(*_sampler, "Environment sampler");
    for (std::size_t frame = 0; frame < framesInFlight; ++frame) {
        _sourceSets.push_back(_descriptors->allocate(*_setLayout));
        device.setName(*_sourceSets.back(), "Sky image set");
    }
}

vk::ImageView EnvironmentMaps::getSpecular() const noexcept {
    return _maps ? *_maps->specular.getImageView() : vk::ImageView{};
}

vk::ImageView EnvironmentMaps::getDiffuse() const noexcept {
    return _maps ? *_maps->diffuse.getImageView() : vk::ImageView{};
}

EnvironmentMaps::Maps& EnvironmentMaps::createMaps() {
    if (_maps) {
        return *_maps;
    }
    constexpr vk::ImageUsageFlags kDrawn = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;
    memory::Image environment{*_device,
                              *_allocator,
                              kEnvironmentExtent,
                              kMapFormat,
                              kDrawn | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst,
                              levelsOf(kEnvironmentExtent)};
    memory::Image specular{*_device, *_allocator, kSkySpecularExtent, kMapFormat, kDrawn, kSkySpecularLevels};
    memory::Image diffuse{*_device, *_allocator, kSkyDiffuseExtent, kMapFormat, kDrawn};
    _device->setName(*environment.getImage(), "Sky environment");
    _device->setName(*specular.getImage(), "Sky specular");
    _device->setName(*diffuse.getImage(), "Sky diffuse");
    vk::raii::ImageView top = levelView(*_device, environment, 0);
    std::vector<vk::raii::ImageView> levels;
    levels.reserve(kSkySpecularLevels);
    for (std::uint32_t level = 0; level < kSkySpecularLevels; ++level) {
        levels.push_back(levelView(*_device, specular, level));
    }
    vk::raii::DescriptorSet set = _descriptors->allocate(*_setLayout);
    descriptor::DescriptorWriter{}
        .writeImage(0, *environment.getImageView(), *_sampler, vk::ImageLayout::eShaderReadOnlyOptimal)
        .update(*_device, *set);
    _device->setName(*set, "Sky environment set");
    _maps.emplace(Maps{.environment = std::move(environment),
                       .specular = std::move(specular),
                       .diffuse = std::move(diffuse),
                       .environmentTop = std::move(top),
                       .specularLevels = std::move(levels),
                       .environmentSet = std::move(set)});
    return *_maps;
}

void EnvironmentMaps::update(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex,
                             const contract::renderer::Environment3D& environment, vk::ImageView skyTexture,
                             vk::Sampler sampler) {
    using contract::renderer::SkyKind;
    const bool wanted = environment.sky != SkyKind::kNone && skyTexture != vk::ImageView{};
    if (!wanted) {
        _shown = false;
        return;
    }
    // The intensity is applied when the maps are read: changing it makes nothing again.
    contract::renderer::Environment3D sky = environment;
    sky.skyIntensity = 1.0F;
    sky.lightingFromSky = true;
    sky.ambient = {};
    sky.strength = 1.0F;
    sky.shadowCascades = 0;
    sky.shadowResolution = 0;
    sky.shadowDistance = 0.0F;
    if (_shown && _made == sky && _madeFrom == skyTexture) {
        return;
    }
    recordMaps(commandBuffer, frameIndex, createMaps(), sky, skyTexture, sampler);
    _made = sky;
    _madeFrom = skyTexture;
    _shown = true;
    ++_version;
}

void EnvironmentMaps::recordMaps(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex, const Maps& maps,
                                 const contract::renderer::Environment3D& environment, vk::ImageView skyTexture,
                                 vk::Sampler sampler) const {
    const command::CommandLabel label{*_device, commandBuffer, "Sky environment"};
    const vk::Image source = *maps.environment.getImage();
    const std::uint32_t sourceLevels = maps.environment.getMipLevels();
    const bool fromTexture = environment.sky == contract::renderer::SkyKind::kTexture;
    const vk::raii::DescriptorSet& sourceSet = _sourceSets.at(frameIndex);
    descriptor::DescriptorWriter{}
        .writeImage(0, skyTexture, sampler, vk::ImageLayout::eShaderReadOnlyOptimal)
        .update(*_device, *sourceSet);

    // The first level, then its mips by halving it.
    transition(commandBuffer, source, 0, 1, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
    recordPass(
        commandBuffer, _source, *maps.environmentTop, kEnvironmentExtent, *sourceSet,
        EnvironmentPassConstants{.top = glm::vec4{environment.skyTop, fromTexture ? 1.0F : 0.0F},
                                 .horizon = glm::vec4{environment.skyHorizon, environment.skyRotation},
                                 .bottom = glm::vec4{environment.skyBottom, 0.0F},
                                 .source = glm::vec4{static_cast<float>(kEnvironmentExtent.width),
                                                     static_cast<float>(kEnvironmentExtent.height), 1.0F, 0.0F}});
    recordMips(commandBuffer, maps);

    // The reflection, a level a roughness, from the environment's mips.
    const vk::Image specular = *maps.specular.getImage();
    transition(commandBuffer, specular, 0, kSkySpecularLevels, vk::ImageLayout::eUndefined,
               vk::ImageLayout::eColorAttachmentOptimal);
    for (std::uint32_t level = 0; level < kSkySpecularLevels; ++level) {
        const float roughness = static_cast<float>(level) / static_cast<float>(kSkySpecularLevels - 1);
        recordPass(commandBuffer, _prefilter, *maps.specularLevels.at(level), levelExtent(kSkySpecularExtent, level),
                   *maps.environmentSet,
                   EnvironmentPassConstants{.bottom = glm::vec4{0.0F, 0.0F, 0.0F, roughness},
                                            .source = glm::vec4{static_cast<float>(kEnvironmentExtent.width),
                                                                static_cast<float>(kEnvironmentExtent.height),
                                                                static_cast<float>(sourceLevels), 0.0F}});
    }
    transition(commandBuffer, specular, 0, kSkySpecularLevels, vk::ImageLayout::eColorAttachmentOptimal,
               vk::ImageLayout::eShaderReadOnlyOptimal);

    // The irradiance, from the level of 32 by 16 texels the shader integrates.
    const vk::Image diffuse = *maps.diffuse.getImage();
    transition(commandBuffer, diffuse, 0, 1, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
    const auto level32 = static_cast<float>(std::bit_width(kEnvironmentExtent.width / 32U) - 1);
    recordPass(commandBuffer, _irradiance, *maps.diffuse.getImageView(), kSkyDiffuseExtent, *maps.environmentSet,
               EnvironmentPassConstants{.bottom = glm::vec4{0.0F, 0.0F, 0.0F, level32}});
    transition(commandBuffer, diffuse, 0, 1, vk::ImageLayout::eColorAttachmentOptimal,
               vk::ImageLayout::eShaderReadOnlyOptimal);
}

void EnvironmentMaps::recordMips(const vk::raii::CommandBuffer& commandBuffer, const Maps& maps) {
    const vk::Image image = *maps.environment.getImage();
    const std::uint32_t levels = maps.environment.getMipLevels();
    transition(commandBuffer, image, 0, 1, vk::ImageLayout::eColorAttachmentOptimal,
               vk::ImageLayout::eTransferSrcOptimal);
    for (std::uint32_t level = 1; level < levels; ++level) {
        transition(commandBuffer, image, level, 1, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        const vk::Extent2D from = levelExtent(kEnvironmentExtent, level - 1);
        const vk::Extent2D to = levelExtent(kEnvironmentExtent, level);
        const vk::ImageBlit blit{vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, level - 1, 0, 1},
                                 {vk::Offset3D{0, 0, 0}, vk::Offset3D{static_cast<std::int32_t>(from.width),
                                                                      static_cast<std::int32_t>(from.height), 1}},
                                 vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, level, 0, 1},
                                 {vk::Offset3D{0, 0, 0}, vk::Offset3D{static_cast<std::int32_t>(to.width),
                                                                      static_cast<std::int32_t>(to.height), 1}}};
        commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image,
                                vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eLinear);
        transition(commandBuffer, image, level, 1, vk::ImageLayout::eTransferDstOptimal,
                   vk::ImageLayout::eTransferSrcOptimal);
    }
    transition(commandBuffer, image, 0, levels, vk::ImageLayout::eTransferSrcOptimal,
               vk::ImageLayout::eShaderReadOnlyOptimal);
}

void EnvironmentMaps::recordSky(const vk::raii::CommandBuffer& commandBuffer, const glm::mat4& inverseViewProjection,
                                float intensity) const {
    if (!_shown || !_maps) {
        return;
    }
    _sky.bind(commandBuffer);
    const vk::PipelineLayout layout = *_skyLayout->getLayout();
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0, *_maps->environmentSet, {});
    commandBuffer.pushConstants<SkyConstants>(
        layout, vk::ShaderStageFlagBits::eFragment, 0,
        SkyConstants{.inverseViewProjection = inverseViewProjection, .values = glm::vec4{intensity, 0.0F, 0.0F, 0.0F}});
    commandBuffer.draw(3, 1, 0, 0);
}

}  // namespace aylo::render::vulkan::rendering
