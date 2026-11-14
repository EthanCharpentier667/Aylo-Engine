/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TypeTraits
*/

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "aylo/ecs/type/FieldTypeOf.hpp"

namespace aylo::ecs {
namespace detail {
template <typename Member>
constexpr FieldDescription describeField(std::string_view name, std::size_t offset) {
    using Element = FieldElement<Member>::Type;
    static_assert(
        HasFieldType<Element>,
        "AYLO_ECS: a listed field has a type with no FieldType. Use a supported type (bool, fixed-size integers, "
        "float, double, Entity, or a std::array or std::vector of them) or specialize aylo::ecs::FieldTypeOf.");
    if constexpr (HasFieldType<Element>) {
        std::span<const std::string_view> enumNames;
        if constexpr (HasEnumNames<Element>) {
            enumNames = FieldEnumNames<Element>::kNames;
        }
        return FieldDescription{.name = name,
                                .type = FieldTypeOf<Element>::value,
                                .offset = static_cast<std::uint32_t>(offset),
                                .count = FieldElement<Member>::kCount,
                                .list = FieldElement<Member>::kList,
                                .enumNames = enumNames};
    } else {
        return FieldDescription{};  // Not reached: the static_assert above already failed; avoids a second error.
    }
}
}  // namespace detail

template <Described T>
TypeInfo TypeTraits<T>::makeTypeInfo() {
    TypeInfo info{
        .name = std::string{kName}, .size = kSize, .alignment = kAlignment, .hooks = kHooks, .copyable = kCopyable};
    info.fields.reserve(kFields.size());
    for (const FieldDescription& field : kFields) {
        info.fields.push_back(FieldInfo{.name = std::string{field.name},
                                        .type = field.type,
                                        .offset = field.offset,
                                        .count = field.count,
                                        .list = field.list,
                                        .enumNames = field.enumNames});
    }
    return info;
}
}  // namespace aylo::ecs
