/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** NamedAssets
*/

#include "NamedAssets.hpp"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace aylo::scripting::asset {
void NamedAssets::name(const std::string_view path) {
    if (auto [where, added] = _named.emplace(path); added) {
        _new.push_back(*where);
    }
}

std::vector<std::string> NamedAssets::take() { return std::exchange(_new, {}); }
}  // namespace aylo::scripting::asset
