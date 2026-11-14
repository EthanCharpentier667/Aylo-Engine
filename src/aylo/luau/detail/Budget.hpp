/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Budget
*/

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "aylo/luau/ErrorKind.hpp"

struct lua_State;

namespace aylo::luau::detail {
/// @brief The limits of a runtime and what its scripts use of them: the memory they hold and the time the current call
/// from C++ has left.
///
/// @details One per runtime, made before its state and given to `lua_newstate` as the allocator's userdata, so any
/// thread of the state finds it (of()). It must outlive the state: `lua_close` frees through allocate().
///
/// The time is checked cooperatively, by Luau's interrupt (loop back edges, calls and returns): a script that runs no
/// Luau code, such as one long engine call, is not cut. The deadline is set by the outermost call from C++ (CallScope)
/// and shared by the calls nested in it.
class Budget {
  public:
    using Clock = std::chrono::steady_clock;  ///< The clock of the deadlines.

    /// @param memoryLimit The most bytes the state may hold; 0: no limit.
    /// @param callBudget How long a call from C++ may run; zero: no limit.
    Budget(std::size_t memoryLimit, Clock::duration callBudget) noexcept;

    /// @brief The `lua_Alloc` of the state: frees, or (re)allocates unless that takes the state past its limit, in
    /// which case it returns null and Luau raises a memory error.
    /// @param userdata The Budget.
    static void* allocate(void* userdata, void* block, std::size_t oldSize, std::size_t newSize) noexcept;

    /// @brief Luau's interrupt: raises an error in the running script once the deadline of the call has passed.
    /// @param gc Not negative when the collector calls it, where no error may be raised.
    static void interrupt(lua_State* state, int gc);

    /// @return The Budget of @p state (any of its threads), or null for a state the wrapper did not create.
    [[nodiscard]] static Budget* of(lua_State* state) noexcept;

    /// @return Whether the calls have a time limit, so the interrupt is needed.
    [[nodiscard]] bool limitsTime() const noexcept;
    /// @return The bytes the state holds now.
    [[nodiscard]] std::size_t used() const noexcept;
    /// @return The time limit of a call.
    [[nodiscard]] Clock::duration callBudget() const noexcept;
    /// @return Whether the interrupt stopped a script since the outermost call began.
    [[nodiscard]] bool timedOut() const noexcept;

    /// @brief A call from C++ into a script begins: the outermost one sets the deadline.
    void enter() noexcept;
    /// @brief A call from C++ into a script ends: the outermost one clears the deadline and the timeout.
    void leave() noexcept;

  private:
    /// The interrupt reads the clock once every that many calls: it runs at every loop back edge.
    static constexpr std::uint32_t kClockPeriod = 1024;

    std::size_t _memoryLimit;      ///< The most bytes the state may hold; 0: no limit.
    std::size_t _used{0};          ///< The bytes the state holds.
    Clock::duration _callBudget;   ///< How long a call may run; zero: no limit.
    Clock::time_point _deadline;   ///< When the current outermost call must have ended.
    std::uint32_t _depth{0};       ///< How many calls from C++ are running, nested.
    std::uint32_t _sinceClock{0};  ///< Interrupts since the clock was last read.
    bool _timedOut{false};         ///< The interrupt stopped a script in the current outermost call.
};

/// @brief A call from C++ into a script, for the time of its `lua_pcall`: see Budget::enter() and Budget::leave().
/// Does nothing on a state with no Budget.
class CallScope {
  public:
    /// @param state The state, or any of its threads, the call runs on.
    explicit CallScope(lua_State* state) noexcept;
    ~CallScope() noexcept;
    CallScope(const CallScope&) = delete;
    CallScope& operator=(const CallScope&) = delete;
    CallScope(CallScope&&) = delete;
    CallScope& operator=(CallScope&&) = delete;

  private:
    Budget* _budget;  ///< The budget of the state, or null.
};

/// @brief What a failed `lua_pcall` means: kTimeout when the interrupt stopped the script, kOutOfMemory on a memory
/// error, kStackOverflow when the calls nested too deep, kRuntime otherwise.
///
/// @details After a memory error it also runs a full collection, so the memory the failed script held is free for
/// the next one: Luau does not collect when an allocation fails.
/// @param state The state the call ran on, inside its CallScope, with the error value popped or about to be.
/// @param status What `lua_pcall` returned.
/// @param message The error message.
[[nodiscard]] ErrorKind failureKind(lua_State* state, int status, std::string_view message);
}  // namespace aylo::luau::detail
