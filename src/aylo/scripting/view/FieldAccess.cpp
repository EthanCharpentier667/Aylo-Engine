/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** FieldAccess
*/

#include "FieldAccess.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <glm/ext/vector_float3.hpp>
#include <iterator>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>
#include <vector>

#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/type/String.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"
#include "aylo/luau/StackAccess.hpp"
#include "aylo/luau/Table.hpp"
#include "aylo/luau/Userdata.hpp"
#include "aylo/luau/Usertype.hpp"
#include "aylo/luau/Value.hpp"
#include "aylo/scripting/Error.hpp"
#include "aylo/scripting/ErrorKind.hpp"
#include "aylo/scripting/Result.hpp"
#include "core/asset/AssetId.hpp"

namespace aylo::scripting::view {
namespace {
/// @brief Largest integer a Luau number holds exactly (2^53), the limit of the 64-bit integer fields.
constexpr double kMaxExactInteger = 9007199254740992.0;

Error fieldError(const std::string_view component, const ecs::FieldInfo& field, const std::string_view problem) {
    return Error{ErrorKind::kInvalidValue, std::format("field '{}' of '{}': {}", field.name, component, problem)};
}

/// @return Nothing when a script can read and write the field, or the error saying why not.
Result<void> checkSupported(const std::string_view component, const ecs::FieldInfo& field,
                            const std::span<const std::byte> bytes) {
    if (field.count != 1) {
        return tl::unexpected{fieldError(component, field, "array fields are not supported")};
    }
    const auto size = ecs::getFieldLayout(field.type).size;
    if (size > bytes.size() || field.offset > bytes.size() - size) {
        return tl::unexpected{fieldError(component, field, "the field is out of the component's bytes")};
    }
    return {};
}

/// @brief Names of the components of a vector field, in memory order.
constexpr std::array<std::string_view, 4> kVectorKeys{"x", "y", "z", "w"};

/// @return How many floats a vector field type holds.
std::size_t vectorWidth(const ecs::FieldType type) {
    switch (type) {
        case ecs::FieldType::kVec2:
            return 2;
        case ecs::FieldType::kVec3:
            return 3;
        default:
            return 4;
    }
}

template <typename T>
T load(const std::span<const std::byte> bytes, const ecs::FieldInfo& field) {
    T value{};
    std::memcpy(&value, bytes.subspan(field.offset, sizeof(T)).data(), sizeof(T));
    return value;
}

template <typename T>
void store(const std::span<std::byte> bytes, const ecs::FieldInfo& field, const T value) {
    std::memcpy(bytes.subspan(field.offset, sizeof(T)).data(), &value, sizeof(T));
}

template <typename T>
Result<void> writeInteger(const std::string_view component, const ecs::FieldInfo& field, std::span<std::byte> bytes,
                          const double number) {
    const auto low = std::max(static_cast<double>(std::numeric_limits<T>::min()), -kMaxExactInteger);
    const auto high = std::min(static_cast<double>(std::numeric_limits<T>::max()), kMaxExactInteger);
    if (std::isnan(number) || std::trunc(number) != number) {
        return tl::unexpected{fieldError(component, field, std::format("{} is not a whole number", number))};
    }
    if (number < low || number > high) {
        return tl::unexpected{
            fieldError(component, field, std::format("{} is out of range [{}, {}]", number, low, high))};
    }
    store<T>(bytes, field, static_cast<T>(number));
    return {};
}

template <typename T>
Result<luau::Value> readInteger(const std::string_view component, const ecs::FieldInfo& field,
                                const std::span<const std::byte> bytes) {
    constexpr auto kLimit = static_cast<std::int64_t>(kMaxExactInteger);
    const auto value = load<T>(bytes, field);
    if (std::cmp_less(value, -kLimit) || std::cmp_greater(value, kLimit)) {
        const auto message = std::format("{} is out of range [{}, {}]", value, -kLimit, kLimit);
        return tl::unexpected{fieldError(component, field, message)};
    }
    return luau::Value{static_cast<double>(value)};
}

/// @return Whether narrowing the number to a float is defined: infinities and NaN pass, finite overflows do not.
bool fitsFloat(const double number) {
    return !std::isfinite(number) || std::fabs(number) <= static_cast<double>(std::numeric_limits<float>::max());
}

/// @return The names of the components of a vector field type.
std::span<const std::string_view> vectorKeys(const ecs::FieldType type) {
    return std::span{kVectorKeys}.first(vectorWidth(type));
}

/// @return What a table written to a vector field type must name, for the messages.
std::string_view expectedKeys(const ecs::FieldType type) {
    switch (type) {
        case ecs::FieldType::kVec2:
            return "x and y";
        case ecs::FieldType::kVec3:
            return "x, y and z";
        default:
            return "x, y, z and w";
    }
}

/// @brief Reads a vector field: a Luau `vector` for a `vec3`, a table of its components for the others.
Result<luau::Value> readVector(const std::string_view component, const ecs::FieldInfo& field,
                               const std::span<const std::byte> bytes, const FieldReadContext& context) {
    const auto keys = vectorKeys(field.type);
    std::array<float, 4> components{};
    std::memcpy(components.data(), bytes.subspan(field.offset, keys.size() * sizeof(float)).data(),
                keys.size() * sizeof(float));
    if (field.type == ecs::FieldType::kVec3) {
        return luau::Value{glm::vec3{components.at(0), components.at(1), components.at(2)}};
    }
    if (context.runtime == nullptr) {
        return tl::unexpected{fieldError(component, field, "no runtime to build a table with")};
    }
    auto table = luau::Table::create(*context.runtime, 0, static_cast<int>(keys.size()));
    if (!table) {
        return tl::unexpected{fieldError(component, field, table.error().message())};
    }
    for (std::size_t index = 0; index < keys.size(); ++index) {
        const luau::Value number{static_cast<double>(components.at(index))};
        if (auto set = table->set(keys.subspan(index, 1).front(), number); !set) {
            return tl::unexpected{fieldError(component, field, set.error().message())};
        }
    }
    return luau::Value{std::move(*table)};
}

/// @brief Writes the components of a vector field from the entries of a table that names every one of them and
/// nothing else.
/// @param forEach Calls its argument with the key of each entry (nothing when it is not a string) and its number (null
/// when it is not one), until it returns false.
template <typename ForEach>
Result<void> writeVectorEntries(const std::string_view component, const ecs::FieldInfo& field,
                                const std::span<std::byte> bytes, const ForEach& forEach) {
    const auto keys = vectorKeys(field.type);
    std::array<float, 4> components{};
    std::array<bool, 4> seen{};
    std::optional<Error> error;
    forEach([&](const std::optional<std::string_view> name, const double* number) {
        // A span's iterator is a pointer on libstdc++ and libc++ but a class on MSVC, so it cannot be spelled `auto*`.
        // NOLINTNEXTLINE(readability-qualified-auto)
        const auto found = name ? std::ranges::find(keys, *name) : keys.end();
        if (found == keys.end()) {
            const std::string_view given = name ? *name : "(not a string)";
            error = fieldError(component, field,
                               std::format("unexpected key '{}', expected {}", given, expectedKeys(field.type)));
            return false;
        }
        if (number == nullptr) {
            error = fieldError(component, field, std::format("'{}' must be a number", *found));
            return false;
        }
        if (!fitsFloat(*number)) {
            error = fieldError(component, field, std::format("'{}' is out of range", *found));
            return false;
        }
        const auto index = static_cast<std::size_t>(std::ranges::distance(keys.begin(), found));
        components.at(index) = static_cast<float>(*number);
        seen.at(index) = true;
        return true;
    });
    if (error) {
        return tl::unexpected{std::move(*error)};
    }
    for (std::size_t index = 0; index < keys.size(); ++index) {
        if (!seen.at(index)) {
            return tl::unexpected{
                fieldError(component, field, std::format("missing '{}'", keys.subspan(index, 1).front()))};
        }
    }
    std::memcpy(bytes.subspan(field.offset, keys.size() * sizeof(float)).data(), components.data(),
                keys.size() * sizeof(float));
    return {};
}

/// @brief Writes the components of a vector field from a table that names every one of them and nothing else.
Result<void> writeVectorTable(const std::string_view component, const ecs::FieldInfo& field,
                              const std::span<std::byte> bytes, const luau::Table& table) {
    const auto entries = table.entries();
    if (!entries) {
        return tl::unexpected{fieldError(component, field, entries.error().message())};
    }
    return writeVectorEntries(component, field, bytes, [&entries](const auto& visit) {
        for (const auto& [key, value] : *entries) {
            const auto* name = key.tryAs<std::string>();
            const auto given = name != nullptr ? std::optional<std::string_view>{*name} : std::nullopt;
            if (!visit(given, value.tryAs<double>())) {
                return;
            }
        }
    });
}

/// @return Whether a field type is a vector.
bool isVector(const ecs::FieldType type) {
    return type == ecs::FieldType::kVec2 || type == ecs::FieldType::kVec3 || type == ecs::FieldType::kVec4;
}

/// @brief Writes a `vec2`, `vec3` or `vec4` field from a table, or from a `vector` for a `vec3`.
Result<void> writeVector(const std::string_view component, const ecs::FieldInfo& field,
                         const std::span<std::byte> bytes, const luau::Value& value) {
    if (const auto* table = value.tryAs<luau::Table>(); table != nullptr) {
        return writeVectorTable(component, field, bytes, *table);
    }
    if (const auto* vector = value.tryAs<glm::vec3>(); vector != nullptr && field.type == ecs::FieldType::kVec3) {
        std::memcpy(bytes.subspan(field.offset, sizeof(glm::vec3)).data(), &vector->x, sizeof(glm::vec3));
        return {};
    }
    switch (field.type) {
        case ecs::FieldType::kVec2:
            return tl::unexpected{fieldError(component, field, "expected a table {x=, y=}")};
        case ecs::FieldType::kVec3:
            return tl::unexpected{fieldError(component, field, "expected a vector or a table {x=, y=, z=}")};
        default:
            return tl::unexpected{fieldError(component, field, "expected a table {x=, y=, z=, w=}")};
    }
}

/// @brief Writes an asset id field from a path, which is hashed, or from an `AssetId`.
Result<void> writeAssetId(const std::string_view component, const ecs::FieldInfo& field,
                          const std::span<std::byte> bytes, const luau::Value& value) {
    if (const auto* path = value.tryAs<std::string>(); path != nullptr) {
        if (path->empty()) {
            return tl::unexpected{fieldError(component, field, "an asset path cannot be empty")};
        }
        store<std::uint64_t>(bytes, field, core::asset::AssetId::fromPath(std::filesystem::path{*path}).value);
        return {};
    }
    const auto* userdata = value.tryAs<luau::Userdata>();
    const auto* id = userdata != nullptr ? userdata->tryAs<core::asset::AssetId>() : nullptr;
    if (id == nullptr) {
        return tl::unexpected{fieldError(component, field, "expected an asset path or an AssetId")};
    }
    store<std::uint64_t>(bytes, field, id->value);
    return {};
}

/// @return Whether a field type is a number a script reads and writes without a range check: every integer up to 32
/// bits and both floats.
bool isPlainNumber(const ecs::FieldType type) {
    switch (type) {
        case ecs::FieldType::kI8:
        case ecs::FieldType::kI16:
        case ecs::FieldType::kI32:
        case ecs::FieldType::kU8:
        case ecs::FieldType::kU16:
        case ecs::FieldType::kU32:
        case ecs::FieldType::kF32:
        case ecs::FieldType::kF64:
            return true;
        default:
            return false;
    }
}

/// @return The value of a plain number field (isPlainNumber()), as a Luau number.
double loadPlainNumber(const std::span<const std::byte> bytes, const ecs::FieldInfo& field) {
    switch (field.type) {
        case ecs::FieldType::kI8:
            return load<std::int8_t>(bytes, field);
        case ecs::FieldType::kI16:
            return load<std::int16_t>(bytes, field);
        case ecs::FieldType::kI32:
            return load<std::int32_t>(bytes, field);
        case ecs::FieldType::kU8:
            return load<std::uint8_t>(bytes, field);
        case ecs::FieldType::kU16:
            return load<std::uint16_t>(bytes, field);
        case ecs::FieldType::kU32:
            return load<std::uint32_t>(bytes, field);
        case ecs::FieldType::kF32:
            return load<float>(bytes, field);
        default:
            return load<double>(bytes, field);
    }
}

/// @brief Writes a number into a number field, checking that it fits.
Result<void> writeNumber(const std::string_view component, const ecs::FieldInfo& field,
                         const std::span<std::byte> bytes, const double number) {
    switch (field.type) {
        case ecs::FieldType::kI8:
            return writeInteger<std::int8_t>(component, field, bytes, number);
        case ecs::FieldType::kI16:
            return writeInteger<std::int16_t>(component, field, bytes, number);
        case ecs::FieldType::kI32:
            return writeInteger<std::int32_t>(component, field, bytes, number);
        case ecs::FieldType::kI64:
            return writeInteger<std::int64_t>(component, field, bytes, number);
        case ecs::FieldType::kU8:
            return writeInteger<std::uint8_t>(component, field, bytes, number);
        case ecs::FieldType::kU16:
            return writeInteger<std::uint16_t>(component, field, bytes, number);
        case ecs::FieldType::kU32:
            return writeInteger<std::uint32_t>(component, field, bytes, number);
        case ecs::FieldType::kU64:
            return writeInteger<std::uint64_t>(component, field, bytes, number);
        case ecs::FieldType::kF32:
            if (!fitsFloat(number)) {
                return tl::unexpected{fieldError(component, field, std::format("{} is out of range", number))};
            }
            store<float>(bytes, field, static_cast<float>(number));
            return {};
        case ecs::FieldType::kF64:
            store<double>(bytes, field, number);
            return {};
        default:
            break;
    }
    return tl::unexpected{fieldError(component, field, "this field type is not supported")};
}

Result<luau::Value> readScalar(const std::string_view component, const ecs::FieldInfo& field,
                               const std::span<const std::byte> bytes, const FieldReadContext& context) {
    if (auto supported = checkSupported(component, field, bytes); !supported) {
        return tl::unexpected{supported.error()};
    }
    switch (field.type) {
        case ecs::FieldType::kBool:
            return luau::Value{load<std::uint8_t>(bytes, field) != 0};
        case ecs::FieldType::kI8:
            return luau::Value{static_cast<double>(load<std::int8_t>(bytes, field))};
        case ecs::FieldType::kI16:
            return luau::Value{static_cast<double>(load<std::int16_t>(bytes, field))};
        case ecs::FieldType::kI32:
            return luau::Value{static_cast<double>(load<std::int32_t>(bytes, field))};
        case ecs::FieldType::kI64:
            return readInteger<std::int64_t>(component, field, bytes);
        case ecs::FieldType::kU8:
            return luau::Value{static_cast<double>(load<std::uint8_t>(bytes, field))};
        case ecs::FieldType::kU16:
            return luau::Value{static_cast<double>(load<std::uint16_t>(bytes, field))};
        case ecs::FieldType::kU32:
            return luau::Value{static_cast<double>(load<std::uint32_t>(bytes, field))};
        case ecs::FieldType::kU64:
            return readInteger<std::uint64_t>(component, field, bytes);
        case ecs::FieldType::kF32:
            return luau::Value{static_cast<double>(load<float>(bytes, field))};
        case ecs::FieldType::kF64:
            return luau::Value{load<double>(bytes, field)};
        case ecs::FieldType::kEntity: {
            auto userdata = context.entities.create(load<ecs::Entity>(bytes, field));
            if (!userdata) {
                return tl::unexpected{fieldError(component, field, userdata.error().message())};
            }
            return luau::Value{std::move(*userdata)};
        }
        case ecs::FieldType::kVec2:
        case ecs::FieldType::kVec3:
        case ecs::FieldType::kVec4:
            return readVector(component, field, bytes, context);
        case ecs::FieldType::kAssetId: {
            auto userdata = context.assetIds.create(core::asset::AssetId{.value = load<std::uint64_t>(bytes, field)});
            if (!userdata) {
                return tl::unexpected{fieldError(component, field, userdata.error().message())};
            }
            return luau::Value{std::move(*userdata)};
        }
        case ecs::FieldType::kString: {
            // The first byte of an ecs::String is its length, the text follows.
            const auto length = std::min<std::size_t>(load<std::uint8_t>(bytes, field), ecs::String::kCapacity);
            const auto* text = reinterpret_cast<const char*>(bytes.data() + field.offset + 1);  // NOLINT
            return luau::Value{std::string{text, length}};
        }
        case ecs::FieldType::kText:  // A byte of a std::string: only read whole, through its list.
            break;
    }
    return tl::unexpected{fieldError(component, field, "this field type is not supported")};
}

Result<void> writeScalar(const std::string_view component, const ecs::FieldInfo& field,
                         const std::span<std::byte> bytes, const luau::Value& value) {
    if (auto supported = checkSupported(component, field, bytes); !supported) {
        return supported;
    }
    if (field.type == ecs::FieldType::kBool) {
        const auto* flag = value.tryAs<bool>();
        if (flag == nullptr) {
            return tl::unexpected{fieldError(component, field, "expected a boolean")};
        }
        store<std::uint8_t>(bytes, field, *flag ? 1 : 0);
        return {};
    }
    if (field.type == ecs::FieldType::kEntity) {
        const auto* userdata = value.tryAs<luau::Userdata>();
        const auto* entity = userdata != nullptr ? userdata->tryAs<ecs::Entity>() : nullptr;
        if (entity == nullptr) {
            return tl::unexpected{fieldError(component, field, "expected an Entity")};
        }
        store<ecs::Entity>(bytes, field, *entity);
        return {};
    }
    if (field.type == ecs::FieldType::kVec2 || field.type == ecs::FieldType::kVec3 ||
        field.type == ecs::FieldType::kVec4) {
        return writeVector(component, field, bytes, value);
    }
    if (field.type == ecs::FieldType::kAssetId) {
        return writeAssetId(component, field, bytes, value);
    }
    if (field.type == ecs::FieldType::kString) {
        const auto* text = value.tryAs<std::string>();
        if (text == nullptr) {
            return tl::unexpected{fieldError(component, field, "expected a string")};
        }
        // A text longer than ecs::String::kCapacity is cut, as everywhere else.
        const ecs::String converted{*text};
        std::memcpy(bytes.subspan(field.offset).data(), &converted, sizeof(ecs::String));
        return {};
    }
    const auto* number = value.tryAs<double>();
    if (number == nullptr) {
        return tl::unexpected{fieldError(component, field, "expected a number")};
    }
    return writeNumber(component, field, bytes, *number);
}

/// @return The field of one value of the list @p field, at the start of its bytes.
ecs::FieldInfo elementOf(const ecs::FieldInfo& field) { return ecs::FieldInfo{.name = field.name, .type = field.type}; }

/// @return Nothing when @p bytes hold the vector of a list field, or the error saying why not.
Result<void> checkList(const std::string_view component, const ecs::FieldInfo& field,
                       const std::span<const std::byte> bytes) {
    if (field.list->size > bytes.size() || field.offset > bytes.size() - field.list->size) {
        return tl::unexpected{fieldError(component, field, "the field is out of the component's bytes")};
    }
    return {};
}

/// @return The values of a list field, as a Luau table: a copy, not a view of the component.
Result<luau::Value> readList(const std::string_view component, const ecs::FieldInfo& field,
                             const std::span<const std::byte> bytes, const FieldReadContext& context) {
    if (auto checked = checkList(component, field, bytes); !checked) {
        return tl::unexpected{checked.error()};
    }
    if (context.runtime == nullptr) {
        return tl::unexpected{fieldError(component, field, "no runtime to build a table with")};
    }
    const std::span<const std::byte> values = field.list->elements(bytes.subspan(field.offset).data());
    if (field.type == ecs::FieldType::kText) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): the bytes of a std::string are its characters
        return luau::Value{std::string{reinterpret_cast<const char*>(values.data()), values.size()}};
    }
    const std::size_t size = ecs::getFieldLayout(field.type).size;
    auto table = luau::Table::create(*context.runtime, static_cast<int>(values.size() / size), 0);
    if (!table) {
        return tl::unexpected{fieldError(component, field, table.error().message())};
    }
    const ecs::FieldInfo element = elementOf(field);
    for (std::size_t index = 0; index < values.size() / size; ++index) {
        auto value = readScalar(component, element, values.subspan(index * size, size), context);
        if (!value) {
            return value;
        }
        if (auto set = table->setAt(index + 1, *value); !set) {
            return tl::unexpected{fieldError(component, field, set.error().message())};
        }
    }
    return luau::Value{std::move(*table)};
}

