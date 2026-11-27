/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TypeDefinitions
*/

#include "TypeDefinitions.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "aylo/ecs/call/CallInfo.hpp"
#include "aylo/ecs/call/Calls.hpp"
#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/event/EventInfo.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"
#include "aylo/ecs/type/TypeRegistry.hpp"
#include "aylo/luau/FunctionRef.hpp"
#include "aylo/luau/Table.hpp"
#include "aylo/luau/Value.hpp"

namespace aylo::scripting::types {
namespace {
/// @brief What every definition file starts with: the classes of the engine, and the types `ctx` and `self` use
/// whatever the game. `TypesPrelude.d.luau`, embedded as a byte array, so it is edited as a Luau file.
constexpr auto kPreludeBytes = std::to_array<char>({
#include "TypesPrelude.d.luau.h"
});
constexpr std::string_view kPrelude{kPreludeBytes.data(), kPreludeBytes.size() - 1};

/// @brief The reserved words of Luau: a field of one of these names is written `["end"]`.
constexpr std::array<std::string_view, 21> kKeywords{"and", "break",    "do",     "else", "elseif", "end",   "false",
                                                     "for", "function", "if",     "in",   "local",  "nil",   "not",
                                                     "or",  "repeat",   "return", "then", "true",   "until", "while"};

/// @return Whether @p name can be written bare as a key of a Luau table type.
[[nodiscard]] bool isIdentifier(const std::string_view name) {
    if (std::ranges::find(kKeywords, name) != kKeywords.end()) {
        return false;
    }
    const auto isStart = [](char letter) {
        return (letter >= 'a' && letter <= 'z') || (letter >= 'A' && letter <= 'Z') || letter == '_';
    };
    const auto isRest = [&isStart](char letter) { return isStart(letter) || (letter >= '0' && letter <= '9'); };
    return !name.empty() && isStart(name.front()) && std::ranges::all_of(name.substr(1), isRest);
}

/// @return @p name as the key of a table type: bare, or `["name"]`.
[[nodiscard]] std::string keyOf(const std::string_view name) {
    return isIdentifier(name) ? std::string{name} : std::format("[\"{}\"]", name);
}

/// @return The Luau type of one value of a field type, as a script reads it.
[[nodiscard]] std::string_view scalarOf(const ecs::FieldType type) {
    switch (type) {
        case ecs::FieldType::kBool:
            return "boolean";
        case ecs::FieldType::kVec2:
            return "Vec2";
        case ecs::FieldType::kVec3:
            return "vector";
        case ecs::FieldType::kVec4:
            return "Vec4";
        case ecs::FieldType::kEntity:
            return "Entity";
        case ecs::FieldType::kAssetId:
            return "AssetId";
        case ecs::FieldType::kText:
        case ecs::FieldType::kString:
            return "string";
        default:
            return "number";
    }
}

/// @return The Luau type a script writes to a field: what it reads, and a path for an asset, a table for a `vec3`.
[[nodiscard]] std::string writeTypeOf(const ecs::FieldInfo& field) {
    if (field.enumNames.empty() && field.units.empty() && field.list == nullptr && field.count == 1) {
        if (field.type == ecs::FieldType::kAssetId) {
            return "AssetId | string";
        }
        if (field.type == ecs::FieldType::kVec3) {
            return "vector | Vec3";
        }
    }
    return readTypeOf(field);
}

/// @return The fields as a table type: each field read as it is, or, for @p written, each optional and written.
[[nodiscard]] std::string recordOf(const std::span<const ecs::FieldInfo> fields, const bool written) {
    if (fields.empty()) {
        return "{}";
    }
    std::string record = "{ ";
    bool first = true;
    for (const ecs::FieldInfo& field : fields) {
        record += std::format("{}{}: {}", first ? "" : ", ", keyOf(field.name),
                              written ? std::format("({})?", writeTypeOf(field)) : readTypeOf(field));
        first = false;
    }
    return record + " }";
}

/// @return Whether a call gives a script nothing.
[[nodiscard]] bool givesNothing(const ecs::CallInfo& call) { return !call.result || call.result->fields.empty(); }

/// @return What a call gives a script, as a type: `nil` (nothing), its one field, or a table of its fields.
[[nodiscard]] std::string resultOf(const ecs::CallInfo& call) {
    if (!call.result || call.result->fields.empty()) {
        return "nil";
    }
    const std::vector<ecs::FieldInfo>& fields = call.result->fields;
    return fields.size() == 1 ? readTypeOf(fields.front()) : recordOf(fields, false);
}

/// @return @p names as a union of string literals, or `string` when there are none.
[[nodiscard]] std::string unionOf(const std::vector<std::string>& names) {
    if (names.empty()) {
        return "string";
    }
    std::string text;
    for (const std::string& name : names) {
        text += std::format("{}\"{}\"", text.empty() ? "" : " | ", name);
    }
    return text;
}

/// @return The overloads @p entries, then the one any other name takes, as one intersection of function types.
[[nodiscard]] std::string overloadsOf(const std::vector<std::string>& entries, const std::string_view fallback) {
    // Luau takes the first overload that fits: the one of any name goes last.
    std::string text;
    for (const std::string& entry : entries) {
        text += std::format("({})\n\t& ", entry);
    }
    return std::format("{}({})", text, fallback);
}

/// @return The types of @p registry, by name.
template <typename Info>
[[nodiscard]] std::vector<const Info*> sorted(const ecs::TypeRegistry<Info>& registry) {
    std::vector<const Info*> infos;
    for (std::size_t index = 0; index < registry.getCount(); ++index) {
        if (const Info* info = registry.findById(static_cast<ecs::TypeId>(index))) {
            infos.push_back(info);
        }
    }
    std::ranges::sort(infos, {}, [](const Info* info) -> const std::string& { return info->type.name; });
    return infos;
}

/// @brief The parts of the file that depend on the game: its components, events and calls.
class Writer {
  public:
    explicit Writer(const TypeSources& sources) : _sources{&sources} {}

