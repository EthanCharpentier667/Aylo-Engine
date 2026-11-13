/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ServerCore
*/

#include "aylo/network/server/ServerCore.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <utility>
#include <variant>
#include <vector>

#include "aylo/network/NetworkSettings.hpp"
#include "aylo/network/RandomSource.hpp"
#include "aylo/network/Types.hpp"
#include "aylo/network/bytes/Overloaded.hpp"
#include "aylo/network/connection/Connection.hpp"
#include "aylo/network/crypto/Crypto.hpp"
#include "aylo/network/io/Address.hpp"
#include "aylo/network/protocol/Constants.hpp"
#include "aylo/network/protocol/Handshake.hpp"
#include "aylo/network/protocol/Packets.hpp"
#include "aylo/network/server/ClientInfo.hpp"
#include "aylo/network/server/RoomMessages.hpp"
#include "aylo/network/server/Session.hpp"

namespace aylo::network::server {
namespace {
/// @brief The message type of `kReady`, the first payload byte.
constexpr std::uint8_t kReadyMessageType = 0x09;

/// @brief The first message type that belongs to the game; below it, the transport and the lobby.
constexpr std::uint8_t kFirstGameMessageType = 0x20;

/// @brief Times a `Disconnect` is sent, since nothing acknowledges it.
constexpr std::uint8_t kDisconnectRepeat = 3;
}  // namespace

namespace {
/// @return The identity of @p settings, or a new one from @p random.
crypto::Identity makeIdentity(const NetworkSettings& settings, const RandomSource& random) {
    crypto::initialize();
    return crypto::Identity::fromSeed(settings.identitySeed.value_or(crypto::makeSeed(random)));
}
}  // namespace

ServerCore::ServerCore(const NetworkSettings& settings, RandomSource random)
    : _settings{settings},
      _random{std::move(random)},
      _identity{makeIdentity(_settings, _random)},
      _exchange{crypto::KeyExchange::fromSeed(crypto::makeSeed(_random))},
      _exchangeSignature{_identity.sign(protocol::exchangeSignedMessage(_exchange.publicKey))} {}

void ServerCore::setWelcomeMessages(std::vector<Bytes> messages) { _welcomeMessages = std::move(messages); }

void ServerCore::receive(const io::Address& from, const std::span<const std::byte> datagram, const TimePoint now) {
    if (_isShutDown) {
        return;
    }
    const auto packet = protocol::PacketCodec::decode(datagram);
    if (!packet) {
        return;
    }
    std::visit(bytes::Overloaded{
                   [&](const protocol::ConnectRequest& request) { handleConnectRequest(from, request, now); },
                   [&](const protocol::ChallengeResponse& response) { handleChallengeResponse(from, response, now); },
                   [&](const protocol::Data& data) { handleData(from, data, now); },
                   [&](const protocol::Disconnect& disconnect) { handleDisconnect(from, disconnect); },
                   [](const auto&) {},
               },
               *packet);
}

bool ServerCore::execute(ServerCommand command) {
    return std::visit(bytes::Overloaded{
                          [this](SendMessage& message) {
                              bool queued = false;
                              if (message.client == kAllClients) {
                                  for (auto& session : _sessions | std::views::values) {
                                      if (session.state == SessionState::kReady) {
                                          queued = session.connection.send(message.channel, message.payload) || queued;
                                      }
                                  }
                                  return queued;
                              }
                              if (const auto found = _sessions.find(message.client);
                                  found != _sessions.end() && found->second.state == SessionState::kReady) {
                                  queued = found->second.connection.send(message.channel, message.payload);
                              }
                              return queued;
                          },
                          [this](const KickClient& kick) {
                              const bool exists = _sessions.contains(kick.client);
                              disconnect(kick.client, kick.reason);
                              return exists;
                          },
                      },
                      command);
}

void ServerCore::update(const TimePoint now) {
    if (_isShutDown) {
        return;
    }
    std::erase_if(_pending, [&](const auto& entry) { return now - entry.second.created > _settings.challengeExpiry; });

    std::vector<std::pair<ClientId, DisconnectReason>> closing;
    for (const auto& [id, session] : _sessions) {
        if (session.connection.isOverflowed()) {
            closing.emplace_back(id, DisconnectReason::kOverflow);
        } else if (now - session.connection.lastReceiveTime() > _settings.timeout) {
            closing.emplace_back(id, DisconnectReason::kTimeout);
        }
    }
    for (const auto& [id, reason] : closing) {
        if (reason == DisconnectReason::kOverflow) {
            disconnect(id, reason);
        } else {
            close(id, reason);
        }
    }
}

void ServerCore::shutdown(const TimePoint /*now*/) {
    if (_isShutDown) {
        return;
    }
    std::vector<ClientId> ids;
    ids.reserve(_sessions.size());
    for (const auto& key : _sessions | std::views::keys) {
        ids.push_back(key);
    }
    for (const ClientId id : ids) {
        disconnect(id, DisconnectReason::kServerShutdown);
    }
    _pending.clear();
    _isShutDown = true;
}

std::vector<OutgoingDatagram> ServerCore::takeDatagrams(const TimePoint now) {
    std::vector<OutgoingDatagram> result = std::move(_outgoing);
    _outgoing.clear();
    for (auto& session : _sessions | std::views::values) {
        for (Bytes& bytes : session.connection.buildPackets(now)) {
            result.push_back(OutgoingDatagram{.to = session.address, .bytes = std::move(bytes)});
        }
    }
    return result;
}

std::vector<ServerEvent> ServerCore::takeEvents() {
    std::vector<ServerEvent> result = std::move(_events);
    _events.clear();
    return result;
}

std::size_t ServerCore::sessionCount() const noexcept { return _sessions.size(); }

std::vector<ClientInfo> ServerCore::clients() const {
    std::vector<ClientInfo> infos;
    infos.reserve(_sessions.size());
    for (const auto& [id, session] : _sessions) {
        infos.push_back(ClientInfo{
            .client = id,
            .state = session.state,
            .address = session.address,
            .rttMs = session.connection.rtt().count(),
            .droppedMessages = session.connection.droppedMessageCount(),
            .protocolErrors = session.connection.protocolErrorCount(),
        });
    }
    return infos;
}

bool ServerCore::hasSession(const ClientId client) const noexcept { return _sessions.contains(client); }

std::size_t ServerCore::pendingChallengeCount() const noexcept { return _pending.size(); }

std::uint64_t ServerCore::malformedPacketCount() const noexcept { return _malformedPackets; }

std::uint64_t ServerCore::unexpectedMessageCount() const noexcept { return _unexpectedMessages; }

const crypto::Key& ServerCore::identityKey() const noexcept { return _identity.publicKey(); }

const crypto::Key& ServerCore::exchangeKey() const noexcept { return _exchange.publicKey; }

void ServerCore::handleConnectRequest(const io::Address& from, const protocol::ConnectRequest& request,
                                      const TimePoint now) {
    if (request.version != protocol::kProtocolVersion) {
        reply(from, protocol::Rejected{.reason = RejectReason::kVersionMismatch});
        return;
    }
    auto pending = _pending.find(from);
    if (pending != _pending.end() && now - pending->second.created > _settings.challengeExpiry) {
        _pending.erase(pending);
        pending = _pending.end();
    }
    if (pending != _pending.end()) {
        reply(from, makeChallenge(pending->second.value));
        return;
    }
    if (isFull(_clientsByAddress.contains(from))) {
        reply(from, protocol::Rejected{.reason = RejectReason::kServerFull});
        return;
    }
    if (!_pending.empty() && _pending.size() >= _settings.maxPendingChallenges) {
        const auto oldest = std::ranges::min_element(
            _pending, [](const auto& left, const auto& right) { return left.second.order < right.second.order; });
        _pending.erase(oldest);
    }
    const std::uint64_t value = _random();
    _pending.emplace(from, PendingChallenge{.value = value, .created = now, .order = _nextOrder++});
    reply(from, makeChallenge(value));
}

void ServerCore::handleChallengeResponse(const io::Address& from, const protocol::ChallengeResponse& response,
                                         const TimePoint now) {
    const auto existing = _clientsByAddress.find(from);
    if (existing != _clientsByAddress.end()) {
        if (const auto session = _sessions.find(existing->second);
            session != _sessions.end() && session->second.challenge == response.challenge) {
            replyBytes(from, session->second.accepted);
            return;
        }
    }
    const auto pending = _pending.find(from);
    if (pending == _pending.end() || pending->second.value != response.challenge) {
        return;
    }
    // The address is proven: only now is the key exchange worth its cost. A response whose seal does not open was
    // not made by the holder of the key it carries, and leaves the challenge waiting.
    const std::optional<crypto::SessionKeys> keys = crypto::serverSessionKeys(_exchange, response.exchange);
    if (!keys || !protocol::openResume(keys->receive, response)) {
        return;
    }
    const bool isExpired = now - pending->second.created > _settings.challengeExpiry;
    _pending.erase(pending);
    if (isExpired) {
        return;
    }
    if (isFull(existing != _clientsByAddress.end())) {
        reply(from, protocol::Rejected{.reason = RejectReason::kServerFull});
        return;
    }
    accept(from, response.challenge, *keys, now);
}

void ServerCore::handleData(const io::Address& from, const protocol::Data& data, const TimePoint now) {
    Session* session = findSession(from, data.sessionToken);
    if (session == nullptr) {
        return;
    }
    if (!session->connection.receive(data, now)) {
        ++_malformedPackets;
        return;
    }
    processMessages(*session);
}

void ServerCore::handleDisconnect(const io::Address& from, const protocol::Disconnect& disconnect) {
    if (Session* session = findSession(from, disconnect.sessionToken);
        session != nullptr && session->connection.openDisconnect(disconnect)) {
        close(session->client, DisconnectReason::kGraceful);
    }
}

void ServerCore::processMessages(Session& session) {
    for (auto& [channel, payload] : session.connection.takeMessages()) {
        if (payload.empty()) {
            ++_unexpectedMessages;
            continue;
        }
        const auto type = static_cast<std::uint8_t>(payload.front());
        if (session.state == SessionState::kWelcoming) {
            if (type == kReadyMessageType && channel == Channel::kReliable) {
                session.state = SessionState::kReady;
                _events.emplace_back(ClientJoined{.client = session.client});
            } else {
                ++_unexpectedMessages;
            }
        } else if (type >= kFirstGameMessageType) {
            _events.emplace_back(ClientMessage{
                .client = session.client,
                .channel = channel,
                .payload = std::move(payload),
            });
        } else if (type != kReadyMessageType) {
            ++_unexpectedMessages;
        }
    }
}

void ServerCore::accept(const io::Address& from, const std::uint64_t challenge, const crypto::SessionKeys& keys,
                        const TimePoint now) {
    if (const auto previous = _clientsByAddress.find(from); previous != _clientsByAddress.end()) {
        close(previous->second, DisconnectReason::kGraceful);
    }
    const ClientId id = _nextClient++;
    const std::uint32_t token = makeSessionToken();
    const std::uint64_t resumeToken = _random();
    auto [entry, inserted] = _sessions.emplace(
        id, Session{
                .address = from,
                .client = id,
                .sessionToken = token,
                .resumeToken = resumeToken,
                .challenge = challenge,
                .state = SessionState::kWelcoming,
                .connection = connection::Connection{token, _settings, now, keys},
                .accepted = protocol::PacketCodec::encode(protocol::Accepted{
                    .sealed = protocol::sealAccepted(
                        keys.transmit, {.sessionToken = token, .client = id, .resumeToken = resumeToken})}),
            });
    _clientsByAddress[from] = id;
    replyBytes(from, entry->second.accepted);
    for (const Bytes& message : _welcomeMessages) {
        entry->second.connection.send(Channel::kReliable, message);
    }
}

Session* ServerCore::findSession(const io::Address& from, const std::uint32_t token) {
    const auto byAddress = _clientsByAddress.find(from);
    if (byAddress == _clientsByAddress.end()) {
        return nullptr;
    }
    const auto session = _sessions.find(byAddress->second);
    if (session == _sessions.end() || session->second.sessionToken != token) {
        return nullptr;
    }
    return &session->second;
}

std::uint32_t ServerCore::makeSessionToken() const {
    constexpr int kMaxTries = 8;
    for (int attempt = 0; attempt < kMaxTries; ++attempt) {
        if (const auto token = static_cast<std::uint32_t>(_random()); token != 0) {
            return token;
        }
    }
    return 1;
}

bool ServerCore::isFull(const bool replacing) const noexcept {
    const std::size_t kept = _sessions.size() - (replacing && !_sessions.empty() ? 1 : 0);
    return kept >= _settings.maxClients || _nextClient == kAllClients;
}

void ServerCore::close(const ClientId client, const DisconnectReason reason) {
    const auto session = _sessions.find(client);
    if (session == _sessions.end()) {
        return;
    }
    const bool wasJoined = session->second.state == SessionState::kReady;
    _clientsByAddress.erase(session->second.address);
    _sessions.erase(session);
    if (wasJoined) {
        _events.emplace_back(ClientLeft{.client = client, .reason = reason});
    }
}

void ServerCore::disconnect(const ClientId client, const DisconnectReason reason) {
    const auto session = _sessions.find(client);
    if (session == _sessions.end()) {
        return;
    }
    for (std::uint8_t sent = 0; sent < kDisconnectRepeat; ++sent) {
        replyBytes(session->second.address, session->second.connection.makeDisconnect(reason));
    }
    close(client, reason);
}

void ServerCore::replyBytes(const io::Address& to, Bytes bytes) {
    if (!bytes.empty()) {
        _outgoing.push_back(OutgoingDatagram{.to = to, .bytes = std::move(bytes)});
    }
}

protocol::Challenge ServerCore::makeChallenge(const std::uint64_t value) const {
    return protocol::Challenge{.challenge = value,
                               .identity = _identity.publicKey(),
                               .exchange = _exchange.publicKey,
                               .signature = _exchangeSignature};
}

void ServerCore::reply(const io::Address& to, const protocol::Packet& packet) {
    std::visit(
        [&](const auto& alternative) {
            if (Bytes bytes = protocol::PacketCodec::encode(alternative); !bytes.empty()) {
                _outgoing.push_back(OutgoingDatagram{.to = to, .bytes = std::move(bytes)});
            }
        },
        packet);
}
}  // namespace aylo::network::server