/// @brief Replaces the values of a list field by those of a Luau array, each checked as a field of its type.
Result<void> writeList(const std::string_view component, const ecs::FieldInfo& field, const std::span<std::byte> bytes,
                       const luau::Value& value) {
    if (auto checked = checkList(component, field, bytes); !checked) {
        return checked;
    }
    if (field.type == ecs::FieldType::kText) {
        const auto* text = value.tryAs<std::string>();
        if (text == nullptr) {
            return tl::unexpected{fieldError(component, field, "expected a string")};
        }
        field.list->assign(bytes.subspan(field.offset).data(), std::as_bytes(std::span{*text}));
        return {};
    }
    const auto* table = value.tryAs<luau::Table>();
    if (table == nullptr) {
        return tl::unexpected{fieldError(component, field, "expected a table of values")};
    }
    auto entries = table->entries();
    if (!entries) {
        return tl::unexpected{fieldError(component, field, entries.error().message())};
    }
    const std::size_t size = ecs::getFieldLayout(field.type).size;
    std::vector<std::byte> values(entries->size() * size);
    const ecs::FieldInfo element = elementOf(field);
    for (std::size_t index = 0; index < entries->size(); ++index) {
        auto item = table->getAt(index + 1);
        if (!item || item->tryAs<luau::Value::Nil>() != nullptr) {
            return tl::unexpected{fieldError(component, field, "expected an array, without holes or named keys")};
        }
        if (auto written = writeScalar(component, element, std::span{values}.subspan(index * size, size), *item);
            !written) {
            return written;
        }
    }
    field.list->assign(bytes.subspan(field.offset).data(), values);
    return {};
}
}  // namespace

