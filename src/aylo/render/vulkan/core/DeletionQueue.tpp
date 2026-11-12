/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** DeletionQueue
*/

#include <cstddef>
#include <memory>
#include <utility>

namespace aylo::render::vulkan::core {

template <class Resource>
void DeletionQueue::defer(std::size_t frameIndex, Resource resource) {
    bucketOf(frameIndex).push_back(std::make_unique<ResourceEntry<Resource>>(std::move(resource)));
}

}  // namespace aylo::render::vulkan::core
