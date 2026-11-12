/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TimePlugin
*/

#include "TimePlugin.hpp"

#include <chrono>
#include <memory>
#include <optional>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/time/FixedTimestep.hpp"
#include "aylo/ecs/time/Time.hpp"
#include "aylo/ecs/time/TimeClock.hpp"

namespace aylo::ecs {
TimePlugin TimePlugin::manual(std::chrono::nanoseconds frame, FixedTimestep timestep) noexcept {
    TimePlugin plugin{timestep};
    plugin._manualFrame = frame;
    return plugin;
}

TimePlugin::TimePlugin(FixedTimestep timestep) noexcept : _timestep{timestep} {}

const FixedTimestep& TimePlugin::getFixedTimestep() const noexcept { return _timestep; }

std::optional<std::chrono::nanoseconds> TimePlugin::getManualFrame() const noexcept { return _manualFrame; }

std::string_view TimePlugin::getName() const noexcept { return kName; }

void TimePlugin::build(App& app) {
    app.insertResource(Time{.delta = 0.F, .fixedDelta = _timestep.getTickSeconds(), .tick = 0, .alpha = 0.F});
    app.getSchedules().setClock(std::make_unique<TimeClock>(_timestep, _manualFrame));
}
}  // namespace aylo::ecs
