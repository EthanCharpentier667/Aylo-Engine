/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EditorBehaviourTests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"
#include "editor/behaviour/BehaviourFiles.hpp"

namespace {
namespace behaviour = aylo::editor::behaviour;

/// @brief An empty folder of the temporary directory, removed with the object.
class TemporaryFolder {
  public:
    explicit TemporaryFolder(const std::string& name) : _path{std::filesystem::temp_directory_path() / name} {
        std::error_code error;
        std::filesystem::remove_all(_path, error);
    }
    ~TemporaryFolder() {
        std::error_code error;
        std::filesystem::remove_all(_path, error);
    }
    TemporaryFolder(const TemporaryFolder& other) = delete;
    TemporaryFolder& operator=(const TemporaryFolder& other) = delete;
    TemporaryFolder(TemporaryFolder&& other) = delete;
    TemporaryFolder& operator=(TemporaryFolder&& other) = delete;

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return _path; }

  private:
    std::filesystem::path _path;
};

std::string readFile(const std::filesystem::path& file) {
    std::ifstream stream{file};
    std::ostringstream content;
    content << stream.rdbuf();
    return content.str();
}

/// @brief Registers a component named @p name, with no field, the way a script declares one.
void declare(aylo::ecs::App& app, const std::string& name) {
    aylo::ecs::ComponentInfo info;
    info.type.name = name;
    ASSERT_TRUE(app.getWorld().getComponents().registerType(std::move(info)).has_value());
}

void noop() {}
}  // namespace

TEST(EditorBehaviour, TheFolderIsTheBehavioursOfTheGame) {
    EXPECT_EQ(behaviour::behavioursFolder("game/assets"), std::filesystem::path{"game/scripts/behaviours"});
}

TEST(EditorBehaviour, NewBehaviourWritesTheTemplateAndRefusesABadName) {
    const TemporaryFolder folder{"aylo-editor-new-behaviour"};
    std::filesystem::path file;

    EXPECT_EQ(behaviour::createBehaviour(folder.path(), "Spinner.luau", file), "");
    EXPECT_EQ(file, folder.path() / "Spinner.luau");
    const std::string source = readFile(file);
    EXPECT_NE(source.find(R"(local Spinner = engine.behaviour("game.Spinner", {)"), std::string::npos) << source;
    for (const char* hook :
         {"onStart()", "onUpdate(dt)", "onFixedUpdate(dt)", "onEnable()", "onDisable()", "onDestroy()"}) {
        EXPECT_NE(source.find(std::string{"function Spinner:"} + hook), std::string::npos) << hook;
    }
    EXPECT_NE(source.find("return Spinner"), std::string::npos);

    EXPECT_NE(behaviour::createBehaviour(folder.path(), "Spinner", file).find("exists already"), std::string::npos);
    for (const char* bad : {"", "2Fast", "game.Spinner", "my spinner", "../Spinner"}) {
        EXPECT_NE(behaviour::createBehaviour(folder.path(), bad, file).find("cannot name a Behaviour"),
                  std::string::npos)
            << bad;
    }
}

TEST(EditorBehaviour, FindsTheFileThatDeclaresABehaviour) {
    const TemporaryFolder folder{"aylo-editor-find-behaviour"};
    std::filesystem::path file;
    ASSERT_EQ(behaviour::createBehaviour(folder.path() / "enemies", "Turret", file), "");
    ASSERT_EQ(behaviour::createBehaviour(folder.path(), "Spinner", file), "");

    EXPECT_EQ(behaviour::findBehaviourFile(folder.path(), "game.Turret"), folder.path() / "enemies" / "Turret.luau");
    EXPECT_EQ(behaviour::findBehaviourFile(folder.path(), "game.Spinner"), folder.path() / "Spinner.luau");
    EXPECT_FALSE(behaviour::findBehaviourFile(folder.path(), "game.Spin"));
    EXPECT_FALSE(behaviour::findBehaviourFile(folder.path() / "missing", "game.Spinner"));
}

TEST(EditorBehaviour, ABehaviourIsAComponentWithAHookSystem) {
    aylo::ecs::App app;
    declare(app, "game.Spinner");
    declare(app, "game.Spinner.Started");
    declare(app, "game.Health");
    declare(app, "game.Active");
    app.addSystem(aylo::ecs::Phase::kUpdate, "game.Spinner.onUpdate", &noop);
    app.addSystem(aylo::ecs::Phase::kUpdate, "game.Health.regen", &noop);

    EXPECT_TRUE(behaviour::isBehaviour(app, "game.Spinner"));
    EXPECT_FALSE(behaviour::isBehaviour(app, "game.Health"));

    const auto& registry = app.getWorld().getComponents().getRegistry();
    EXPECT_TRUE(behaviour::isBehaviourTag(registry, "game.Spinner.Started"));
    EXPECT_FALSE(behaviour::isBehaviourTag(registry, "game.Active")) << "'game' is no component";
    EXPECT_FALSE(behaviour::isBehaviourTag(registry, "game.Spinner"));
}
