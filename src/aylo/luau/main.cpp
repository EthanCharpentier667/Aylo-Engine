/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** main
*/

#include <iostream>

#include "Export.hpp"

extern "C" AYLO_LUAU_API void lua_hello() { std::cout << "hello world!" << std::endl; }
