/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** System
*/

#include "System.hpp"

#include <string_view>
#include <utility>

#include "aylo/ecs/change/Tick.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "aylo/ecs/system/SystemAccess.hpp"
#include "aylo/ecs/system/SystemParam.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::ecs {
std::string_view System::getName() const noexcept { return _name; }

Side System::getSide() const noexcept { return _side; }

const SystemAccess& System::getAccess() const noexcept { return _access; }

Tick System::getLastRun() const noexcept { return _lastRun; }

void System::setSide(Side side) noexcept { _side = side; }

void System::setRunCondition(RunCondition condition) { _condition = std::move(condition); }

bool System::hasRunCondition() const noexcept { return static_cast<bool>(_condition); }

bool System::canRun() const { return !_condition || _condition(*_world); }

void System::run() {
    const Tick thisRun = _world->getChangeTick();
    _body->run(SystemContext{.world = _world, .commands = &_commands, .lastRun = _lastRun, .thisRun = thisRun});
    _lastRun = thisRun;
    _world->advanceChangeTick();  // the next run starts after this tick: what this run wrote is not reported to it
}

void System::flushCommands() { _commands.flush(); }
}  // namespace aylo::ecs
