/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** CapsuleFilesTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "aylo/core/reload/HotReloader.hpp"
#include "aylo/core/reload/ReloadOutcome.hpp"
#include "aylo/core/reload/ReloadReport.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/change/Mut.hpp"
#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/hierarchy/Children.hpp"
#include "aylo/ecs/world/World.hpp"
#include "core/asset/exceptions/AssetExceptions.hpp"
#include "core/vfs/FileSource.hpp"
#include "plugins/AssetDirectory.hpp"
#include "plugins/HotReload.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsuleFiles.hpp"
#include "plugins/capsule/CapsuleInstance.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/capsule/CapsuleReloadable.hpp"
#include "plugins/capsule/CapsuleSpawn.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/LoadedCapsule.hpp"
#include "plugins/capsule/ReloadedCapsules.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"

namespace aylo::test::capsule_files {
struct Health {
    std::int32_t value{1};
};
AYLO_ECS_COMPONENT(Health, "test.files.Health", value);

struct Armor {
    std::int32_t value{0};
    std::int32_t extra{0};
};
AYLO_ECS_COMPONENT(Armor, "test.files.Armor", value, extra);
}  // namespace aylo::test::capsule_files

namespace capsule = aylo::plugins::capsule;
namespace tf = aylo::test::capsule_files;

namespace {
constexpr std::string_view kTurret = R"([node path="."]
test.files.Health = { value = 5 }
test.files.Armor = { value = 1, extra = 2 }
)";
constexpr std::string_view kBoss = R"([node path="."]
test.files.Health = { value = 50 }
[node path="Turret" instance="actors/turret.actor"]
test.files.Armor = { extra = 9 }
)";

/// @brief A game directory of its own, removed when the test ends, with the test components registered.
class CapsuleFilesTest : public ::testing::Test {
  protected:
    void SetUp() override {
        std::filesystem::create_directories(_game / "actors");
        static_cast<void>(world.getComponents().registerType<tf::Health>());
        static_cast<void>(world.getComponents().registerType<tf::Armor>());
    }
    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(_game, error);
    }

    /// @brief Writes the file `path` of the game's assets, dated a second after the last one so that it reads as
    /// changed.
    void write(std::string_view path, std::string_view text) {
        const std::filesystem::path file = _game / path;
        std::filesystem::create_directories(file.parent_path());
        std::ofstream{file, std::ios::binary} << text;
        _time += std::chrono::seconds{1};
        std::filesystem::last_write_time(file, _time);
    }

    /// @return A capsule's node value of `T`, or nullptr.
    template <typename T>
    const T* valueOf(std::string_view path, std::size_t node) const {
        const capsule::Capsule* found = capsules.find(path);
        return found != nullptr ? found->getNodes()[node].values.get<T>(world.getComponents()) : nullptr;
    }

    /// @brief Polls the reloader twice, as its debounce wants the same stamp two times.
    static std::vector<aylo::core::reload::ReloadReport> settle(aylo::core::reload::HotReloader& reloader) {
        static_cast<void>(reloader.poll());
        return reloader.poll();
    }

    std::filesystem::path _game{std::filesystem::temp_directory_path() /
                                std::format("aylo_capsule_files_{:08x}", std::random_device{}())};
    std::filesystem::file_time_type _time{std::filesystem::file_time_type::clock::now()};  ///< Of the last write.
    aylo::ecs::World world;                                                                ///< Holds the components.
    capsule::Capsules capsules;  ///< What the files are read into.
};
}  // namespace

TEST_F(CapsuleFilesTest, ReadsEveryCapsuleFileAnInstancedOneFirst) {
    write("actors/boss.actor", kBoss);  // sorted before the turret it instances
    write("actors/turret.actor", kTurret);
    write("scenes/level.scene", R"([node path="Boss" instance="actors/boss.actor"])"
                                "\n");  // not a root first: refused below
    write("scenes/empty.capsule", "");
    write("sprites/ship.png", "not a capsule");
    std::filesystem::remove(_game / "scenes/level.scene");
    capsule::CapsuleFiles files{_game, world.getComponents()};
    const std::vector<capsule::CapsuleId> read = files.loadAll(capsules);
    EXPECT_EQ(read.size(), 3U);
    EXPECT_EQ(capsules.getCount(), 3U);
    const tf::Armor* armor = valueOf<tf::Armor>("actors/boss.actor", 1);
    ASSERT_NE(armor, nullptr);
    EXPECT_EQ(armor->value, 1);  // the turret's, read first
    EXPECT_EQ(armor->extra, 9);  // the boss's override
    EXPECT_NE(capsules.find("scenes/empty.capsule"), nullptr);
}

