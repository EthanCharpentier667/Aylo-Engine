/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ReliableReceiver
*/

#include "aylo/network/connection/ReliableReceiver.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "aylo/network/Types.hpp"
#include "aylo/network/protocol/Constants.hpp"
#include "aylo/network/protocol/Sequence.hpp"

namespace aylo::network::connection {
bool ReliableReceiver::receive(const std::uint16_t id, const std::span<const std::byte> payload) {
    if (id == _expected) {
        _delivered.emplace_back(payload.begin(), payload.end());
        ++_expected;
        while (Bytes* next = _early.find(_expected)) {
            _delivered.push_back(std::move(*next));
            _early.remove(_expected);
            ++_expected;
        }
        return true;
    }
    if (!protocol::isNewer(id, _expected)) {
        return true;
    }
    if (static_cast<std::uint16_t>(id - _expected) >= protocol::kReliableWindow) {
        return false;
    }
    if (_early.find(id) == nullptr) {
        _early.insert(id).assign(payload.begin(), payload.end());
    }
    return true;
}

std::vector<Bytes> ReliableReceiver::takeDelivered() { return std::exchange(_delivered, {}); }
}  // namespace aylo::network::connection
