/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Texture
*/

#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "render/vulkan/memory/Image.hpp"

namespace aylo::render::vulkan::texture {

/// @brief A texture of the renderer, behind a contract::renderer::TextureId: its image, how it is sampled,
/// and the descriptor set that hands both to the shaders.
struct Texture {
    memory::Image image;                   ///< RGBA8 sRGB, in SHADER_READ_ONLY_OPTIMAL once uploaded.
    contract::renderer::TextureDesc desc;  ///< Its size, filter and repeat mode (the sampler to use).
    vk::raii::DescriptorSet set;           ///< Set 1: the image and its sampler, at binding 0.
};

}  // namespace aylo::render::vulkan::texture
