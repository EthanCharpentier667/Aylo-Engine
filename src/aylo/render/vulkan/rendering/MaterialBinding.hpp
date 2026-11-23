/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MaterialBinding
*/

#pragma once

#include <functional>
#include <vulkan/vulkan_raii.hpp>

#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"

namespace aylo::render::vulkan::rendering {

/// @brief What a batch with a game material is drawn with.
struct MaterialBinding {
    const pipeline::GraphicsPipeline* pipeline = nullptr;  ///< Its shader's; null: drawn as the default material.
    vk::DescriptorSet set;                                 ///< Its set 2, for this frame.
    bool instanceValues = false;                           ///< Its pipeline reads the instance values binding.
    contract::renderer::RenderState state;  ///< How it is drawn (kMesh): its material's, or its shader's.
    const pipeline::GraphicsPipeline* shadowPipeline = nullptr;  ///< kMesh: what draws it into a shadow map; none.
    bool readsDepth = false;                                     ///< kScreen: the effect reads the 3D depth (set 3).
};

/// @brief Finds what a game material is drawn with this frame for a target, or a null pipeline if it cannot be drawn.
using MaterialFinder = std::function<MaterialBinding(contract::renderer::MaterialId, contract::renderer::ShaderTarget)>;

}  // namespace aylo::render::vulkan::rendering
