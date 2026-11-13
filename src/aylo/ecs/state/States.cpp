/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** States
*/

#include "aylo/ecs/state/States.hpp"

#include <algorithm>
#include <cstddef>
#include <format>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "aylo/ecs/exceptions/StateException.hpp"

namespace aylo::ecs {
void States::add(std::string name, std::vector<std::string> values) {
    if (name.empty()) {
        throw exceptions::StateException{"A state machine needs a name"};
    }
    if (values.empty()) {
        throw exceptions::StateException{std::format("State machine '{}' needs at least one value", name)};
    }
    for (auto value = values.begin(); value != values.end(); ++value) {
        if (value->empty()) {
            throw exceptions::StateException{std::format("State machine '{}' has an empty value", name)};
        }
        if (std::find(std::next(value), values.end(), *value) != values.end()) {
            throw exceptions::StateException{std::format("State machine '{}' has the value '{}' twice", name, *value)};
        }
    }
    if (const Machine* known = find(name)) {
        if (known->values != values) {
            throw exceptions::StateException{
                std::format("State machine '{}' is already declared with other values", name)};
        }
        return;
    }
    _machines.push_back(Machine{.name = std::move(name), .values = std::move(values)});
}

bool States::has(const std::string_view name) const noexcept { return find(name) != nullptr; }

std::span<const std::string> States::getValues(const std::string_view name) const { return require(name).values; }

std::optional<std::string_view> States::get(const std::string_view name) const {
    const Machine& machine = require(name);
    if (machine.current == kNone) {
        return std::nullopt;
    }
    return machine.values.at(machine.current);
}

bool States::isIn(const std::string_view name, const std::string_view value) const noexcept {
    const Machine* machine = find(name);
    return machine != nullptr && machine->current != kNone && machine->current == indexOf(*machine, value);
}

void States::request(const std::string_view name, const std::string_view value) {
    Machine* machine = find(name);
    if (machine == nullptr) {
        throw exceptions::StateException{std::format("No state machine is named '{}'", name)};
    }
    const std::size_t index = indexOf(*machine, value);
    if (index == kNone) {
        throw exceptions::StateException{std::format("State machine '{}' has no value '{}'", name, value)};
    }
    machine->next = index == machine->current ? kNone : index;
}

bool States::hasPending() const noexcept {
    return std::ranges::any_of(_machines, [](const Machine& machine) { return machine.next != kNone; });
}

void States::beginExit() noexcept {
    for (Machine& machine : _machines) {
        if (machine.next != kNone) {
            machine.leaving = machine.current;
            machine.entering = machine.next;
            machine.next = kNone;
        }
    }
    _stage = Stage::kExit;
}

void States::beginEnter() noexcept {
    for (Machine& machine : _machines) {
        if (machine.entering != kNone) {
            machine.current = machine.entering;
        }
    }
    _stage = Stage::kEnter;
}

void States::finish() noexcept {
    for (Machine& machine : _machines) {
        machine.leaving = kNone;
        machine.entering = kNone;
    }
    _stage = Stage::kNone;
}

bool States::isLeaving(const std::string_view name, const std::string_view value) const noexcept {
    const Machine* machine = find(name);
    return _stage == Stage::kExit && machine != nullptr && machine->entering != kNone && machine->leaving != kNone &&
           machine->leaving == indexOf(*machine, value);
}

bool States::isEntering(const std::string_view name, const std::string_view value) const noexcept {
    const Machine* machine = find(name);
    return _stage == Stage::kEnter && machine != nullptr && machine->entering != kNone &&
           machine->entering == indexOf(*machine, value);
}

const States::Machine* States::find(const std::string_view name) const noexcept {
    const auto found = std::ranges::find(_machines, name, &Machine::name);
    return found != _machines.end() ? &*found : nullptr;
}

States::Machine* States::find(const std::string_view name) noexcept {
    const auto found = std::ranges::find(_machines, name, &Machine::name);
    return found != _machines.end() ? &*found : nullptr;
}

const States::Machine& States::require(const std::string_view name) const {
    if (const Machine* machine = find(name)) {
        return *machine;
    }
    throw exceptions::StateException{std::format("No state machine is named '{}'", name)};
}

std::size_t States::indexOf(const Machine& machine, const std::string_view value) noexcept {
    const auto found = std::ranges::find(machine.values, value);
    return found != machine.values.end() ? static_cast<std::size_t>(found - machine.values.begin()) : kNone;
}
}  // namespace aylo::ecs
