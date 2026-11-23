/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MaterialAssets
*/

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "core/asset/AssetId.hpp"
#include "core/asset/AssetStore.hpp"
#include "plugins/shader/Export.hpp"
#include "plugins/shader/Material.hpp"

namespace aylo::plugins::shader {

/// @brief The game's materials, as the ECS resource "aylo.MaterialAssets" the ShaderPlugin inserts: SpriteRenderer
/// and the other drawings keep the AssetId of the one they are drawn with.
///
/// @details Built in code (add()) or read from a `.material` file (loadMaterialFile(), MaterialFile.hpp). The
/// ShaderPlugin creates each material in the renderer once its shader and textures are ready, and updates it when
/// setValue() or setTexture() change it, from the next frame drawn.
/// @code
/// aylo::plugins::shader::Material hit{.shader = shaders->load("assets/shaders/flash.glsl")};
/// const aylo::core::asset::AssetId material = materials->add("enemy_hit", hit.set("tint", glm::vec4{1.0F}));
/// @endcode
class AYLO_PLUGIN_SHADER_API MaterialAssets {
  public:
    /// @brief One material: the name it was added under, and the material.
    using Entry = core::asset::AssetStore<Material>::Entry;

    MaterialAssets() = default;
    ~MaterialAssets() = default;
    MaterialAssets(const MaterialAssets& other) = delete;
    MaterialAssets& operator=(const MaterialAssets& other) = delete;
    MaterialAssets(MaterialAssets&& other) noexcept = default;
    MaterialAssets& operator=(MaterialAssets&& other) noexcept = default;

    /// @return The number of materials known.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @brief Adds @p material, known by @p name. Its values are checked against its shader when the ShaderPlugin
    /// creates it: a value the shader does not declare is reported then.
    /// @return Its id (AssetId::fromPath(name)). Adding a name twice keeps the first material.
    /// @throws core::asset::exceptions::AssetLoadException If another name already has this id (a hash collision).
    core::asset::AssetId add(const std::string& name, Material material);

    /// @brief Makes @p material the one known by @p name, adding it or replacing the one it had: a material file read
    /// again (loadMaterialFile()).
    /// @return Its id (AssetId::fromPath(name)).
    /// @throws core::asset::exceptions::AssetLoadException If another name already has this id (a hash collision).
    core::asset::AssetId define(const std::string& name, Material material);

    /// @brief Changes the shader of the material @p id, keeping its values (a value the new shader does not declare is
    /// reported when the material is created).
    /// @return False, doing nothing, if no material has this id.
    bool setShader(core::asset::AssetId id, core::asset::AssetId shader);

    /// @return The material @p id names, or nullptr if it is unknown.
    [[nodiscard]] const Material* get(core::asset::AssetId id) const;

    /// @return The entry of @p id, or nullptr if it is unknown.
    [[nodiscard]] const Entry* getEntry(core::asset::AssetId id) const;

    /// @brief Changes the value @p name of the material @p id: every drawing using it sees it from the next frame.
    /// @return False, doing nothing, if no material has this id.
    bool setValue(core::asset::AssetId id, const std::string& name, MaterialValue value);

    /// @brief Changes the texture @p name of the material @p id, as setValue().
    /// @return False, doing nothing, if no material has this id.
    bool setTexture(core::asset::AssetId id, const std::string& name, core::asset::AssetId texture);

    /// @brief Changes what the material @p id changes of its 3D shader's render state, as setValue().
    /// @return False, doing nothing, if no material has this id.
    bool setRenderState(core::asset::AssetId id, const RenderStateOverride& renderState);

    /// @brief Sets the material drawn after the material @p id on the same model (none: a default AssetId), as
    /// setValue().
    /// @return False, doing nothing, if no material has this id.
    bool setNextPass(core::asset::AssetId id, core::asset::AssetId nextPass);

    /// @brief Forgets the material @p id names; the ShaderPlugin destroys it in the renderer.
    /// @return False, doing nothing, if no material has this id.
    bool unload(core::asset::AssetId id);

    /// @return The materials added or changed since the last call, each once, for the ShaderPlugin.
    [[nodiscard]] std::vector<core::asset::AssetId> takeChanged();

    /// @return The materials unloaded since the last call, for the ShaderPlugin to destroy.
    [[nodiscard]] std::vector<core::asset::AssetId> takeUnloaded();

  private:
    /// @brief Records that @p id changed, once.
    void markChanged(core::asset::AssetId id);

    core::asset::AssetStore<Material> _materials{"Material"};  ///< Every material, by id.
    std::vector<core::asset::AssetId> _changed;                ///< Added or changed since the last takeChanged().
    std::vector<core::asset::AssetId> _unloaded;               ///< Unloaded since the last takeUnloaded().
};
AYLO_ECS_RESOURCE(MaterialAssets, "aylo.MaterialAssets");

}  // namespace aylo::plugins::shader
