/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** CallBuffer
*/

#include "aylo/ecs/call/CallBuffer.hpp"

#include <algorithm>
#include <cstddef>
#include <new>
#include <optional>

#include "aylo/ecs/type/TypeInfo.hpp"

namespace aylo::ecs {

CallBuffer::CallBuffer(const std::optional<TypeInfo>& type) {
    if (!type || type->size == 0) {
        return;
    }
    _alignment = std::align_val_t{std::max<std::size_t>(type->alignment, 1)};
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): released by the destructor, with the same alignment.
    _data = static_cast<std::byte*>(::operator new(type->size, _alignment));
    _size = type->size;
    std::fill_n(_data, _size, std::byte{0});
    if (type->hooks.construct != nullptr) {
        try {
            type->hooks.construct(_data);
        } catch (...) {
            ::operator delete(_data, _alignment);
            throw;
        }
    }
    _destruct = type->hooks.destruct;
}

CallBuffer::~CallBuffer() {
    if (_data == nullptr) {
        return;
    }
    if (_destruct != nullptr) {
        _destruct(_data);
    }
    ::operator delete(_data, _alignment);
}

}  // namespace aylo::ecs
