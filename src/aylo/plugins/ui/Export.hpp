/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Export
*/

#pragma once

#ifdef _WIN32
#ifdef AYLO_PLUGIN_UI_BUILD
#define AYLO_PLUGIN_UI_API __declspec(dllexport)
#else
#define AYLO_PLUGIN_UI_API __declspec(dllimport)
#endif
#else
#define AYLO_PLUGIN_UI_API __attribute__((visibility("default")))
#endif