    [[nodiscard]] std::string write() {
        std::string text{kPrelude};
        text += components();
        text += events();
        text += calls();
        text += std::format("\ndeclare engine: {}\n", walk(*_sources->engine, "engine", 0));
        return text;
    }

  private:
    /// @return The view type of each component, the name union and the overloads of `ctx:get` and `getComponent`.
    [[nodiscard]] std::string components() const {
        std::string text = "\n-- Components: the view of each.\n";
        std::vector<std::string> gets;
        std::vector<std::string> selfGets;
        for (const ecs::ComponentInfo* info : sorted(*_sources->components)) {
            const std::string& name = info->type.name;
            const std::string type = typeNameOf("C_", name);
            text += std::format("type {} = {}\n", type, recordOf(info->type.fields, false));
            gets.push_back(std::format("(self: any, entity: Entity, name: \"{}\") -> {}?", name, type));
            selfGets.push_back(std::format("(self: any, name: \"{}\") -> {}?", name, type));
        }
        text +=
            std::format("type CtxGet = {}\n", overloadsOf(gets, "(self: any, entity: Entity, name: string) -> any"));
        text += std::format("type SelfGet = {}\n", overloadsOf(selfGets, "(self: any, name: string) -> any"));
        return text;
    }

    /// @return The type of each event, the name union and the overloads of `ctx.events.read` and `send`.
    [[nodiscard]] std::string events() const {
        std::string text = "\n-- Events: what read gives and send takes.\n";
        std::vector<std::string> reads;
        std::vector<std::string> sends;
        for (const ecs::EventInfo* info : sorted(*_sources->events)) {
            const std::string& name = info->type.name;
            const std::string type = typeNameOf("E_", name);
            text += std::format("type {} = {}\n", type, recordOf(info->type.fields, false));
            reads.push_back(std::format("(name: \"{}\") -> () -> {}?", name, type));
            sends.push_back(std::format("(name: \"{}\", fields: {}?) -> ()", name, recordOf(info->type.fields, true)));
        }
        text += std::format("type EventRead = {}\n", overloadsOf(reads, "(name: string) -> () -> any"));
        text += std::format("type EventSend = {}\n", overloadsOf(sends, "(name: string, fields: Fields?) -> ()"));
        return text;
    }

    /// @return The result type of each call, the name union and the overloads of `ctx.call` and `engine.call`.
    [[nodiscard]] std::string calls() {
        std::string text = "\n-- Engine calls: what each takes and gives.\n";
        std::vector<const ecs::CallInfo*> calls;
        for (std::size_t index = 0; index < _sources->calls->getCount(); ++index) {
            if (const ecs::CallInfo* call = _sources->calls->findById(static_cast<ecs::CallId>(index))) {
                calls.push_back(call);
            }
        }
        std::ranges::sort(calls, {}, &ecs::CallInfo::name);
        std::vector<std::string> overloads;
        for (const ecs::CallInfo* call : calls) {
            const std::string result = typeNameOf("R_", call->name);
            text += std::format("type {} = {}\n", result, resultOf(*call));
            _results.insert_or_assign(call->name, result);
            const std::string arguments = call->arguments ? recordOf(call->arguments->fields, true) : std::string{"{}"};
            overloads.push_back(std::format("(name: \"{}\", args: {}?) -> {}", call->name, arguments,
                                            givesNothing(*call) ? std::string{"()"} : result));
        }
        text += std::format("type Call = {}\n", overloadsOf(overloads, "(name: string, args: Fields?) -> any"));
        return text;
    }

    /// @return The result type of the call @p name, or `any` when the game does not have it.
    [[nodiscard]] std::string resultOr(const std::string& name) const {
        const auto found = _results.find(name);
        return found == _results.end() ? std::string{"any"} : found->second;
    }

