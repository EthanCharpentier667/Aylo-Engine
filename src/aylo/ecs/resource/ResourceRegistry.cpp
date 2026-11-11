/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ResourceRegistry
*/

#include "ResourceRegistry.hpp"

#include <string_view>
#include <tl/expected.hpp>

#include "aylo/ecs/error/Error.hpp"
#include "aylo/ecs/error/Result.hpp"
#include "aylo/ecs/resource/ResourceInfo.hpp"
#include "aylo/ecs/type/TypeRegistry.hpp"

namespace aylo::ecs {
namespace {
[[nodiscard]] std::string_view toThread(bool nonSend) noexcept { return nonSend ? "main" : "any"; }
}  // namespace

Result<void> checkSameThread(const ResourceInfo& registered, const ResourceInfo& requested) {
    if (registered.nonSend != requested.nonSend) {
        return tl::unexpected<Error>{detail::optionsMismatch(KindTraits<ResourceInfo>::kLabel, requested.type.name,
                                                             "thread", toThread(registered.nonSend),
                                                             toThread(requested.nonSend))};
    }
    return {};
}
}  // namespace aylo::ecs
