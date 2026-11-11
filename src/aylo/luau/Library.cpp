/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Library
*/

#include "Library.hpp"

#include <string>
#include <string_view>
#include <utility>

namespace aylo::luau {
Library::Library(std::string name, Table table, LibraryMembers ownedMembers)
    : _name{std::move(name)}, _table{std::move(table)}, _ownedMembers{std::move(ownedMembers)} {}

Library::~Library() = default;

Library::Library(Library&&) noexcept = default;

Library& Library::operator=(Library&&) noexcept = default;

std::string_view Library::name() const { return _name; }

const Table& Library::table() const { return _table; }
}  // namespace aylo::luau
