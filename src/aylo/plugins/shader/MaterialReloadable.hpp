/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MaterialReloadable
*/

#pragma once

#include <filesystem>
#include <string_view>
#include <vector>

#include "aylo/core/reload/ChangeSet.hpp"
#include "aylo/core/reload/ReloadOutcome.hpp"
#include "aylo/core/reload/Reloadable.hpp"
#include "aylo/plugins/shader/Export.hpp"

namespace aylo::plugins::graphics {
class TextureAssets;
}  // namespace aylo::plugins::graphics

namespace aylo::plugins::shader {

class MaterialAssets;
class ShaderAssets;

/// @brief Follows the material files of a game for the HotReloader: a `.material` the game loaded, edited anywhere in
/// its assets, is read again (loadMaterialFile()), and what draws with it changes in the next frame.
/// @details A file that cannot be read keeps the last version of its material, and the reload is reported as failed.
/// The ShaderPlugin adds one when hot reload is on.
class AYLO_PLUGIN_SHADER_API MaterialReloadable : public core::reload::Reloadable {
  public:
    /// @param shaders, materials, textures The game's assets; must outlive this.
    /// @param assetDirectory The game's assets, relative to the HotReloader's root.
    MaterialReloadable(ShaderAssets* shaders, MaterialAssets* materials, graphics::TextureAssets* textures,
                       std::filesystem::path assetDirectory);
    ~MaterialReloadable() override = default;
    MaterialReloadable(const MaterialReloadable& other) = default;
    MaterialReloadable& operator=(const MaterialReloadable& other) = default;
    MaterialReloadable(MaterialReloadable&& other) noexcept = default;
    MaterialReloadable& operator=(MaterialReloadable&& other) noexcept = default;

    /// @return "materials".
    [[nodiscard]] std::string_view name() const noexcept override;

    /// @return The game's assets, `.material` files only.
    [[nodiscard]] std::vector<core::reload::WatchedPath> watched() const override;

    /// @brief Reads again the materials whose file changed.
    [[nodiscard]] core::reload::ReloadOutcome reload(const core::reload::ChangeSet& changes) override;

  private:
    ShaderAssets* _shaders{nullptr};              ///< The game's shaders.
    MaterialAssets* _materials{nullptr};          ///< The game's materials.
    graphics::TextureAssets* _textures{nullptr};  ///< The game's textures.
    std::filesystem::path _assets;                ///< What the changed paths start with.
};

}  // namespace aylo::plugins::shader
