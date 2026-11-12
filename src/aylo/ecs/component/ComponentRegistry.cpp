/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ComponentRegistry
*/

#include "ComponentRegistry.hpp"

#include <string_view>
#include <tl/expected.hpp>

#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/error/Error.hpp"
#include "aylo/ecs/error/Result.hpp"
#include "aylo/ecs/type/TypeRegistry.hpp"

namespace aylo::ecs {
namespace {
[[nodiscard]] std::string_view toString(StorageKind storage) noexcept {
    switch (storage) {
        case StorageKind::kSparseSet:
            return "sparseSet";
        case StorageKind::kTable:
            return "table";
    }
    return "unknown";
}
}  // namespace

Result<void> checkSameStorage(const ComponentInfo& registered, const ComponentInfo& requested) {
    if (registered.storage != requested.storage) {
        return tl::unexpected<Error>{detail::optionsMismatch(KindTraits<ComponentInfo>::kLabel, requested.type.name,
                                                             "storage", toString(registered.storage),
                                                             toString(requested.storage))};
    }
    return {};
}
}  // namespace aylo::ecs
