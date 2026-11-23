/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Model
*/

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "aylo/contract/renderer/drawing/StaticMesh.hpp"

namespace aylo::plugins::mesh::detail {

/// @brief An image a model uses: a file next to it, or bytes inside it (a `.glb`, a data URI, an FBX's embedded image).
struct ModelImage {
    std::string path;                ///< The image file, from the assets folder; empty for one inside the file.
    std::vector<std::byte> encoded;  ///< The encoded image (PNG, JPEG), for one inside the file.
};

/// @brief What a model file holds for the renderer, whatever its format: one mesh, its nodes flattened, and the images
/// of its parts.
struct Model {
    contract::renderer::StaticMeshDesc mesh;  ///< The vertices, indices and parts; the parts' textures unset.
    std::vector<std::optional<std::size_t>> partImages;  ///< Per part: its base colour image in `images`, or none.
    std::vector<ModelImage> images;                      ///< The images the parts use.
    std::vector<std::string> partNames;  ///< Per part: the name of its material in the file, or empty without one.
};

}  // namespace aylo::plugins::mesh::detail
