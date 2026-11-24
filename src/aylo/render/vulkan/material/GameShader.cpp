/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** GameShader
*/

#include "GameShader.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <string_view>
#include <vulkan/vulkan_raii.hpp>

#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/Spirv.hpp"

namespace aylo::render::vulkan::material {

namespace {

/// @brief What a target is called in the name of its pipeline, in ShaderTarget order.
constexpr std::array<std::string_view, contract::renderer::kShaderTargetCount> kTargetNames{
    "sprite", "glyph", "particle", "shape", "screen", "mesh"};

/// @return A module named @p name for @p spirv, checked first.
vk::raii::ShaderModule createModule(const core::Device& device, std::span<const std::uint32_t> spirv,
                                    std::string_view name) {
    pipeline::validateSpirv(spirv, name);
    vk::raii::ShaderModule created{device.getDevice(), vk::ShaderModuleCreateInfo{{}, spirv}};
    device.setName(*created, name);
    return created;
}

}  // namespace

GameShader::GameShader(const core::Device& device, const contract::renderer::CompiledShader& shader)
    : _name{shader.name},
      _materialSize{shader.materialSize},
      _instanceValues{!shader.instanceValues.empty()},
      _renderState{shader.renderState},
      _castsShadows{shader.castsShadows},
      _cutsOut{shader.cutsOut},
      _readsDepth{shader.readsDepth},
      _readsPreviousFrame{shader.readsPreviousFrame} {
    contract::renderer::validate(shader);
    for (const contract::renderer::ShaderVariant& variant : shader.variants) {
        Variant& created = _variants.at(static_cast<std::size_t>(variant.target));
        created.vertex = createModule(device, variant.vertex, std::format("{} (vertex)", shader.name));
        created.fragment = createModule(device, variant.fragment, std::format("{} (fragment)", shader.name));
    }
}

bool GameShader::serves(contract::renderer::ShaderTarget target) const noexcept {
    // Every ShaderTarget has its variant (an array of kShaderTargetCount): no index can be out of range, but at() could
    // throw from a noexcept function, so the span is walked instead.
    const auto index = static_cast<std::size_t>(target);
    const std::span<const Variant> variants{_variants};
    return index < variants.size() && *variants.subspan(index).front().vertex != nullptr;
}

const pipeline::GraphicsPipeline* GameShader::getPipeline(contract::renderer::ShaderTarget target,
                                                          const core::Device& device,
                                                          const pipeline::PipelineLayout& layout,
                                                          vk::Format colorFormat, const vk::raii::PipelineCache* cache,
                                                          PipelineBuilder build,
                                                          const contract::renderer::RenderState& state) {
    Variant& variant = _variants.at(static_cast<std::size_t>(target));
    if (*variant.vertex == nullptr) {
        return nullptr;
    }
    // A 2D target ignores the render state: one pipeline for all its materials. A format a pass writes is part of the
    // key, so a screen effect writing a half-float and an 8-bit target keeps both.
    const contract::renderer::RenderState key =
        target == contract::renderer::ShaderTarget::kMesh ? state : contract::renderer::RenderState{};
    for (const Built& built : variant.built) {
        if (built.state == key && built.format == colorFormat) {
            return &built.pipeline;
        }
    }
    const Built& made = variant.built.emplace_back(Built{
        .state = key,
        .format = colorFormat,
        .pipeline = build(device, variant.vertex, variant.fragment, _instanceValues, layout, colorFormat, cache, key)});
    const pipeline::GraphicsPipeline& pipeline = made.pipeline;
    device.setName(*pipeline.getPipeline(),
                   std::format("{} {} pipeline", _name, kTargetNames.at(static_cast<std::size_t>(target))));
    return &pipeline;
}

const pipeline::GraphicsPipeline* GameShader::getShadowPipeline(const core::Device& device,
                                                                contract::renderer::CullMode cull,
                                                                const ShadowPipelineBuilder& build) {
    Variant& variant = _variants.at(static_cast<std::size_t>(contract::renderer::ShaderTarget::kMesh));
    if (*variant.vertex == nullptr || !_castsShadows) {
        return nullptr;
    }
    for (const auto& [builtCull, pipeline] : variant.shadows) {
        if (builtCull == cull) {
            return &pipeline;
        }
    }
    auto& [builtCull, pipeline] =
        variant.shadows.emplace_back(cull, build(variant.vertex, variant.fragment, _cutsOut, cull));
    device.setName(*pipeline.getPipeline(), std::format("{} shadow pipeline", _name));
    return &pipeline;
}

}  // namespace aylo::render::vulkan::material
