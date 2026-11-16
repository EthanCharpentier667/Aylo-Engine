/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** SoundFiles
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace aylo::test::plugins {
/// @brief Appends @p tag's letters to @p bytes.
inline void putTag(std::vector<std::byte>& bytes, std::string_view tag) {
    for (const char letter : tag) {
        bytes.push_back(static_cast<std::byte>(letter));
    }
}

/// @brief Appends the @p count low bytes of @p value, little-endian.
inline void putLe(std::vector<std::byte>& bytes, std::uint64_t value, int count) {
    for (int index = 0; index < count; ++index) {
        bytes.push_back(static_cast<std::byte>((value >> (8 * index)) & 0xFFU));
    }
}

/// @brief Appends the @p count low bytes of @p value, big-endian.
inline void putBe(std::vector<std::byte>& bytes, std::uint64_t value, int count) {
    for (int index = count - 1; index >= 0; --index) {
        bytes.push_back(static_cast<std::byte>((value >> (8 * index)) & 0xFFU));
    }
}

/// @brief What a WAV file's `fmt ` chunk says, and the size of its data.
struct WavShape {
    std::uint32_t format{1};  ///< 1: PCM, 2: MS ADPCM, 3: float, 0x11: IMA ADPCM.
    std::uint32_t channels{1};
    std::uint32_t sampleRate{44100};
    std::uint32_t bitsPerSample{16};
    std::uint32_t blockAlign{2};
    std::uint32_t dataSize{0};  ///< Bytes of the data chunk, all silent.
};

/// @return A WAV file of that shape, with a LIST chunk before its data, as encoders write.
[[nodiscard]] inline std::vector<std::byte> wavFile(const WavShape& shape) {
    std::vector<std::byte> chunks;
    putTag(chunks, "fmt ");
    putLe(chunks, 16, 4);
    putLe(chunks, shape.format, 2);
    putLe(chunks, shape.channels, 2);
    putLe(chunks, shape.sampleRate, 4);
    putLe(chunks, static_cast<std::uint64_t>(shape.sampleRate) * shape.blockAlign, 4);
    putLe(chunks, shape.blockAlign, 2);
    putLe(chunks, shape.bitsPerSample, 2);
    putTag(chunks, "LIST");
    putLe(chunks, 3, 4);  // an odd size: padded with one byte
    putTag(chunks, "abc");
    chunks.push_back(std::byte{0});
    putTag(chunks, "data");
    putLe(chunks, shape.dataSize, 4);
    chunks.resize(chunks.size() + shape.dataSize, std::byte{0});

    std::vector<std::byte> bytes;
    putTag(bytes, "RIFF");
    putLe(bytes, 4 + chunks.size(), 4);
    putTag(bytes, "WAVE");
    bytes.insert(bytes.end(), chunks.begin(), chunks.end());
    return bytes;
}

/// @return A 16-bit PCM WAV of @p frames silent frames.
[[nodiscard]] inline std::vector<std::byte> pcmWav(std::uint32_t frames, std::uint32_t channels = 1,
                                                   std::uint32_t sampleRate = 44100) {
    return wavFile(WavShape{
        .channels = channels, .sampleRate = sampleRate, .blockAlign = 2 * channels, .dataSize = frames * 2 * channels});
}

/// @return The "fLaC" marker and a STREAMINFO block (16-bit samples), the start of a FLAC file.
[[nodiscard]] inline std::vector<std::byte> flacStreamInfo(std::uint32_t sampleRate, std::uint32_t channels,
                                                           std::uint64_t frames) {
    std::vector<std::byte> bytes;
    putTag(bytes, "fLaC");
    putBe(bytes, 0x80, 1);  // the last metadata block, of type STREAMINFO
    putBe(bytes, 34, 3);
    putBe(bytes, 4096, 2);  // block sizes
    putBe(bytes, 4096, 2);
    putBe(bytes, 0, 3);  // frame sizes
    putBe(bytes, 0, 3);
    // 20 bits of sample rate, 3 of channels - 1, 5 of bits per sample - 1, 36 of frames.
    const std::uint64_t packed = (static_cast<std::uint64_t>(sampleRate) << 44U) |
                                 (static_cast<std::uint64_t>(channels - 1) << 41U) | (15ULL << 36U) | frames;
    putBe(bytes, packed, 8);
    bytes.resize(bytes.size() + 16, std::byte{0});  // the MD5 of the samples
    return bytes;
}

/// @brief Appends an Ogg page holding @p body (one packet), at @p granule.
inline void putOggPage(std::vector<std::byte>& bytes, std::uint64_t granule, const std::vector<std::byte>& body,
                       std::uint32_t serial = 0x1234) {
    putTag(bytes, "OggS");
    putLe(bytes, 0, 1);  // version
    putLe(bytes, 0, 1);  // header type
    putLe(bytes, granule, 8);
    putLe(bytes, serial, 4);
    putLe(bytes, 0, 4);  // sequence
    putLe(bytes, 0, 4);  // CRC, not checked by the probe
    const std::size_t full = body.size() / 255;
    putLe(bytes, full + 1, 1);
    for (std::size_t segment = 0; segment < full; ++segment) {
        putLe(bytes, 255, 1);
    }
    putLe(bytes, body.size() % 255, 1);
    bytes.insert(bytes.end(), body.begin(), body.end());
}

/// @return An Ogg Vorbis file: its identification header, then pages of silence whose last ends at @p frames.
[[nodiscard]] inline std::vector<std::byte> oggVorbis(std::uint32_t sampleRate, std::uint32_t channels,
                                                      std::uint64_t frames) {
    std::vector<std::byte> identification;
    putLe(identification, 1, 1);
    putTag(identification, "vorbis");
    putLe(identification, 0, 4);  // version
    putLe(identification, channels, 1);
    putLe(identification, sampleRate, 4);
    identification.resize(30, std::byte{0});
    std::vector<std::byte> bytes;
    putOggPage(bytes, 0, identification);
    putOggPage(bytes, 0, std::vector<std::byte>(300, std::byte{0}));  // comments and setup headers
    putOggPage(bytes, frames / 2, std::vector<std::byte>(600, std::byte{0}));
    putOggPage(bytes, ~std::uint64_t{0}, std::vector<std::byte>(40, std::byte{0}));  // a packet ending on no page
    putOggPage(bytes, frames, std::vector<std::byte>(100, std::byte{0}));
    return bytes;
}

/// @return An MP3 of @p frames MPEG-1 layer III frames (128 kbit/s, 44.1 kHz, stereo, 1152 samples each), after an
/// ID3v2 tag; with @p delay and @p padding, preceded by an Info frame whose LAME tag gives them.
[[nodiscard]] inline std::vector<std::byte> mp3File(std::uint32_t frames, std::uint32_t delay = 0,
                                                    std::uint32_t padding = 0, bool info = false) {
    constexpr std::uint32_t kHeader = 0xFFFB9000;  // MPEG-1 layer III, no CRC, 128 kbit/s, 44.1 kHz, stereo
    constexpr std::size_t kFrameSize = 417;        // 144 * 128000 / 44100
    std::vector<std::byte> bytes;
    putTag(bytes, "ID3");
    putBe(bytes, 0x0300, 2);  // version
    putBe(bytes, 0, 1);       // flags
    putBe(bytes, 20, 4);      // syncsafe size
    bytes.resize(bytes.size() + 20, std::byte{0});
    if (info) {
        const std::size_t start = bytes.size();
        putBe(bytes, kHeader, 4);
        bytes.resize(bytes.size() + 32, std::byte{0});  // side information
        putTag(bytes, "Info");
        putBe(bytes, 0x01, 4);  // flags: the frame count follows
        putBe(bytes, frames, 4);
        putTag(bytes, "LAME3.100");
        bytes.resize(bytes.size() + 12, std::byte{0});
        const std::uint32_t delayField = delay - 529;  // the LAME tag counts the decoder's delay apart
        const std::uint32_t paddingField = padding + 529;
        putBe(bytes, (static_cast<std::uint64_t>(delayField) << 12U) | paddingField, 3);
        bytes.resize(start + kFrameSize, std::byte{0});
    }
    for (std::uint32_t frame = 0; frame < frames; ++frame) {
        const std::size_t start = bytes.size();
        putBe(bytes, kHeader, 4);
        bytes.resize(start + kFrameSize, std::byte{0});
    }
    putTag(bytes, "TAG");  // an ID3v1 tag ends the frames
    bytes.resize(bytes.size() + 125, std::byte{0});
    return bytes;
}
}  // namespace aylo::test::plugins
