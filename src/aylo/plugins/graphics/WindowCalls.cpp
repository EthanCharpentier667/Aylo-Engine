/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** WindowCalls
*/

#include "plugins/graphics/WindowCalls.hpp"

#include <string>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "plugins/graphics/CursorCapture.hpp"

namespace aylo::plugins::graphics {
namespace {
/// @brief `aylo.graphics.captureCursor`: what the game wants; the system "aylo.graphics.cursor" applies it.
void captureCursor(ecs::ResMut<CursorCapture> cursor, const CaptureCursorArguments& arguments) {
    cursor->wanted = arguments.capture;
}
}  // namespace

void addWindowCalls(ecs::App& app) { app.addCall(std::string{kCaptureCursorCall}, &captureCursor); }

}  // namespace aylo::plugins::graphics
