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
void States::add(std::string name, std::vector<std::string> values, const Replication replication) {
    if (name.empty()) {
        throw exceptions::StateException{"A state machine needs a name"};
    }
    if (name.size() > kMaxLength) {
        throw exceptions::StateException{
            std::format("State machine '{}...': its name is longer than {} bytes", name.substr(0, 32), kMaxLength)};
    }
    if (values.empty()) {
        throw exceptions::StateException{std::format("State machine '{}' needs at least one value", name)};
    }
    for (auto value = values.begin(); value != values.end(); ++value) {
        if (value->empty()) {
            throw exceptions::StateException{std::format("State machine '{}' has an empty value", name)};
        }
        if (value->size() > kMaxLength) {
            throw exceptions::StateException{
                std::format("State machine '{}' has a value longer than {} bytes", name, kMaxLength)};
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
        if (known->replication != replication) {
            throw exceptions::StateException{
                std::format("State machine '{}' is already declared with another replication", name)};
        }
        return;
    }
    // A machine of the server, where the world follows it, waits for the server's value instead of its first one.
    const bool waits = _followsServer && replication == Replication::kFromServer;
    _machines.push_back(Machine{
        .name = std::move(name),
        .values = std::move(values),
        .next = waits ? kNone : 0,
        .replication = replication,
    });
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
    if (_followsServer && machine->replication == Replication::kFromServer) {
        throw exceptions::StateException{
            std::format("State machine '{}' is replicated: only the server changes it", name)};
    }
    machine->next = index == machine->current ? kNone : index;
}

bool States::isReplicated(const std::string_view name) const noexcept {
    const Machine* machine = find(name);
    return machine != nullptr && machine->replication == Replication::kFromServer;
}

std::vector<std::string_view> States::getReplicated() const {
    std::vector<std::string_view> names;
    for (const Machine& machine : _machines) {
        if (machine.replication == Replication::kFromServer) {
            names.emplace_back(machine.name);
        }
    }
    return names;
}

void States::followServer() noexcept {
    _followsServer = true;
    for (Machine& machine : _machines) {
        // Declared before: it has not entered its first value yet, and waits for the server's instead.
        if (machine.replication == Replication::kFromServer && machine.current == kNone) {
            machine.next = kNone;
        }
    }
}

bool States::followsServer() const noexcept { return _followsServer; }

bool States::follow(const std::string_view name, const std::string_view value) noexcept {
    Machine* machine = find(name);
    if (!_followsServer || machine == nullptr || machine->replication != Replication::kFromServer) {
        return false;
    }
    const std::size_t index = indexOf(*machine, value);
    if (index == kNone) {
        return false;
    }
    machine->next = index == machine->current ? kNone : index;
    return true;
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
