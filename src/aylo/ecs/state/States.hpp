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
/// resource "aylo.States". Each side has its own, except a machine declared Replication::kFromServer: the server owns
/// it, and a world that follows the server (followServer(), a client) takes its values from the server only.
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

    /// @brief Who changes a machine.
    enum class Replication : std::uint8_t {
        kLocal,       ///< Each side has its own and changes it.
        kFromServer,  ///< The server changes it and sends its value; a client follows it and cannot change it.
    };

    /// @brief The longest name or value, in bytes: what a script reads back and the network sends whole.
    static constexpr std::size_t kMaxLength = 255;

    States() = default;
    ~States() = default;
    States(const States& other) = default;
    States& operator=(const States& other) = default;
    States(States&& other) noexcept = default;
    States& operator=(States&& other) noexcept = default;

    /// @brief Declares the machine @p name with @p values, the first one entered on the next transition (in a world
    /// that follows the server, a machine of the server enters the value the server sends instead). Declaring it again
    /// as it is does nothing (a script that runs again on a hot reload).
    /// @throws exceptions::StateException When the name or a value is empty or longer than kMaxLength, a value is given
    /// twice, there is no value, or the machine exists with other values or another replication.
    void add(std::string name, std::vector<std::string> values, Replication replication = Replication::kLocal);

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
    /// @throws exceptions::StateException When no machine has this name, it has no such value, or the world follows
    /// the server and the server owns the machine.
    void request(std::string_view name, std::string_view value);

    /// @return Whether the server owns the machine (Replication::kFromServer); false for an unknown name.
    [[nodiscard]] bool isReplicated(std::string_view name) const noexcept;

    /// @return The names of the machines the server owns, in the order they were declared.
    [[nodiscard]] std::vector<std::string_view> getReplicated() const;

    /// @brief Makes this world follow the server: the machines it owns enter no value of their own, and only follow()
    /// changes them. A client's replication calls it once.
    void followServer() noexcept;

    /// @return Whether this world follows the server.
    [[nodiscard]] bool followsServer() const noexcept;

    /// @brief Applies a value the server sent: the machine goes to it at the next transition, as with request().
    /// @return False, changing nothing, when the world does not follow the server, or no machine the server owns has
    /// this name, or it has no such value: the bytes came from the network.
    bool follow(std::string_view name, std::string_view value) noexcept;

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
        Replication replication{Replication::kLocal};  ///< Who changes it.
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
    bool _followsServer{false};      ///< Set by followServer().
};
AYLO_ECS_RESOURCE(States, "aylo.States");
}  // namespace aylo::ecs
