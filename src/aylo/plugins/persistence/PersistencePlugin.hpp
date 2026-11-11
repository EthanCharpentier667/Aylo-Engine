/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PersistencePlugin
*/

#pragma once

#include <span>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/Plugin.hpp"
#include "plugins/persistence/Export.hpp"
#include "plugins/persistence/PersistenceSettings.hpp"

namespace aylo::plugins::persistence {

/// @brief Gives scripts a per-user save directory: read, write (atomic), delete and exists of named opaque texts.
/// Server-side or client-side as the project adds it; it needs no other plugin.
class AYLO_PLUGIN_PERSISTENCE_API PersistencePlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.Persistence";  ///< The plugin's name.

    explicit PersistencePlugin(PersistenceSettings settings = {});
    ~PersistencePlugin() override = default;
    PersistencePlugin(const PersistencePlugin& other) = default;
    PersistencePlugin& operator=(const PersistencePlugin& other) = default;
    PersistencePlugin(PersistencePlugin&& other) noexcept = default;
    PersistencePlugin& operator=(PersistencePlugin&& other) noexcept = default;

    /// @return "rtype.Persistence".
    [[nodiscard]] std::string_view getName() const noexcept override;
    /// @return No plugin: it needs none.
    [[nodiscard]] std::span<const std::string_view> getDependencies() const noexcept override;
    /// @brief Inserts the SaveStore and registers the calls.
    void build(ecs::App& app) override;

  private:
    PersistenceSettings _settings;
};

}  // namespace aylo::plugins::persistence
