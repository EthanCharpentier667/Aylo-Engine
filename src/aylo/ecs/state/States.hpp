/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** States
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "aylo/ecs/Export.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"

namespace aylo::ecs {
/// @brief The game-flow state machines of a world, by name ("game.Phase", with "Menu", "Playing", "GameOver"), as the
/// resource "aylo.States". Each side has its own: they are not replicated.
///
/// @details A machine starts in its first value, entered on the first frame. A change asked with request() is applied
/// after PreUpdate of the next frame that simulates: the systems that leave the old value (onExit()) run, then those
/// that enter the new one (onEnter()), in the StateTransition phase, each pass with its commands applied. A system
/// with inState() runs only while its machine is in that value.
/// @code
/// states.add("game.Phase", {"Menu", "Playing", "GameOver"});
/// app.addSystem(aylo::ecs::Phase::kFixedUpdate, "game.spawnWaves", &spawnWaves)
///     .runIf(aylo::ecs::inState("game.Phase", "Playing"));
/// states.request("game.Phase", "Playing");  // applied at the next frame
/// @endcode
class AYLO_ECS_API States {
  public:
    /// @brief Which pass of a transition the StateTransition phase runs.
    enum class Stage : std::uint8_t {
        kNone,   ///< No transition: the phase is not running.
        kExit,   ///< The systems that leave the old values run.
        kEnter,  ///< The systems that enter the new values run.
    };

    States() = default;
    ~States() = default;
    States(const States& other) = default;
    States& operator=(const States& other) = default;
    States(States&& other) noexcept = default;
    States& operator=(States&& other) noexcept = default;

    /// @brief Declares the machine @p name with @p values, the first one entered on the next transition. Declaring it
    /// again with the same values does nothing (a script that runs again on a hot reload).
    /// @throws exceptions::StateException When the name or a value is empty, a value is given twice, there is no value,
    /// or the machine exists with other values.
    void add(std::string name, std::vector<std::string> values);

    /// @return Whether a machine has this name.
    [[nodiscard]] bool has(std::string_view name) const noexcept;

    /// @return The values of the machine, in the order they were declared.
    /// @throws exceptions::StateException When no machine has this name.
    [[nodiscard]] std::span<const std::string> getValues(std::string_view name) const;

    /// @return The value the machine is in, or nothing before it entered its first one.
    /// @throws exceptions::StateException When no machine has this name.
    [[nodiscard]] std::optional<std::string_view> get(std::string_view name) const;

    /// @return Whether the machine is in @p value; false for a machine or a value that does not exist.
    [[nodiscard]] bool isIn(std::string_view name, std::string_view value) const noexcept;

    /// @brief Asks the machine to go to @p value at the next transition; the last request of a frame wins, and asking
    /// for the value it is in, with nothing else asked, changes nothing.
    /// @throws exceptions::StateException When no machine has this name, or it has no such value.
    void request(std::string_view name, std::string_view value);

    /// @return Whether a machine has a change to apply.
    [[nodiscard]] bool hasPending() const noexcept;

    /// @brief Starts the exit pass of the pending changes: the machines that change remember what they leave.
    void beginExit() noexcept;

    /// @brief Moves the changing machines to their new values and starts the enter pass.
    void beginEnter() noexcept;

    /// @brief Ends the transition.
    void finish() noexcept;

    /// @return Whether the exit pass runs and the machine leaves @p value.
    [[nodiscard]] bool isLeaving(std::string_view name, std::string_view value) const noexcept;

    /// @return Whether the enter pass runs and the machine enters @p value.
    [[nodiscard]] bool isEntering(std::string_view name, std::string_view value) const noexcept;

  private:
    /// @brief No value yet: the position of a machine before it entered one, or of no change.
    static constexpr std::size_t kNone = static_cast<std::size_t>(-1);

    /// @brief One machine.
    struct Machine {
        std::string name;                 ///< Its name.
        std::vector<std::string> values;  ///< Its values.
        std::size_t current{kNone};       ///< The value it is in.
        std::size_t next{0};              ///< The value to go to at the next transition, or kNone.
        std::size_t leaving{kNone};       ///< During a transition: the value it leaves (kNone on its first one).
        std::size_t entering{kNone};      ///< During a transition: the value it enters.
    };

    /// @return The machine of this name, or null.
    [[nodiscard]] const Machine* find(std::string_view name) const noexcept;

    /// @return The machine of this name, or null, to change.
    [[nodiscard]] Machine* find(std::string_view name) noexcept;

    /// @return The machine of this name.
    /// @throws exceptions::StateException When there is none.
    [[nodiscard]] const Machine& require(std::string_view name) const;

    /// @return The position of @p value in @p machine, or kNone.
    [[nodiscard]] static std::size_t indexOf(const Machine& machine, std::string_view value) noexcept;

    std::vector<Machine> _machines;  ///< In the order they were declared.
    Stage _stage{Stage::kNone};      ///< The pass of the transition running.
};
AYLO_ECS_RESOURCE(States, "aylo.States");
}  // namespace aylo::ecs