Result<luau::Value> readField(const std::string_view component, const ecs::FieldInfo& field,
                              const std::span<const std::byte> bytes, const FieldReadContext& context) {
    return field.list != nullptr ? readList(component, field, bytes, context)
                                 : readScalar(component, field, bytes, context);
}

Result<void> writeField(const std::string_view component, const ecs::FieldInfo& field, const std::span<std::byte> bytes,
                        const luau::Value& value) {
    return field.list != nullptr ? writeList(component, field, bytes, value)
                                 : writeScalar(component, field, bytes, value);
}

Result<void> pushField(const std::string_view component, const ecs::FieldInfo& field,
                       const std::span<const std::byte> bytes, const FieldReadContext& context, luau::Pusher& result) {
    // Numbers, tables and userdata are pushed as they are made: a Value would be built and destroyed around each, and
    // a table or a userdata held through the registry first.
    const bool fast =
        field.list == nullptr &&
        (isPlainNumber(field.type) || field.type == ecs::FieldType::kBool || field.type == ecs::FieldType::kVec2 ||
         field.type == ecs::FieldType::kVec4 || field.type == ecs::FieldType::kEntity);
    if (!fast) {
        auto value = readField(component, field, bytes, context);
        if (!value) {
            return tl::unexpected{std::move(value.error())};
        }
        result.value(*value);
        return {};
    }
    if (auto supported = checkSupported(component, field, bytes); !supported) {
        return supported;
    }
    if (isPlainNumber(field.type)) {
        result.number(loadPlainNumber(bytes, field));
        return {};
    }
    if (field.type == ecs::FieldType::kBool) {
        result.boolean(load<std::uint8_t>(bytes, field) != 0);
        return {};
    }
    if (field.type == ecs::FieldType::kEntity) {
        result.userdata(context.entities, load<ecs::Entity>(bytes, field));
        return {};
    }
    const auto keys = vectorKeys(field.type);
    std::array<float, 4> components{};
    std::memcpy(components.data(), bytes.subspan(field.offset, keys.size() * sizeof(float)).data(),
                keys.size() * sizeof(float));
    std::array<double, 4> numbers{};
    std::ranges::copy(components, numbers.begin());
    result.record(keys, std::span{numbers}.first(keys.size()));
    return {};
}

