/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** CapsuleFiles
*/

#include "CapsuleFiles.hpp"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <format>
#include <memory>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "aylo/ecs/component/Components.hpp"
#include "core/asset/AssetFiles.hpp"
#include "core/asset/exceptions/AssetExceptions.hpp"
#include "core/vfs/DirectorySource.hpp"
#include "core/vfs/FileSource.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"
#include "plugins/capsule/format/CapsuleText.hpp"

namespace aylo::plugins::capsule {
CapsuleFiles::CapsuleFiles(std::filesystem::path assetDirectory, const ecs::Components& components)
    : CapsuleFiles{std::make_shared<core::vfs::DirectorySource>(std::move(assetDirectory)), components} {}

CapsuleFiles::CapsuleFiles(std::shared_ptr<const core::vfs::FileSource> assets, const ecs::Components& components)
    : _assets{std::move(assets)}, _components{&components} {}

std::vector<CapsuleId> CapsuleFiles::loadAll(Capsules& capsules) {
    const std::vector<std::string> files = scan();
    std::set<std::string> pending{files.begin(), files.end()};
    std::vector<CapsuleId> read;
    for (const std::string& path : files) {
        if (pending.contains(path)) {
            this->read(capsules, path, pending, read);
        }
    }
    return read;
}

CapsuleFiles::Reread CapsuleFiles::reload(Capsules& capsules, std::span<const std::string> paths) {
    std::set<std::string> pending;
    for (const std::string& path : paths) {
        if (format::isCapsuleFile(path)) {
            pending.insert(path);
        }
    }
    for (bool grew = !pending.empty(); grew;) {  // and the files that instance them, at any depth
        grew = false;
        for (const auto& [path, uses] : _uses) {
            if (pending.contains(path)) {
                continue;
            }
            for (const std::string& used : uses) {
                if (pending.contains(used)) {
                    grew = pending.insert(path).second || grew;
                    break;
                }
            }
        }
    }
    Reread reread;
    while (!pending.empty()) {
        const std::string path = *pending.begin();
        try {
            read(capsules, path, pending, reread.capsules);
        } catch (const std::exception& error) {  // the old version stays: fix the file and save it again
            pending.erase(path);
            reread.errors.emplace_back(error.what());
        }
    }
    return reread;
}

std::vector<std::string> CapsuleFiles::scan() const {
    std::vector<std::string> files;
    for (const std::string& file : _assets->list("")) {
        if (format::isCapsuleFile(std::filesystem::path{file})) {
            files.push_back(core::asset::normalizePath(file));
        }
    }
    std::ranges::sort(files);
    return files;
}

// NOLINTNEXTLINE(misc-no-recursion): each file is read once per pass, `pending` stops a file that instances itself.
void CapsuleFiles::read(Capsules& capsules, const std::string& path, std::set<std::string>& pending,
                        std::vector<CapsuleId>& read) {
    pending.erase(path);
    std::string text;
    try {
        text = _assets->read(path);
    } catch (const core::asset::exceptions::AssetLoadException& error) {
        throw exceptions::CapsuleException{std::format("{}: {}", path, error.what())};
    }
    std::set<std::string> uses;
    Capsule capsule = format::readCapsule(path, text, *_components, capsules, [&](std::string_view used) {
        const std::string instanced{used};
        uses.insert(instanced);
        if (pending.contains(instanced)) {
            // NOLINTNEXTLINE(misc-no-recursion): the instanced file's values are the base of this file's overrides
            this->read(capsules, instanced, pending, read);
        }
    });
    read.push_back(capsules.define(std::move(capsule)).getId());
    _uses.insert_or_assign(path, std::move(uses));
}
}  // namespace aylo::plugins::capsule
