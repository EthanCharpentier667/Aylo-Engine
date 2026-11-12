/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Overloaded
*/

#pragma once

namespace aylo::network::bytes {
/// @brief Builds one callable out of several lambdas, to give `std::visit` one handler per alternative.
///
/// @details It is an aggregate on purpose (its bases are the lambdas), so it declares no special member.
template <typename... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};
}  // namespace aylo::network::bytes
