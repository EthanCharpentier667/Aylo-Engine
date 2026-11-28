/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ImportFile
*/

#include "ImportFile.hpp"

#include <format>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string>

#include "aylo/contract/audiodevice/AudioTypes.hpp"
#include "aylo/contract/audiodevice/exceptions/AudioExceptions.hpp"
#include "core/vfs/FileSource.hpp"

namespace aylo::plugins::audio::detail {
namespace {
using contract::audiodevice::exceptions::AudioLoadException;
}  // namespace

contract::audiodevice::SoundDesc applyImportFile(const core::vfs::FileSource& source, const std::string& path,
                                                 contract::audiodevice::SoundDesc desc) {
    const std::string file = path + std::string{kImportFileExtension};
    if (!source.exists(file)) {
        return desc;
    }
    nlohmann::json options;
    try {
        options = nlohmann::json::parse(source.read(file));
    } catch (const nlohmann::json::exception& error) {
        throw AudioLoadException{std::format("Import file '{}': {}", file, error.what())};
    }
    if (!options.is_object()) {
        throw AudioLoadException{
            std::format("Import file '{}': expected an object, such as {{ \"stream\": true }}", file)};
    }
    for (const auto& [key, value] : options.items()) {
        if (key != "stream") {
            throw AudioLoadException{
                std::format("Import file '{}': unknown key '{}'; a sound's import file takes 'stream'", file, key)};
        }
        if (!value.is_boolean()) {
            throw AudioLoadException{std::format("Import file '{}': 'stream' takes true or false", file)};
        }
        desc.stream = desc.stream || value.get<bool>();
    }
    return desc;
}
}  // namespace aylo::plugins::audio::detail
