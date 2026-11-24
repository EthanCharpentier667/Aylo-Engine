/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MaterialFile
*/

#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "aylo/plugins/shader/Export.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/shader/Material.hpp"

namespace aylo::plugins::graphics {
class TextureAssets;
}  // namespace aylo::plugins::graphics

namespace aylo::plugins::shader {

class MaterialAssets;
class ShaderAssets;

/// @brief Most materials drawn on one model part: a material and the next passes it chains.
inline constexpr std::size_t kMaxMaterialPasses = 8;

/// @brief The extension of a material file.
inline constexpr std::string_view kMaterialExtension = ".material";

/// @brief A `.material` file as written: a shader, values and textures by name, all by path in the assets. JSON:
/// @code{.json}
/// { "shader": "shaders/flash.glsl", "values": { "tint": [1, 0.2, 0.2, 1], "amount": 0.8 },
///   "textures": { "noise": "textures/noise.png" } }
/// @endcode
/// @details What it leaves out keeps the shader's default. A value is a number or 2 to 4 numbers; a whole number given
/// to an `int` uniform is an int. `renderState` replaces parts of a 3D shader's render state:
/// `{ "cull": "front", "blend": "alpha", "depth": "test nowrite", "depthBias": -2, "depthBiasSlope": -1 }`
/// (RenderStateNames.hpp), each optional.
struct AYLO_PLUGIN_SHADER_API MaterialFile {
    /// @brief One value, by name.
    struct Value {
        std::string name;               ///< As declared in the shader.
        std::vector<float> components;  ///< 1 to 4 numbers.
    };
    /// @brief One texture, by name.
    struct Texture {
        std::string name;  ///< As declared in the shader (`uniform sampler2D <name>;`).
        std::string path;  ///< The image, in the assets.
    };

    std::string shader;               ///< The shader, in the assets or `@aylo/...`; empty: none yet.
    std::vector<Value> values;        ///< In the order of the file.
    std::vector<Texture> textures;    ///< In the order of the file.
    RenderStateOverride renderState;  ///< What it changes of its 3D shader's render state; written only if any.
    std::string nextPass;             ///< The `.material` drawn after it on the same model, in the assets; empty: none.

    /// @return The material read from @p text, a file named @p path for the messages.
    /// @throws core::asset::exceptions::AssetLoadException If it is not JSON or not a material: naming the file, the
    /// key and what is wrong.
    [[nodiscard]] static MaterialFile parse(std::string_view text, std::string_view path);

    /// @return This material as the JSON of a file, indented, ending with a new line.
    [[nodiscard]] std::string write() const;
};

/// @brief Reads the material file @p path (in the assets, as ShaderAssets reads its files), loads its shader,
/// textures and next pass (a file of its own, read the same way: a chain of kMaxMaterialPasses at most), and makes it
/// the material known by @p path (MaterialAssets::define()): read again, it replaces itself, so what draws with it
/// follows.
/// @return Its id.
/// @throws core::asset::exceptions::AssetLoadException If it cannot be read or is not a material, or its next passes
/// loop back to it or chain more than kMaxMaterialPasses.
AYLO_PLUGIN_SHADER_API core::asset::AssetId loadMaterialFile(const std::filesystem::path& path, ShaderAssets& shaders,
                                                             MaterialAssets& materials,
                                                             graphics::TextureAssets& textures);

/// @return The file of the material @p id, its shader and textures named by the paths they were loaded from; for the
/// editor to save it.
/// @throws core::asset::exceptions::AssetLoadException If no material has this id.
[[nodiscard]] AYLO_PLUGIN_SHADER_API MaterialFile describeMaterial(core::asset::AssetId id, const ShaderAssets& shaders,
                                                                   const MaterialAssets& materials,
                                                                   const graphics::TextureAssets& textures);

}  // namespace aylo::plugins::shader
