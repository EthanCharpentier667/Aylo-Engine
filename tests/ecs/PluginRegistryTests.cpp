/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PluginRegistryTests
*/

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/Plugin.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"

namespace aylo::test::plugin_registry {
/// @brief A plugin that only has a name, and a tag telling which factory built it.
class NamedPlugin : public aylo::ecs::Plugin {
  public:
    NamedPlugin(std::string name, int tag) : _name{std::move(name)}, _tag{tag} {}
    ~NamedPlugin() override = default;
    NamedPlugin(const NamedPlugin& other) = default;
    NamedPlugin& operator=(const NamedPlugin& other) = default;
    NamedPlugin(NamedPlugin&& other) noexcept = default;
    NamedPlugin& operator=(NamedPlugin&& other) noexcept = default;

    [[nodiscard]] int getTag() const noexcept { return _tag; }

    [[nodiscard]] std::string_view getName() const noexcept override { return _name; }
    void build(aylo::ecs::App& /*app*/) override {}

  private:
    std::string _name;  ///< The plugin's name.
    int _tag;           ///< Which factory built it.
};

/// @return A factory building a NamedPlugin with this name and tag.
aylo::ecs::PluginRegistry::DefaultFactory makeFactory(std::string name, int tag) {
    return [name = std::move(name), tag] { return std::make_unique<NamedPlugin>(name, tag); };
}
}  // namespace aylo::test::plugin_registry

namespace preg = aylo::test::plugin_registry;

TEST(PluginRegistry, StartsEmpty) {
    const aylo::ecs::PluginRegistry registry;
    EXPECT_TRUE(registry.getNames().empty());
    EXPECT_FALSE(registry.has("test.a"));
    EXPECT_EQ(registry.create("test.a"), nullptr);
}

TEST(PluginRegistry, CreatesARegisteredPlugin) {
    aylo::ecs::PluginRegistry registry;
    registry.add("test.a", preg::makeFactory("test.a", 1));
    EXPECT_TRUE(registry.has("test.a"));
    const std::unique_ptr<aylo::ecs::Plugin> plugin = registry.create("test.a");
    ASSERT_NE(plugin, nullptr);
    EXPECT_EQ(plugin->getName(), "test.a");
}

TEST(PluginRegistry, CreatesANewPluginOnEachCall) {
    aylo::ecs::PluginRegistry registry;
    registry.add("test.a", preg::makeFactory("test.a", 1));
    const std::unique_ptr<aylo::ecs::Plugin> first = registry.create("test.a");
    const std::unique_ptr<aylo::ecs::Plugin> second = registry.create("test.a");
    EXPECT_NE(first.get(), second.get());
}

TEST(PluginRegistry, ReturnsNullForAnUnknownName) {
    aylo::ecs::PluginRegistry registry;
    registry.add("test.a", preg::makeFactory("test.a", 1));
    EXPECT_FALSE(registry.has("test.b"));
    EXPECT_EQ(registry.create("test.b"), nullptr);
}

TEST(PluginRegistry, ListsTheNamesSorted) {
    aylo::ecs::PluginRegistry registry;
    registry.add("test.c", preg::makeFactory("test.c", 1));
    registry.add("test.a", preg::makeFactory("test.a", 1));
    registry.add("test.b", preg::makeFactory("test.b", 1));
    EXPECT_EQ(registry.getNames(), (std::vector<std::string>{"test.a", "test.b", "test.c"}));
}

TEST(PluginRegistry, ReplacesAFactoryRegisteredUnderTheSameName) {
    aylo::ecs::PluginRegistry registry;
    registry.add("test.a", preg::makeFactory("test.a", 1));
    registry.add("test.a", preg::makeFactory("test.a", 2));
    EXPECT_EQ(registry.getNames().size(), 1U);
    const std::unique_ptr<aylo::ecs::Plugin> plugin = registry.create("test.a");
    ASSERT_NE(plugin, nullptr);
    EXPECT_EQ(dynamic_cast<const preg::NamedPlugin&>(*plugin).getTag(), 2);
}
