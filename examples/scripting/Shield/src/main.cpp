/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** main
*/

// A game made of Luau files: the ScriptingPlugin runs the entry game/scripts/main.luau, which requires the other
// files, and the components and systems they declare join the App.

#include <chrono>
#include <exception>
#include <iostream>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/time/FixedTimestep.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/scripting/plugin/ScriptingPlugin.hpp"

using namespace aylo;

int main() {
    try {
        ecs::App app;
        // One fixed tick per frame, one second long.
        app.addPlugin(ecs::TimePlugin::manual(std::chrono::seconds{1}, ecs::FixedTimestep{1, 5}));
        app.addPlugin(scripting::ScriptingPlugin{{
            .directory = "examples/scripting/Shield/game/scripts",
            .options = {.isClient = true, .isServer = true},
        }});

        for (int frame = 0; frame < 4; ++frame) {
            std::cout << "frame " << frame << '\n';
            app.runFrame();
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
