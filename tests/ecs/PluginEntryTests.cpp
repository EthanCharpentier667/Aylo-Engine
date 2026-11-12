/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PluginEntryTests
*/

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/Plugin.hpp"
#include "aylo/ecs/app/PluginEntry.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "core/config/Settings.hpp"
#include "core/library/LibraryEntry.hpp"
#include "core/library/exceptions/LibraryException.hpp"

namespace {
/// @brief A plugin that keeps a setting, as one read by a factory.
class ConfiguredPlugin final : public aylo::ecs::Plugin {
  public:
    explicit ConfiguredPlugin(std::string level) : _level{std::move(level)} {}
    ~ConfiguredPlugin() override = default;
    ConfiguredPlugin(const ConfiguredPlugin& other) = default;
    ConfiguredPlugin& operator=(const ConfiguredPlugin& other) = default;
    ConfiguredPlugin(ConfiguredPlugin&& other) noexcept = default;
    ConfiguredPlugin& operator=(ConfiguredPlugin&& other) noexcept = default;

    [[nodiscard]] const std::string& getLevel() const noexcept { return _level; }
    [[nodiscard]] std::string_view getName() const noexcept override { return "test.configured"; }
    void build(aylo::ecs::App& /*app*/) override {}

  private:
    std::string _level;  ///< The setting read.
};
}  // namespace

AYLO_PLUGIN_LIBRARY(registrar) {
    registrar.getPlugins().add("test.configured", [](const aylo::core::config::Settings& settings) {
        return std::make_unique<ConfiguredPlugin>(settings.getString("test.level", "none"));
    });
}

TEST(PluginEntry, ALibraryPassesItsOwnCheck) {
    EXPECT_NO_THROW(aylo::core::library::checkLibrary(*ayloLibrary(), "test"));
}

TEST(PluginEntry, ALibraryRegistersItsPluginsWithTheSettings) {
    aylo::ecs::PluginRegistry registry;
    aylo::core::config::Settings settings;
    settings.set("test.level", "hard");
    registry.setSettings(settings);
    aylo::core::library::LibraryRegistrar registrar;
    registrar.provide(aylo::ecs::kPluginRegistryService, registry);
    ayloLibrary()->registerLibrary(registrar);

    const std::unique_ptr<aylo::ecs::Plugin> plugin = registry.create("test.configured");
    ASSERT_NE(plugin, nullptr);
    EXPECT_EQ(dynamic_cast<ConfiguredPlugin&>(*plugin).getLevel(), "hard");
}

TEST(PluginEntry, RefusesAnotherVersionAndNamesTheFile) {
    aylo::core::library::LibraryEntry library = *ayloLibrary();
    library.abiVersion = aylo::core::library::kLibraryAbiVersion + 1;
    try {
        aylo::core::library::checkLibrary(library, "boss.so");
        FAIL() << "the library was accepted";
    } catch (const aylo::core::library::exceptions::LibraryException& error) {
        EXPECT_TRUE(std::string_view{error.what()}.contains("boss.so"));
    }
}

TEST(PluginEntry, RefusesAnotherToolchain) {
    aylo::core::library::LibraryEntry library = *ayloLibrary();
    library.toolchain = "another compiler";
    EXPECT_THROW(aylo::core::library::checkLibrary(library, "boss.so"),
                 aylo::core::library::exceptions::LibraryException);
    library.toolchain = nullptr;
    EXPECT_THROW(aylo::core::library::checkLibrary(library, "boss.so"),
                 aylo::core::library::exceptions::LibraryException);
}