    /// @return The type of the function at @p path: the one of a known form, or one taking and giving anything.
    [[nodiscard]] std::string functionAt(const std::string& path) const {
        const std::map<std::string_view, std::string> known{
            {"engine.call", "Call"},
            {"engine.behaviour",
             "(name: string, spec: { fields: { [string]: string }, defaults: Fields?, replication: string?, presence: "
             "string?, side: Side? }) -> Self"},
            {"engine.ecs.component",
             "(name: string, spec: { fields: { [string]: string }, defaults: Fields?, replication: string?, presence: "
             "string? }) -> ()"},
            {"engine.ecs.event", "(name: string, spec: { fields: { [string]: string } }) -> ()"},
            {"engine.ecs.system", "(name: string, spec: SystemSpec) -> ()"},
            {"engine.ecs.state", "(name: string, values: { string }, options: { replicate: boolean? }?) -> ()"},
            {"engine.state.get", "(ctx: CtxLike, machine: string) -> string"},
            {"engine.state.set", "(ctx: CtxLike, machine: string, value: string) -> ()"},
            {"engine.input.read",
             std::format("((ctx: CtxLike, action: string) -> {}) & ((ctx: CtxLike, entity: Entity, "
                         "action: string) -> {})",
                         resultOr("aylo.input.read"), resultOr("aylo.replication.readInput"))},
            {"engine.audio.play",
             "(ctx: CtxLike, sound: AssetId | string, options: { bus: string?, volume: number?, pitch: number?, "
             "position: Vec2?, loop: boolean?, startAt: number?, lowpass: number? }?) -> number"},
            {"engine.net.localClient", "(ctx: CtxLike) -> { connected: boolean, client: number }"},
            {"engine.physics.moveAndSlide",
             std::format("(ctx: CtxLike, entity: Entity, from: Vec2, delta: Vec2, options: {{ skin: number?, "
                         "maxSlides: number? }}?) -> {}",
                         resultOr("aylo.physics.moveAndSlide"))},
            {"engine.capsule.spawn", "(ctx: CtxLike, path: string, overrides: { [string]: Fields }?) -> Entity"},
            {"engine.capsule.load", "(ctx: CtxLike, path: string, overrides: { [string]: Fields }?) -> Entity"},
            {"engine.capsule.reload", "(ctx: CtxLike, root: Entity) -> boolean"},
            {"engine.assetId", "(path: string) -> AssetId"},
            {"engine.hasCall", "(name: string) -> boolean"},
        };
        const auto found = known.find(path);
        return found == known.end() ? std::string{"(...any) -> ...any"} : found->second;
    }

    /// @return The type of the table @p table at @p path, its keys sorted; the helpers of the forms are left out.
    // NOLINTNEXTLINE(misc-no-recursion): the depth is the depth of the `engine` tree.
    [[nodiscard]] std::string walk(const luau::Table& table, const std::string& path, const std::size_t depth) {
        const auto entries = table.entries();
        if (!entries) {
            return "{ [string]: any }";
        }
        std::map<std::string, std::string> fields;
        for (const auto& [key, value] : *entries) {
            const auto* name = key.tryAs<std::string>();
            if (name == nullptr || !isIdentifier(*name) || (depth == 0 && *name == "forms")) {
                continue;
            }
            fields.insert_or_assign(*name, typeOfValue(value, std::format("{}.{}", path, *name), depth + 1));
        }
        const std::string indent(depth + 1, '\t');
        std::string text = "{\n";
        for (const auto& [name, type] : fields) {
            text += std::format("{}{}: {},\n", indent, name, type);
        }
        return text + std::string(depth, '\t') + "}";
    }

    /// @return The type of one value of the `engine` tree.
    // NOLINTNEXTLINE(misc-no-recursion): see walk().
    [[nodiscard]] std::string typeOfValue(const luau::Value& value, const std::string& path, const std::size_t depth) {
        if (const auto* nested = value.tryAs<luau::Table>()) {
            return walk(*nested, path, depth);
        }
        if (value.is<luau::FunctionRef>()) {
            return functionAt(path);
        }
        if (value.is<bool>()) {
            return "boolean";
        }
        if (value.is<double>()) {
            return "number";
        }
        if (value.is<std::string>()) {
            return "string";
        }
        return "any";
    }

    const TypeSources* _sources;                  ///< What the file describes.
    std::map<std::string, std::string> _results;  ///< The result type of each call, by name.
};
}  // namespace

std::string typeNameOf(const std::string_view prefix, const std::string_view name) {
    std::string type{prefix};
    for (const char letter : name) {
        const bool kept = (letter >= 'a' && letter <= 'z') || (letter >= 'A' && letter <= 'Z') ||
                          (letter >= '0' && letter <= '9') || letter == '_';
        type += kept ? letter : '_';
    }
    return type;
}

std::string readTypeOf(const ecs::FieldInfo& field) {
    if (!field.enumNames.empty()) {
        const std::vector<std::string> names{field.enumNames.begin(), field.enumNames.end()};
        return unionOf(names);
    }
    if (!field.units.empty()) {
        return "number | string";
    }
    if (field.type == ecs::FieldType::kText) {
        return "string";
    }
    if (field.list != nullptr) {
        return "{ any }";
    }
    const std::string_view scalar = scalarOf(field.type);
    return field.count > 1 ? std::format("{{ {} }}", scalar) : std::string{scalar};
}

std::string generateDefinitions(const TypeSources& sources) { return Writer{sources}.write(); }
}  // namespace aylo::scripting::types
