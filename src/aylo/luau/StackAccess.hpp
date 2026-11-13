/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StackAccess
*/

#pragma once

#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "Export.hpp"
#include "Value.hpp"

struct lua_State;

namespace aylo::luau {
namespace detail {
class UsertypeData;
}  // namespace detail

template <typename T>
class Usertype;

/// @brief Writes the results of a native call straight onto the Luau stack, without building a `Value` for each.
///
/// @details Given to the raw handlers (`Function::setRawHandler()`, `Usertype::rawIndex()`), which run on every step of
/// a script's loop: a table or a userdata pushed here goes to the script as is, where a `Value` would first be held
/// through a reference in the registry. Each push adds one result; a push that fails raises a Luau error.
/// @code
/// iterator.setRawHandler([&](aylo::luau::Pusher& results) {
///     results.userdata(entities, entity);
///     results.record(keys, numbers);  // { x = ..., y = ... }
/// });
/// @endcode
class AYLO_LUAU_API Pusher {
  public:
    /// @brief Writes onto the stack of @p state. Made by the module for its raw handlers.
    explicit Pusher(lua_State* state) noexcept;
    ~Pusher() = default;
    Pusher(const Pusher& other) = delete;
    Pusher& operator=(const Pusher& other) = delete;
    Pusher(Pusher&& other) = delete;
    Pusher& operator=(Pusher&& other) = delete;

    /// @brief Pushes `nil`.
    void nil();

    /// @brief Pushes a number.
    void number(double number);

    /// @brief Pushes a boolean.
    void boolean(bool flag);

    /// @brief Pushes any value, as `pushValue()` would.
    /// @throws exceptions::RuntimeError If it cannot be pushed (a table of another state, out of memory).
    void value(const Value& value);

    /// @brief Pushes a new table of numbers: `{ [keys[i]] = numbers[i] }`.
    /// @param keys The names of the fields; as many as @p numbers.
    /// @param numbers Their values.
    /// @throws exceptions::InvalidOperation If the two spans differ in size.
    void record(std::span<const std::string_view> keys, std::span<const double> numbers);

    /// @brief Pushes a new userdata of @p type holding @p object.
    template <typename T>
    void userdata(const Usertype<T>& type, T object);

    /// @brief Pushes a new userdata built by @p initialize from @p source. Used by Usertype::push().
    /// @throws The exception of @p initialize, after removing the half-built userdata.
    void pushUserdata(const detail::UsertypeData& data, void (*initialize)(void* storage, void* source), void* source);

    /// @return How many values were pushed.
    [[nodiscard]] int count() const noexcept;

  private:
    /// @brief Makes room for @p slots more values.
    /// @throws exceptions::RuntimeError If the stack cannot grow.
    void reserve(int slots) const;

    lua_State* _state;  ///< The stack written to.
    int _count{0};      ///< The values pushed so far.
    int _room;          ///< The slots known to be free above the first push, without asking Luau.
};

/// @brief One value of the Luau stack of a native call, read where it is.
///
/// @details Given to the raw handlers (`Usertype::rawNewIndex()`): a table a script writes is visited in place instead
/// of being copied into a `Value` held through the registry. Valid only during the call that received it.
class AYLO_LUAU_API StackSlot {
  public:
    /// @brief The value at @p index of the stack of @p state. Made by the module for its raw handlers.
    StackSlot(lua_State* state, int index) noexcept;

    /// @return Whether the value is a table.
    [[nodiscard]] bool isTable() const noexcept;

    /// @return The value if it is a number (not a string that looks like one), nothing otherwise.
    [[nodiscard]] std::optional<double> number() const noexcept;

    /// @return The value if it is a boolean, nothing otherwise.
    [[nodiscard]] std::optional<bool> boolean() const noexcept;

    /// @return The value, as `readValue()` would read it.
    /// @throws exceptions::RuntimeError If its type cannot be read (a thread, a buffer).
    [[nodiscard]] Value value() const;

    /// @brief Visits the entries of a table, in no particular order.
    /// @param visit Called with the key when it is a string (nothing otherwise) and the value; returns false to stop.
    /// It must not keep the slot it is given. Does nothing when the value is not a table.
    template <typename F>
    void forEachEntry(F visit) const;

  private:
    /// @brief Visits the entries of the table; see forEachEntry().
    using Visitor = bool (*)(void* context, std::optional<std::string_view> key, const StackSlot& value);
    void visitEntries(Visitor visitor, void* context) const;

    lua_State* _state;  ///< The stack read from.
    int _index;         ///< Absolute index of the value.
};

template <typename T>
void Pusher::userdata(const Usertype<T>& type, T object) {
    type.push(*this, std::move(object));
}

template <typename F>
void StackSlot::forEachEntry(F visit) const {
    visitEntries([](void* context, const std::optional<std::string_view> key,
                    const StackSlot& value) { return (*static_cast<F*>(context))(key, value); },
                 &visit);
}
}  // namespace aylo::luau
