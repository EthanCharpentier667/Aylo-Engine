/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** NamedAssets
*/

#pragma once

#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "aylo/scripting/Export.hpp"

namespace aylo::scripting::asset {
/// @brief The asset paths scripts wrote where an asset id goes (a component's field, an engine call's argument, a
/// default), so that the engine loads them as a capsule's are: a script names `"fonts/orbitron.json"` and the font is
/// there, with no setup form.
/// @details Each path is kept once: take() gives the ones named since the last take, and a path named again after is
/// not given again.
class AYLO_SCRIPTING_API NamedAssets {
  public:
    /// @brief Records that a script named @p path (as written, relative to the assets).
    void name(std::string_view path);

    /// @return The paths named for the first time since the last call, in the order they were named.
    [[nodiscard]] std::vector<std::string> take();

  private:
    std::unordered_set<std::string> _named;  ///< Every path named so far.
    std::vector<std::string> _new;           ///< The ones take() has not given yet.
};
}  // namespace aylo::scripting::asset
