/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** NetClient
*/

#include "aylo/network/client/NetClient.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "aylo/network/NetworkSettings.hpp"
#include "aylo/network/Types.hpp"
#include "aylo/network/client/ClientEvent.hpp"
#include "aylo/network/io/Address.hpp"

namespace aylo::network::client {
NetClient::NetClient(const io::Address& server, const NetworkSettings& settings)
    : _server{server}, _socket{0}, _core{settings} {}

void NetClient::connect(const TimePoint now) {
    _core.connect(now);
    flush(now);
}

void NetClient::poll(const TimePoint now) {
    _socket.poll();
    for (const auto& datagram : _socket.takeReceived()) {
        if (datagram.from == _server) {
            _core.receive(datagram.bytes, now);
        }
    }
    _core.update(now);
    flush(now);
}

bool NetClient::send(const Channel channel, const std::span<const std::byte> payload) {
    return _core.send(channel, payload);
}

void NetClient::sendReady() { _core.sendReady(); }

void NetClient::flush(const TimePoint now) {
    for (const auto& datagram : _core.takeDatagrams(now)) {
        _socket.sendTo(_server, datagram);
    }
}

void NetClient::disconnect(const TimePoint now, const DisconnectReason reason) {
    _core.disconnect(now, reason);
    flush(now);
}

std::vector<ClientEvent> NetClient::takeEvents() { return _core.takeEvents(); }

std::uint16_t NetClient::localPort() const { return _socket.localPort(); }

const ClientCore& NetClient::core() const noexcept { return _core; }
}  // namespace aylo::network::client
