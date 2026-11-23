/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StaticMeshConstants
*/

#include "StaticMeshConstants.hpp"

#include <cstdint>
#include <glm/ext/vector_float4.hpp>

#include "aylo/contract/renderer/drawing/StaticMesh.hpp"
#include "aylo/contract/renderer/drawing/Texture.hpp"

namespace aylo::render::vulkan::rendering {

namespace {

/// @brief The bits of StaticMeshConstants::flags: the maps a part has (ayloPartHas* in common.glsl), its alpha mode.
constexpr std::uint32_t kHasMetallicRoughness = 1U;
constexpr std::uint32_t kHasNormal = 2U;
constexpr std::uint32_t kHasOcclusion = 4U;
constexpr std::uint32_t kHasEmissive = 8U;
constexpr std::uint32_t kAlphaModeShift = 4U;

}  // namespace

StaticMeshConstants StaticMeshConstants::of(const contract::renderer::StaticMeshPart& part,
                                            std::uint32_t draw) noexcept {
    using contract::renderer::TextureId;
    const auto has = [](TextureId texture, std::uint32_t flag) { return texture != TextureId{} ? flag : 0U; };
    return StaticMeshConstants{
        .baseColor = part.baseColor,
        .emissive = glm::vec4{part.emissive, part.normalScale},
        .surface = glm::vec4{part.metallic, part.roughness, part.occlusionStrength, part.alphaCutoff},
        .uvTransform = part.uvTransform,
        .draw = draw,
        .flags = has(part.metallicRoughnessTexture, kHasMetallicRoughness) | has(part.normalTexture, kHasNormal) |
                 has(part.occlusionTexture, kHasOcclusion) | has(part.emissiveTexture, kHasEmissive) |
                 (static_cast<std::uint32_t>(part.alphaMode) << kAlphaModeShift),
        .uvRotation = part.uvRotation};
}

}  // namespace aylo::render::vulkan::rendering