TEST_F(CapsuleFilesTest, RefusesAFileThatCannotBeRead) {
    write("actors/broken.actor", "[node path=\".\"]\ntest.files.Health = { value = \"x\" }\n");
    capsule::CapsuleFiles files{_game, world.getComponents()};
    try {
        static_cast<void>(files.loadAll(capsules));
        FAIL() << "no CapsuleException";
    } catch (const capsule::exceptions::CapsuleException& error) {
        EXPECT_TRUE(std::string{error.what()}.starts_with("actors/broken.actor:2:31:")) << error.what();
    }
}

TEST_F(CapsuleFilesTest, ReadsASavedFileAgainWithTheFilesThatInstanceIt) {
    write("actors/turret.actor", kTurret);
    write("actors/boss.actor", kBoss);
    capsule::CapsuleFiles files{_game, world.getComponents()};
    static_cast<void>(files.loadAll(capsules));
    write("actors/turret.actor", "[node path=\".\"]\ntest.files.Armor = { value = 4 }\n");
    const std::vector<std::string> saved{"actors/turret.actor"};
    const capsule::CapsuleFiles::Reread reread = files.reload(capsules, saved);
    EXPECT_EQ(reread.capsules.size(), 2U);  // the turret, and the boss whose override starts from it
    EXPECT_TRUE(reread.errors.empty());
    EXPECT_EQ(capsules.find("actors/turret.actor")->getVersion(), 1U);
    EXPECT_EQ(capsules.find("actors/boss.actor")->getVersion(), 1U);
    EXPECT_EQ(valueOf<tf::Armor>("actors/boss.actor", 1)->value, 4);  // the turret's new value
    EXPECT_EQ(valueOf<tf::Armor>("actors/boss.actor", 1)->extra, 9);
}

TEST_F(CapsuleFilesTest, KeepsTheOldVersionOfAFileSavedBroken) {
    write("actors/turret.actor", kTurret);
    capsule::CapsuleFiles files{_game, world.getComponents()};
    static_cast<void>(files.loadAll(capsules));
    write("actors/turret.actor", "[node path=\".\"]\ntest.files.Health = { value = oops }\n");
    const std::vector<std::string> saved{"actors/turret.actor"};
    const capsule::CapsuleFiles::Reread reread = files.reload(capsules, saved);
    EXPECT_TRUE(reread.capsules.empty());
    ASSERT_EQ(reread.errors.size(), 1U);
    EXPECT_TRUE(reread.errors[0].starts_with("actors/turret.actor:2:")) << reread.errors[0];
    EXPECT_EQ(capsules.find("actors/turret.actor")->getVersion(), 0U);
    EXPECT_EQ(valueOf<tf::Health>("actors/turret.actor", 0)->value, 5);
}

TEST_F(CapsuleFilesTest, ReadsAFileThatAppearsAndSkipsOtherFiles) {
    capsule::CapsuleFiles files{_game, world.getComponents()};
    static_cast<void>(files.loadAll(capsules));
    write("actors/turret.actor", kTurret);
    write("sprites/ship.png", "not a capsule");
    const std::vector<std::string> added{"actors/turret.actor", "sprites/ship.png"};
    EXPECT_EQ(files.reload(capsules, added).capsules.size(), 1U);
    EXPECT_NE(capsules.find("actors/turret.actor"), nullptr);
}

