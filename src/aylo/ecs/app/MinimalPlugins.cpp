/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MinimalPlugins
*/

#include "MinimalPlugins.hpp"

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"

namespace aylo::ecs {
void MinimalPlugins::build(App& app) { app.addPlugin(TimePlugin{}); }
}  // namespace aylo::ecs
