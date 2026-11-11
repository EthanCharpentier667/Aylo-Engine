/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** FrameClock
*/

#pragma once

#include <cstdint>

#include "aylo/ecs/Export.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::ecs {
/// @brief What decides how many times a frame runs FixedUpdate, called by Schedules::runFrame() at three points of
/// every frame.
/// @details The time plugin installs one that measures the clock, runs a fixed timestep and keeps the Time resource
/// up to date; the schedules know nothing about time. Tests install one that returns the ticks they want.
/// @code
/// class EveryFrame : public aylo::ecs::FrameClock {
///   public:
///     std::uint32_t startFrame(aylo::ecs::World& world) override { return 1; }  // one fixed tick per frame
///     void finishFixedTick(aylo::ecs::World& world) override {}
///     void startUpdate(aylo::ecs::World& world) override {}
/// };
/// schedules.setClock(std::make_unique<EveryFrame>());
/// @endcode
class AYLO_ECS_API FrameClock {
  public:
    FrameClock() = default;
    virtual ~FrameClock();
    FrameClock(const FrameClock& other) = delete;
    FrameClock& operator=(const FrameClock& other) = delete;
    FrameClock(FrameClock&& other) = delete;
    FrameClock& operator=(FrameClock&& other) = delete;

    /// @brief Called first in every frame, before Startup on the first one: measures the frame.
    /// @return How many times the frame runs FixedUpdate.
    [[nodiscard]] virtual std::uint32_t startFrame(World& world) = 0;

    /// @brief Called after each FixedUpdate run, once the queues of its events moved forward.
    virtual void finishFixedTick(World& world) = 0;

    /// @brief Called after the fixed ticks of the frame, before Update.
    virtual void startUpdate(World& world) = 0;
};
}  // namespace aylo::ecs