TEST_F(CapsuleFilesTest, TheReloadableReadsWhatTheReloaderSees) {
    write("actors/turret.actor", kTurret);
    write("actors/boss.actor", kBoss);
    capsule::CapsuleFiles files{_game, world.getComponents()};
    static_cast<void>(files.loadAll(capsules));
    capsule::ReloadedCapsules reloaded;
    std::vector<std::string> log;
    aylo::core::reload::HotReloader reloader{_game.parent_path(),
                                             [&](std::string_view line) { log.emplace_back(line); }};
    const std::filesystem::path assets = _game.filename();  // the assets, in the reloader's root
    auto reloadable = std::make_unique<capsule::CapsuleReloadable>(&files, &capsules, &reloaded, assets);
    EXPECT_EQ(reloadable->name(), "capsules");
    ASSERT_EQ(reloadable->watched().size(), 1U);
    EXPECT_EQ(reloadable->watched()[0].path, assets);
    EXPECT_EQ(reloadable->watched()[0].extensions, (std::vector<std::string>{".capsule", ".scene", ".actor"}));
    reloader.add(std::move(reloadable));

    write("actors/turret.actor", "[node path=\".\"]\ntest.files.Armor = { value = 4 }\n");
    write("sprites/ship.png", "not followed");
    std::vector<aylo::core::reload::ReloadReport> reports = settle(reloader);
    ASSERT_EQ(reports.size(), 1U);
    EXPECT_EQ(reports[0].outcome.status, aylo::core::reload::ReloadOutcome::Status::kApplied);
    EXPECT_EQ(reloaded.capsules.size(), 2U);  // the turret and the boss, for the next frame
    EXPECT_EQ(capsules.find("actors/boss.actor")->getVersion(), 1U);

    write("actors/turret.actor", "[node path=\".\"]\ntest.files.Armor = { value = oops }\n");
    reports = settle(reloader);
    ASSERT_EQ(reports.size(), 1U);
    EXPECT_EQ(reports[0].outcome.status, aylo::core::reload::ReloadOutcome::Status::kFailed);
    EXPECT_NE(reports[0].outcome.message.find("actors/turret.actor:2:"), std::string::npos)
        << reports[0].outcome.message;
    EXPECT_EQ(capsules.find("actors/turret.actor")->getVersion(), 1U);  // kept
    EXPECT_EQ(log.size(), 2U);
}

TEST_F(CapsuleFilesTest, TheGameLoadsItsFilesAndReloadsWhatTheySpawned) {
    write("actors/turret.actor", kTurret);
    write("scenes/level.scene", "[node path=\".\"]\n[node path=\"Turret\" instance=\"actors/turret.actor\"]\n");
    aylo::core::reload::HotReloader reloader{_game.parent_path(), [](std::string_view /*line*/) {}};
    aylo::ecs::App app;
    app.insertResource(aylo::plugins::AssetDirectory{.path = _game});
    app.insertResource(aylo::plugins::HotReload{.reloader = &reloader});
    static_cast<void>(app.getWorld().getComponents().registerType<tf::Health>());
    static_cast<void>(app.getWorld().getComponents().registerType<tf::Armor>());
    app.addPlugin(capsule::CapsulePlugin{});
    app.finishPlugins();
    EXPECT_EQ(reloader.size(), 1U);
    aylo::ecs::Commands commands{app.getWorld()};
    const aylo::ecs::Entity level =
        capsule::loadCapsule(commands, app.getWorld().getResources().require<capsule::Capsules>(),
                             *app.getWorld().getResources().getMut<capsule::LoadedCapsule>(),
                             capsule::CapsuleId::fromPath("scenes/level.scene"))
            .getEntity();
    commands.flush();
    const aylo::ecs::Entity turret = app.getWorld().getComponents().get<aylo::ecs::Children>(level)->entities.at(0);
    EXPECT_EQ(app.getWorld().getComponents().get<tf::Health>(turret)->value, 5);

    write("actors/turret.actor", "[node path=\".\"]\ntest.files.Health = { value = 8 }\n");
    EXPECT_EQ(settle(reloader).size(), 1U);
    app.runFrame();
    ASSERT_TRUE(app.getWorld().isAlive(level));
    const aylo::ecs::Entity reloaded = app.getWorld().getComponents().get<aylo::ecs::Children>(level)->entities.at(0);
    EXPECT_EQ(app.getWorld().getComponents().get<tf::Health>(reloaded)->value, 8);
    EXPECT_EQ(app.getWorld().getComponents().get<capsule::CapsuleInstance>(reloaded)->version, 1U);
    EXPECT_TRUE(app.getWorld().getResources().get<capsule::ReloadedCapsules>()->capsules.empty());
}

