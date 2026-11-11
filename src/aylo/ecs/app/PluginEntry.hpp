/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PluginEntry
*/

#pragma once

#include <string_view>

#include "aylo/ecs/Export.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "core/library/LibraryEntry.hpp"

namespace aylo::ecs {

/// @brief The name the App's PluginRegistry is offered under to the libraries.
inline constexpr std::string_view kPluginRegistryService = "rtype.ecs.plugins";

/// @brief What a plugin library registers, given to its AYLO_PLUGIN_LIBRARY body.
class AYLO_ECS_API PluginRegistrar {
  public:
    /// @param library What the libraries exchange; it offers the PluginRegistry and must outlive the registrar.
    explicit PluginRegistrar(core::library::LibraryRegistrar& library) noexcept;
    ~PluginRegistrar() = default;
    PluginRegistrar(const PluginRegistrar& other) = delete;
    PluginRegistrar& operator=(const PluginRegistrar& other) = delete;
    PluginRegistrar(PluginRegistrar&& other) = delete;
    PluginRegistrar& operator=(PluginRegistrar&& other) = delete;

    /// @return Where the library's plugins go.
    [[nodiscard]] PluginRegistry& getPlugins() const noexcept;

    /// @return What the libraries exchange, for a plugin that offers a registry to its backends.
    [[nodiscard]] core::library::LibraryRegistrar& getLibrary() const noexcept;

  private:
    core::library::LibraryRegistrar* _library;  ///< What the libraries exchange.
};

}  // namespace aylo::ecs

/// @brief Makes a shared library a plugin library: put it once in a `.cpp`, followed by what the library registers.
/// @code
/// AYLO_PLUGIN_LIBRARY(registrar) {
///     registrar.getPlugins().add("game.Boss", [](const aylo::core::config::Settings& settings) {
///         return std::make_unique<BossPlugin>(settings);
///     });
/// }
/// @endcode
// NOLINTBEGIN(cppcoreguidelines-macro-usage,bugprone-macro-parentheses): it wraps AYLO_LIBRARY and names a parameter.
#define AYLO_PLUGIN_LIBRARY(target)                                 \
    namespace {                                                     \
    namespace aylo_library {                                        \
    void registerPlugins(::aylo::ecs::PluginRegistrar& target);     \
    }                                                               \
    }                                                               \
    AYLO_LIBRARY(ayloLibraryRegistrar) {                            \
        ::aylo::ecs::PluginRegistrar plugins{ayloLibraryRegistrar}; \
        aylo_library::registerPlugins(plugins);                     \
    }                                                               \
    void aylo_library::registerPlugins(::aylo::ecs::PluginRegistrar& target)
// NOLINTEND(cppcoreguidelines-macro-usage,bugprone-macro-parentheses)
