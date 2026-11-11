/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** RunModeTests
*/

#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/system/Side.hpp"

namespace {
using aylo::ecs::RunMode;
using aylo::ecs::runsIn;
using aylo::ecs::Side;

// A dedicated server runs server systems and systems of both sides.
static_assert(runsIn(Side::kServer, RunMode::kServer));
static_assert(runsIn(Side::kBoth, RunMode::kServer));
static_assert(!runsIn(Side::kClient, RunMode::kServer));

// A client runs client systems and systems of both sides.
static_assert(runsIn(Side::kClient, RunMode::kClient));
static_assert(runsIn(Side::kBoth, RunMode::kClient));
static_assert(!runsIn(Side::kServer, RunMode::kClient));

// Single player runs everything in one world.
static_assert(runsIn(Side::kServer, RunMode::kStandalone));
static_assert(runsIn(Side::kClient, RunMode::kStandalone));
static_assert(runsIn(Side::kBoth, RunMode::kStandalone));
}  // namespace