TEST_F(CapsuleFilesTest, WithoutHotReloadTheGameOnlyLoadsItsFiles) {
    write("actors/turret.actor", kTurret);
    aylo::ecs::App app;
    app.insertResource(aylo::plugins::AssetDirectory{.path = _game});
    static_cast<void>(app.getWorld().getComponents().registerType<tf::Health>());
    static_cast<void>(app.getWorld().getComponents().registerType<tf::Armor>());
    app.addPlugin(capsule::CapsulePlugin{});
    app.finishPlugins();
    EXPECT_NE(app.getWorld().getResources().get<capsule::Capsules>()->find("actors/turret.actor"), nullptr);
    // Nothing reads the files again, but a tool can still ask for what it changed to be reloaded.
    ASSERT_NE(app.getWorld().getResources().get<capsule::ReloadedCapsules>(), nullptr);
    EXPECT_TRUE(app.getWorld().getResources().get<capsule::ReloadedCapsules>()->capsules.empty());
}

TEST_F(CapsuleFilesTest, ReloadsWhatACapsuleAToolRedefinedSpawnedWithoutHotReload) {
    write("actors/turret.actor", kTurret);
    aylo::ecs::App app;
    app.insertResource(aylo::plugins::AssetDirectory{.path = _game});
    static_cast<void>(app.getWorld().getComponents().registerType<tf::Health>());
    static_cast<void>(app.getWorld().getComponents().registerType<tf::Armor>());
    app.addPlugin(capsule::CapsulePlugin{});
    app.finishPlugins();
    aylo::ecs::Resources& resources = app.getWorld().getResources();
    const capsule::CapsuleId id = capsule::CapsuleId::fromPath("actors/turret.actor");
    aylo::ecs::Commands commands{app.getWorld()};
    const aylo::ecs::Entity turret =
        capsule::spawnCapsule(commands, resources.require<capsule::Capsules>(), id).getEntity();
    commands.flush();

    capsule::Capsule changed{"actors/turret.actor"};
    changed.editNode(0).values.set(app.getWorld().getComponents(), tf::Health{.value = 42});
    resources.requireMut<capsule::Capsules>()->define(std::move(changed));
    resources.requireMut<capsule::ReloadedCapsules>()->capsules.push_back(id);
    app.runFrame();
    ASSERT_TRUE(app.getWorld().isAlive(turret));
    EXPECT_EQ(app.getWorld().getComponents().get<tf::Health>(turret)->value, 42);
    EXPECT_TRUE(resources.get<capsule::ReloadedCapsules>()->capsules.empty());
}

namespace {
/// @brief Capsule files held in memory, as a pack holds them.
class MemoryAssets final : public aylo::core::vfs::FileSource {
  public:
    explicit MemoryAssets(std::map<std::string, std::string> files) : _files{std::move(files)} {}

    [[nodiscard]] std::string read(const std::string_view path) const override {
        const auto found = _files.find(std::string{path});
        if (found == _files.end()) {
            throw aylo::core::asset::exceptions::AssetLoadException{std::format("no '{}' in memory", path)};
        }
        return found->second;
    }
    [[nodiscard]] bool exists(const std::string_view path) const noexcept override {
        return _files.contains(std::string{path});
    }
    [[nodiscard]] std::vector<std::string> list(std::string_view /*folder*/) const override {
        std::vector<std::string> paths;
        for (const auto& [path, text] : _files) {
            paths.push_back(path);
        }
        return paths;
    }
    [[nodiscard]] std::optional<std::filesystem::path> directory() const override { return std::nullopt; }

  private:
    std::map<std::string, std::string> _files;  ///< The files, by path.
};
}  // namespace

TEST_F(CapsuleFilesTest, ReadsTheCapsuleFilesOfASourceWithNoFolder) {
    capsule::CapsuleFiles files{std::make_shared<MemoryAssets>(std::map<std::string, std::string>{
                                    {"actors/boss.actor", std::string{kBoss}},
                                    {"actors/turret.actor", std::string{kTurret}},
                                    {"sprites/ship.png", "not a capsule"},
                                }),
                                world.getComponents()};
    const std::vector<capsule::CapsuleId> read = files.loadAll(capsules);

    EXPECT_EQ(read.size(), 2U);
    EXPECT_NE(capsules.find("actors/boss.actor"), nullptr);
    EXPECT_NE(capsules.find("actors/turret.actor"), nullptr);
    const tf::Armor* armor = valueOf<tf::Armor>("actors/boss.actor", 1);
    ASSERT_NE(armor, nullptr);
    EXPECT_EQ(armor->extra, 9);
}
