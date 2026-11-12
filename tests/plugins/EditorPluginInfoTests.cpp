/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EditorPluginInfoTests
*/

#include <gtest/gtest.h>

#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/Plugin.hpp"
#include "editor/panels/PluginInfo.hpp"

namespace {
/// @brief A plugin with a name and the dependencies it is given.
class NamedPlugin : public aylo::ecs::Plugin {
  public:
    NamedPlugin(std::string_view name, std::vector<std::string_view> dependencies)
        : _name{name}, _dependencies{std::move(dependencies)} {}
    [[nodiscard]] std::string_view getName() const noexcept override { return _name; }
    [[nodiscard]] std::span<const std::string_view> getDependencies() const noexcept override { return _dependencies; }
    void build(aylo::ecs::App& /*app*/) override {}

  private:
    std::string_view _name;
    std::vector<std::string_view> _dependencies;
};
}  // namespace

TEST(EditorPluginInfo, ListsLoadedPluginsWithRequiresAndRequiredBy) {
    aylo::ecs::App app;
    app.addPlugin(NamedPlugin{"aylo.Base", {}});
    app.addPlugin(NamedPlugin{"game.Boss", {"aylo.Base"}});
    const auto list = aylo::editor::panels::collectPlugins(app, "");
    ASSERT_EQ(list.loaded.size(), 2U);
    EXPECT_EQ(list.loaded[0].name, "aylo.Base");
    EXPECT_TRUE(list.loaded[0].isEngine);
    EXPECT_EQ(list.loaded[0].requiredBy, (std::vector<std::string>{"game.Boss"}));
    EXPECT_EQ(list.loaded[1].requires_, (std::vector<std::string>{"aylo.Base"}));
    EXPECT_FALSE(list.loaded[1].isEngine);
}

TEST(EditorPluginInfo, FiltersByNameIgnoringCaseButKeepsRequiredBy) {
    aylo::ecs::App app;
    app.addPlugin(NamedPlugin{"aylo.Base", {}});
    app.addPlugin(NamedPlugin{"game.Boss", {"aylo.Base"}});
    const auto list = aylo::editor::panels::collectPlugins(app, "BASE");
    ASSERT_EQ(list.loaded.size(), 1U);
    EXPECT_EQ(list.loaded[0].requiredBy, (std::vector<std::string>{"game.Boss"}));
}

TEST(EditorPluginInfo, ListsRegisteredPluginsThatWereNotAdded) {
    aylo::ecs::App app;
    app.getPluginRegistry().add(
        "game.Spare", [] { return std::make_unique<NamedPlugin>("game.Spare", std::vector<std::string_view>{}); });
    app.addPlugin(NamedPlugin{"game.Boss", {}});
    EXPECT_EQ(aylo::editor::panels::collectPlugins(app, "").notLoaded, (std::vector<std::string>{"game.Spare"}));
    app.addPlugin(NamedPlugin{"game.Spare", {}});
    EXPECT_TRUE(aylo::editor::panels::collectPlugins(app, "").notLoaded.empty());
}
