/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AudioAssets
*/

#include "plugins/audio/AudioAssets.hpp"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "aylo/contract/audiodevice/AudioTypes.hpp"
#include "aylo/contract/audiodevice/IAudioDevice.hpp"
#include "aylo/contract/audiodevice/exceptions/AudioExceptions.hpp"
#include "core/asset/AssetFiles.hpp"
#include "core/asset/AssetId.hpp"
#include "core/vfs/DirectorySource.hpp"
#include "core/vfs/FileSource.hpp"
#include "plugins/audio/SoundProbe.hpp"
#include "plugins/audio/detail/ImportFile.hpp"

namespace aylo::plugins::audio {
namespace {
/// @brief Reads the header of @p entry's bytes, or of its file in @p source, into Entry::probed, unless it was read
/// already.
void probeEntry(const core::vfs::FileSource& source, AudioAssets::Entry& entry) {
    if (entry.probed) {
        return;
    }
    if (entry.encoded.empty() && !source.exists(entry.path)) {
        entry.probed = SoundProbe{.error = std::format("file not found: {}", entry.path)};
        return;
    }
    try {
        entry.probed =
            entry.encoded.empty() ? probeSound(core::vfs::readBytes(source, entry.path)) : probeSound(entry.encoded);
        if (!entry.probed->info) {
            entry.probed->error = std::format("{}: {}", entry.path, entry.probed->error);
        }
    } catch (const std::exception& error) {
        entry.probed = SoundProbe{.error = error.what()};
    }
}
}  // namespace

void AudioAssets::setDirectory(std::filesystem::path directory) {
    _source = std::make_shared<core::vfs::DirectorySource>(std::move(directory));
}

void AudioAssets::setSource(std::shared_ptr<const core::vfs::FileSource> source) {
    _source = source ? std::move(source) : std::make_shared<core::vfs::DirectorySource>(std::filesystem::path{});
}

const core::vfs::FileSource& AudioAssets::getSource() const noexcept { return *_source; }

std::size_t AudioAssets::getCount() const noexcept { return _entries.size(); }

core::asset::AssetId AudioAssets::load(const std::filesystem::path& path,
                                       const contract::audiodevice::SoundDesc& desc) {
    std::string normalized = core::asset::normalizePath(path);
    if (get(core::asset::AssetId::fromPath(normalized)) != nullptr) {
        return request(std::move(normalized), {}, desc);  // asked for already: the first request wins
    }
    const contract::audiodevice::SoundDesc options = detail::applyImportFile(*_source, normalized, desc);
    return request(std::move(normalized), {}, options);
}

core::asset::AssetId AudioAssets::load(const std::string& name, std::span<const std::byte> encoded,
                                       const contract::audiodevice::SoundDesc& desc) {
    return request(core::asset::normalizePath(name), {encoded.begin(), encoded.end()}, desc);
}

const AudioAssets::Entry* AudioAssets::get(core::asset::AssetId id) const {
    const auto found = _entries.find(id);
    return found != _entries.end() ? &found->second : nullptr;
}

std::optional<contract::audiodevice::SoundId> AudioAssets::getSound(core::asset::AssetId id) const {
    const Entry* entry = get(id);
    return entry != nullptr && entry->state == SoundState::kLoaded ? std::optional{entry->sound} : std::nullopt;
}

std::optional<SoundProbe> AudioAssets::probe(core::asset::AssetId id) {
    const auto found = _entries.find(id);
    if (found == _entries.end()) {
        return std::nullopt;
    }
    Entry& entry = found->second;
    if (entry.state == SoundState::kFailed) {
        return SoundProbe{.error = entry.error};
    }
    probeEntry(*_source, entry);
    return entry.probed;
}

bool AudioAssets::unload(core::asset::AssetId id) {
    const auto found = _entries.find(id);
    if (found == _entries.end()) {
        return false;
    }
    if (found->second.state == SoundState::kLoaded) {
        _unloaded.push_back(found->second.sound);
    }
    std::erase(_queued, id);
    _entries.erase(found);
    return true;
}

const std::vector<core::asset::AssetId>& AudioAssets::getQueued() const noexcept { return _queued; }

void AudioAssets::setLoaded(core::asset::AssetId id, contract::audiodevice::SoundId sound) {
    if (Entry* entry = dequeue(id)) {
        if (!entry->encoded.empty()) {
            probeEntry(*_source, *entry);  // while its bytes are in memory: they are freed below
        }
        entry->state = SoundState::kLoaded;
        entry->sound = sound;
        entry->encoded = {};
    }
}

void AudioAssets::setFailed(core::asset::AssetId id, std::string error) {
    if (Entry* entry = dequeue(id)) {
        entry->state = SoundState::kFailed;
        entry->error = std::move(error);
        entry->encoded = {};
    }
}

std::vector<contract::audiodevice::SoundId> AudioAssets::takeUnloaded() { return std::exchange(_unloaded, {}); }

core::asset::AssetId AudioAssets::request(std::string path, std::vector<std::byte> encoded,
                                          const contract::audiodevice::SoundDesc& desc) {
    const core::asset::AssetId id = core::asset::AssetId::fromPath(path);
    if (const Entry* known = get(id)) {
        if (known->path != path) {
            throw contract::audiodevice::exceptions::AudioLoadException{
                std::format("Sound '{}' has the same id as '{}': rename one of them", path, known->path)};
        }
        return id;
    }
    _entries.emplace(id, Entry{.path = std::move(path), .desc = desc, .encoded = std::move(encoded)});
    _queued.push_back(id);
    return id;
}

AudioAssets::Entry* AudioAssets::dequeue(core::asset::AssetId id) {
    const auto queued = std::ranges::find(_queued, id);
    if (queued == _queued.end()) {
        return nullptr;
    }
    _queued.erase(queued);
    return &_entries.at(id);
}

void updateSounds(AudioAssets& assets, contract::audiodevice::IAudioDevice& device) {
    for (const contract::audiodevice::SoundId sound : assets.takeUnloaded()) {
        device.destroySound(sound);
    }
    const std::vector<core::asset::AssetId> queued = assets.getQueued();
    for (const core::asset::AssetId id : queued) {
        const AudioAssets::Entry& entry = *assets.get(id);
        try {
            contract::audiodevice::SoundId sound;
            if (!entry.encoded.empty()) {
                sound = device.createSound(entry.encoded, entry.desc);
            } else if (const std::optional<std::filesystem::path> directory = assets.getSource().directory()) {
                // A file on disk: the backend opens it, and a streamed sound keeps reading it.
                sound = device.createSound(core::asset::resolveAssetPath(*directory, entry.path), entry.desc);
            } else {
                // A pack: the backend gets the bytes (EXP-05 streams them).
                const std::vector<std::byte> bytes = core::vfs::readBytes(assets.getSource(), entry.path);
                sound = device.createSound(bytes, entry.desc);
            }
            assets.setLoaded(id, sound);
        } catch (const std::exception& error) {
            std::cerr << "[AudioAssets] " << error.what() << '\n';
            assets.setFailed(id, error.what());
        }
    }
}
}  // namespace aylo::plugins::audio
