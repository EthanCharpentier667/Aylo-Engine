/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** FunctionHelper
*/

#include "FunctionHelper.hpp"

#include <string>
#include <string_view>

#include "aylo/luau/FunctionRef.hpp"
#include "aylo/luau/Table.hpp"
#include "aylo/luau/Userdata.hpp"
#include "aylo/luau/Value.hpp"

namespace aylo::luau::detail {
std::string_view luaTypeNameOf(const Value& value) noexcept {
    // We cannot use std::visit here because it may throw.
    if (value.is<bool>()) {
        return luaTypeName<bool>();
    }
    if (value.is<double>()) {
        return luaTypeName<double>();
    }
    if (value.is<Value::Vector>()) {
        return luaTypeName<Value::Vector>();
    }
    if (value.is<std::string>()) {
        return luaTypeName<std::string>();
    }
    if (value.is<Table>()) {
        return luaTypeName<Table>();
    }
    if (value.is<FunctionRef>()) {
        return luaTypeName<FunctionRef>();
    }
    if (value.is<Userdata>()) {
        return luaTypeName<Userdata>();
    }
    return luaTypeName<Value::Nil>();
}
}  // namespace aylo::luau::detail
