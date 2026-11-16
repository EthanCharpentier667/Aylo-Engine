/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** SoundProbe
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>

#include "plugins/audio/Export.hpp"

namespace aylo::plugins::audio {
/// @brief What a sound file holds, as written in it: before the device resamples or remixes it.
struct SoundInfo {
    std::uint32_t sampleRate{0};  ///< Frames per second.
    std::uint32_t channels{0};    ///< Samples per frame.
    std::uint64_t frames{0};      ///< Its length, in frames.

    [[nodiscard]] bool operator==(const SoundInfo& other) const = default;
};

/// @brief The outcome of reading a sound file's header: its SoundInfo, or why it has none.
struct SoundProbe {
    std::optional<SoundInfo> info;  ///< Set when the file is a sound the engine decodes.
    std::string error;              ///< Why it is not, when `info` is not set.
};

/// @brief Reads the header of a sound file (WAV, FLAC, OGG Vorbis, MP3), without decoding it, so it works with every
/// audio backend, the silent one included.
/// @details The length is the one miniaudio's decoders play, counted as they count it: the WAV data chunk (by blocks
/// for ADPCM; its `fact` chunk is ignored, as dr_wav ignores it), the FLAC STREAMINFO, the granule position of the
/// last Ogg page (Vorbis or FLAC), the MP3 frames less the encoder's delay and padding its LAME tag gives.
/// @param encoded The file's bytes.
/// @return The info, or an error naming what is wrong with the data (not its path: the caller knows it).
[[nodiscard]] AYLO_PLUGIN_AUDIO_API SoundProbe probeSound(std::span<const std::byte> encoded);
}  // namespace aylo::plugins::audio
