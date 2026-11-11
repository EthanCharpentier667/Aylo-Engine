/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ReplicationExceptions
*/

#include "aylo/replication/exceptions/ReplicationExceptions.hpp"

#include <stdexcept>
#include <string>

namespace aylo::replication::exceptions {
ManifestException::ManifestException(const std::string& message) : std::runtime_error{message} {}

ManifestException::~ManifestException() = default;

InputLayoutException::InputLayoutException(const std::string& message) : std::runtime_error{message} {}

InputLayoutException::~InputLayoutException() = default;

ReplicationException::ReplicationException(const std::string& message) : std::runtime_error{message} {}

ReplicationException::~ReplicationException() = default;

PacketException::PacketException(const std::string& message) : std::runtime_error{message} {}

PacketException::~PacketException() = default;
}  // namespace aylo::replication::exceptions
