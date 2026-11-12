/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptPersistenceTests
*/

#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>

#include "aylo/ecs/app/App.hpp"
#include "aylo/scripting/ScriptHost.hpp"
#include "plugins/persistence/PersistencePlugin.hpp"
#include "plugins/persistence/PersistenceSettings.hpp"

TEST(ScriptPersistence, TheWriteHelperAcceptsItsDataField) {
    static std::atomic<int> counter{0};
    const auto directory =
        std::filesystem::temp_directory_path() / ("aylo-script-persistence-" + std::to_string(++counter));
    std::filesystem::remove_all(directory);
    {
        aylo::ecs::App app;
        app.addPlugin(aylo::plugins::persistence::PersistencePlugin{
            aylo::plugins::persistence::PersistenceSettings{.directory = directory.string(), .folder = "aylo"}});
        auto created = aylo::scripting::ScriptHost::create(app, {.isClient = true, .isServer = true});
        ASSERT_TRUE(created);
        const auto result = (*created)->run("persistence-test", R"(
            assert(engine.persistence.write(nil, { name = "t", data = "hello" }).ok)
            local read = engine.persistence.read(nil, { name = "t" })
            assert(read.ok and read.data == "hello")
            assert(engine.persistence.exists(nil, { name = "t" }) ~= nil)
            assert(engine.persistence.delete(nil, { name = "t" }).ok)
            assert(not pcall(engine.persistence.write, nil, { name = "t", data = "x", extra = 1 }))
        )");
        EXPECT_TRUE(result) << (result ? "" : std::string{result.error().message()});
    }
    std::filesystem::remove_all(directory);
}
