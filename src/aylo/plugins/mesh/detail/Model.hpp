/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Model
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
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

/// @brief The maps of a part besides its base colour, in the order of Model::partMaps.
enum class PartMap : std::uint8_t {
    kMetallicRoughness,  ///< Roughness in green, metallic in blue: data, read linear.
    kNormal,             ///< A tangent-space normal map: data, read linear.
    kOcclusion,          ///< Ambient occlusion in red: data, read linear.
    kEmissive,           ///< The light it gives off: a colour, read as sRGB.
};
/// @brief Number of PartMap values.
inline constexpr std::size_t kPartMapCount = 4;

/// @brief What a model file holds for the renderer, whatever its format: one mesh, its nodes flattened, and the images
/// of its parts.
struct Model {
    contract::renderer::StaticMeshDesc mesh;  ///< The vertices, indices and parts; the parts' textures unset.
    std::vector<std::optional<std::size_t>> partImages;  ///< Per part: its base colour image in `images`, or none.
    /// Per part: the images of its other maps in `images`, by PartMap; none for a map it does not have.
    std::vector<std::array<std::optional<std::size_t>, kPartMapCount>> partMaps;
    std::vector<ModelImage> images;      ///< The images the parts use.
    std::vector<std::string> partNames;  ///< Per part: the name of its material in the file, or empty without one.
};

}  // namespace aylo::plugins::mesh::detail
