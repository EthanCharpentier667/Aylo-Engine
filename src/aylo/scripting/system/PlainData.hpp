/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PlainData
*/

#pragma once

#include <string>

#include "aylo/ecs/type/TypeInfo.hpp"
#include "aylo/scripting/Export.hpp"

namespace aylo::scripting::system {
/// @return Why a resource cannot be given to scripts as bytes, or an empty string when it can: a script only gets plain
/// data, whose bytes it may read and write in place.
/// @param type The resource's type.
/// @param mainThread Whether the resource is main-thread only (ResourceInfo::nonSend).
[[nodiscard]] AYLO_SCRIPTING_API std::string whyNotPlainData(const ecs::TypeInfo& type, bool mainThread);
}  // namespace aylo::scripting::system