Result<void> writeFieldFrom(const std::string_view component, const ecs::FieldInfo& field,
                            const std::span<std::byte> bytes, const luau::StackSlot& value) {
    if (field.list == nullptr &&
        (isPlainNumber(field.type) || field.type == ecs::FieldType::kI64 || field.type == ecs::FieldType::kU64)) {
        if (const auto number = value.number()) {
            if (auto supported = checkSupported(component, field, bytes); !supported) {
                return supported;
            }
            return writeNumber(component, field, bytes, *number);
        }
    }
    if (field.list == nullptr && field.type == ecs::FieldType::kBool) {
        if (const auto flag = value.boolean()) {
            if (auto supported = checkSupported(component, field, bytes); !supported) {
                return supported;
            }
            store<std::uint8_t>(bytes, field, *flag ? 1 : 0);
            return {};
        }
    }
    if (field.list != nullptr || !isVector(field.type) || !value.isTable()) {
        // Anything else, and every wrong type, goes the general way: same values, same messages.
        return writeField(component, field, bytes, value.value());
    }
    if (auto supported = checkSupported(component, field, bytes); !supported) {
        return supported;
    }
    // The table is visited where it is: no copy of it, nor of its keys.
    return writeVectorEntries(component, field, bytes, [&value](const auto& visit) {
        value.forEachEntry([&visit](const std::optional<std::string_view> key, const luau::StackSlot& entry) {
            const std::optional<double> number = entry.number();
            return visit(key, number ? &*number : nullptr);
        });
    });
}

Result<void> fillFromTable(const ecs::TypeInfo& type, const luau::Table& table, const std::span<std::byte> bytes) {
    auto entries = table.entries();
    if (!entries) {
        return fail(ErrorKind::kInvalidValue,
                    std::format("component '{}': cannot read the table: {}", type.name, entries.error().message()));
    }
    for (const auto& [key, value] : *entries) {
        const auto* name = key.tryAs<std::string>();
        if (name == nullptr) {
            return fail(ErrorKind::kInvalidValue,
                        std::format("component '{}': field names must be strings", type.name));
        }
        const auto field = std::ranges::find(type.fields, *name, &ecs::FieldInfo::name);
        if (field == type.fields.end()) {
            return fail(ErrorKind::kInvalidValue, std::format("component '{}' has no field '{}'", type.name, *name));
        }
        if (auto written = writeField(type.name, *field, bytes, value); !written) {
            return written;
        }
    }
    return {};
}
}  // namespace aylo::scripting::view
