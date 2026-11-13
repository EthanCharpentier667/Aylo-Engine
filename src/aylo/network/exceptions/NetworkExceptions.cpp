/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** NetworkExceptions
*/

#include "aylo/network/exceptions/NetworkExceptions.hpp"

#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>

namespace aylo::network::exceptions {

SocketException::SocketException(const std::uint16_t port, const std::string& reason)
    : std::runtime_error{std::format("Cannot bind UDP port {}: {}", port, reason)}, _port{port} {}

std::uint16_t SocketException::getPort() const noexcept { return _port; }

CryptoException::CryptoException(const std::string& reason)
    : std::runtime_error{std::format("Cannot start the network's cryptography: {}", reason)} {}

}  // namespace aylo::network::exceptions
