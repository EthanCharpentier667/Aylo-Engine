/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** main
*/

// A game whose only content is its user interface, in game/scripts/menu.luau (its `scripts.main` entry). No C++ knows
// the UI: the Engine loads the plugins project.luau names, as for any game.

#include <exception>
#include <iostream>

#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/engine/Engine.hpp"

int main() {
    try {
        aylo::engine::Engine engine{{.mode = aylo::ecs::RunMode::kStandalone, .game = "examples/plugins/ui/game"}};
        return engine.run();
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
