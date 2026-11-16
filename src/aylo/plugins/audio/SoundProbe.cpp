/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** SoundProbe
*/

#include "plugins/audio/SoundProbe.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace aylo::plugins::audio {
namespace {
using Bytes = std::span<const std::byte>;

/// @return The byte at @p at, or 0 past the end (the callers check the size first; this keeps a mistake harmless).
[[nodiscard]] std::uint32_t byteAt(Bytes bytes, std::size_t at) {
    return at < bytes.size() ? std::to_integer<std::uint32_t>(bytes.subspan(at).front()) : 0;
}

[[nodiscard]] std::uint32_t readLe16(Bytes bytes, std::size_t at) {
    return byteAt(bytes, at) | (byteAt(bytes, at + 1) << 8U);
}

[[nodiscard]] std::uint32_t readLe32(Bytes bytes, std::size_t at) {
    return readLe16(bytes, at) | (readLe16(bytes, at + 2) << 16U);
}

[[nodiscard]] std::uint64_t readLe64(Bytes bytes, std::size_t at) {
    return static_cast<std::uint64_t>(readLe32(bytes, at)) |
           (static_cast<std::uint64_t>(readLe32(bytes, at + 4)) << 32U);
}

[[nodiscard]] std::uint32_t readBe32(Bytes bytes, std::size_t at) {
    return (byteAt(bytes, at) << 24U) | (byteAt(bytes, at + 1) << 16U) | (byteAt(bytes, at + 2) << 8U) |
           byteAt(bytes, at + 3);
}

/// @return Whether @p bytes holds @p tag at @p at.
[[nodiscard]] bool hasTag(Bytes bytes, std::size_t at, std::string_view tag) {
    if (at > bytes.size() || bytes.size() - at < tag.size()) {
        return false;
    }
    return std::ranges::equal(bytes.subspan(at, tag.size()), tag,
                              [](std::byte byte, char letter) { return std::to_integer<char>(byte) == letter; });
}

[[nodiscard]] SoundProbe fail(std::string error) { return SoundProbe{.error = std::move(error)}; }

[[nodiscard]] SoundProbe found(std::uint32_t sampleRate, std::uint32_t channels, std::uint64_t frames) {
    return SoundProbe{.info = SoundInfo{.sampleRate = sampleRate, .channels = channels, .frames = frames}};
}

/// @return Where the data starts after an ID3v2 tag (MP3, sometimes FLAC), 0 without one.
[[nodiscard]] std::size_t skipId3(Bytes bytes) {
    constexpr std::size_t kHeader = 10;
    if (!hasTag(bytes, 0, "ID3") || bytes.size() < kHeader) {
        return 0;
    }
    std::size_t size = 0;
    for (std::size_t index = 6; index < kHeader; ++index) {
        size = (size << 7U) | (byteAt(bytes, index) & 0x7FU);  // syncsafe: 7 bits per byte
    }
    const bool footer = (byteAt(bytes, 5) & 0x10U) != 0;
    return std::min(bytes.size(), kHeader + size + (footer ? kHeader : 0));
}

/// @name The WAV format tags dr_wav (miniaudio's decoder) reads.
/// @{
constexpr std::uint32_t kPcm = 0x0001;
constexpr std::uint32_t kMsAdpcm = 0x0002;
constexpr std::uint32_t kFloat = 0x0003;
constexpr std::uint32_t kALaw = 0x0006;
constexpr std::uint32_t kMuLaw = 0x0007;
constexpr std::uint32_t kImaAdpcm = 0x0011;
constexpr std::uint32_t kExtensible = 0xFFFE;
/// @}

/// @brief The fields of a WAV `fmt ` chunk the length needs.
struct WavFormatChunk {
    std::uint32_t format{0};
    std::uint32_t channels{0};
    std::uint32_t sampleRate{0};
    std::uint32_t blockAlign{0};
    std::uint32_t bitsPerSample{0};
};

/// @return The frames of an ADPCM WAV's @p dataSize bytes, counted as dr_wav does: by blocks, each with a header of
/// @p headerBytes per channel; an IMA block also holds one frame in its header.
[[nodiscard]] std::uint64_t adpcmFrames(const WavFormatChunk& format, std::uint64_t dataSize, std::uint64_t headerBytes,
                                        bool ima) {
    const std::uint64_t blocks = (dataSize + format.blockAlign - 1) / format.blockAlign;
    const std::uint64_t headers = blocks * headerBytes * format.channels;
    const std::uint64_t frames = dataSize > headers ? (dataSize - headers) * 2 / format.channels : 0;
    return ima ? frames + blocks : frames;
}

[[nodiscard]] SoundProbe probeWav(Bytes bytes) {
    constexpr std::size_t kRiffHeader = 12;
    constexpr std::size_t kChunkHeader = 8;
    constexpr std::uint32_t kFmtSize = 16;
    constexpr std::uint32_t kExtensibleSize = 26;
    if (bytes.size() < kRiffHeader || !hasTag(bytes, 8, "WAVE")) {
        return fail("a RIFF file that is not a WAV");
    }
    std::optional<WavFormatChunk> format;
    std::optional<std::uint64_t> dataSize;
    std::uint64_t position = kRiffHeader;
    while (position + kChunkHeader <= bytes.size()) {
        const auto chunk = static_cast<std::size_t>(position);
        const std::uint32_t size = readLe32(bytes, chunk + 4);
        const std::size_t body = chunk + kChunkHeader;
        const std::size_t available = bytes.size() - body;
        if (hasTag(bytes, chunk, "fmt ") && size >= kFmtSize && available >= kFmtSize) {
            format = WavFormatChunk{.format = readLe16(bytes, body),
                                    .channels = readLe16(bytes, body + 2),
                                    .sampleRate = readLe32(bytes, body + 4),
                                    .blockAlign = readLe16(bytes, body + 12),
                                    .bitsPerSample = readLe16(bytes, body + 14)};
            if (format->format == kExtensible && size >= kExtensibleSize && available >= kExtensibleSize) {
                format->format = readLe16(bytes, body + 24);  // the sub-format GUID starts with the format's tag
            }
        } else if (hasTag(bytes, chunk, "data")) {
            dataSize = std::min<std::uint64_t>(size, available);  // a file cut short plays what it has
        }
        position = body + static_cast<std::uint64_t>(size) + (size & 1U);  // chunks are padded to an even size
    }
    if (!format) {
        return fail("a WAV without a fmt chunk");
    }
    if (!dataSize) {
        return fail("a WAV without a data chunk");
    }
    if (format->channels == 0 || format->sampleRate == 0) {
        return fail("a WAV with no channel or no sample rate");
    }
    // The lengths dr_wav, miniaudio's decoder, gives.
    constexpr std::uint64_t kMsAdpcmHeader = 6;
    constexpr std::uint64_t kImaAdpcmHeader = 4;
    switch (format->format) {
        case kPcm:
        case kFloat:
        case kALaw:
        case kMuLaw: {
            const std::uint32_t frameBytes =
                format->bitsPerSample % 8 == 0 ? format->bitsPerSample * format->channels / 8 : format->blockAlign;
            if (frameBytes == 0) {
                return fail("a WAV with frames of 0 bytes");
            }
            return found(format->sampleRate, format->channels, *dataSize / frameBytes);
        }
        case kMsAdpcm:
        case kImaAdpcm:
            if (format->channels > 2 || format->blockAlign == 0) {
                return fail("an ADPCM WAV with more than 2 channels or a block size of 0");
            }
            return found(format->sampleRate, format->channels,
                         adpcmFrames(*format, *dataSize, format->format == kMsAdpcm ? kMsAdpcmHeader : kImaAdpcmHeader,
                                     format->format == kImaAdpcm));
        default:
            return fail(std::format("a WAV in a format the engine does not decode (0x{:04x})", format->format));
    }
}

/// @return The STREAMINFO of the FLAC stream @p bytes holds ("fLaC" and its first block), its frames 0 when it does
/// not give its length; nothing if it has none.
[[nodiscard]] std::optional<SoundInfo> readStreamInfo(Bytes bytes) {
    constexpr std::size_t kStreamInfo = 8;  // after "fLaC" and the block's header
    constexpr std::size_t kStreamInfoSize = 34;
    if (!hasTag(bytes, 0, "fLaC") || bytes.size() < kStreamInfo + kStreamInfoSize || (byteAt(bytes, 4) & 0x7FU) != 0) {
        return std::nullopt;
    }
    // After the block and frame sizes (10 bytes): 20 bits of sample rate, 3 of channels - 1, 5 of bits per sample - 1,
    // 36 of total samples per channel.
    const std::size_t info = kStreamInfo + 10;
    return SoundInfo{
        .sampleRate = (byteAt(bytes, info) << 12U) | (byteAt(bytes, info + 1) << 4U) | (byteAt(bytes, info + 2) >> 4U),
        .channels = ((byteAt(bytes, info + 2) >> 1U) & 0x07U) + 1,
        .frames = (static_cast<std::uint64_t>(byteAt(bytes, info + 3) & 0x0FU) << 32U) | readBe32(bytes, info + 4),
    };
}

[[nodiscard]] SoundProbe probeFlac(Bytes bytes) {
    const std::optional<SoundInfo> info = readStreamInfo(bytes);
    if (!info) {
        return fail("a FLAC without its STREAMINFO block");
    }
    if (info->sampleRate == 0) {
        return fail("a FLAC with no sample rate");
    }
    if (info->frames == 0) {
        return fail("a FLAC that does not give its length");
    }
    return SoundProbe{.info = info};
}

/// @brief One Ogg page.
struct OggPage {
    std::size_t body{0};       ///< Where its data starts.
    std::size_t size{0};       ///< Bytes of data.
    std::uint64_t granule{0};  ///< Its granule position: frames decoded by its end; all ones when none ends on it.
    std::uint32_t serial{0};   ///< The stream it belongs to.
};

/// @return The Ogg page starting at @p at, or nothing if there is none or it is cut short.
[[nodiscard]] std::optional<OggPage> oggPageAt(Bytes bytes, std::size_t at) {
    constexpr std::size_t kPageHeader = 27;
    if (at > bytes.size() || bytes.size() - at < kPageHeader || !hasTag(bytes, at, "OggS")) {
        return std::nullopt;
    }
    const std::size_t segments = byteAt(bytes, at + kPageHeader - 1);
    OggPage page{
        .body = at + kPageHeader + segments, .granule = readLe64(bytes, at + 6), .serial = readLe32(bytes, at + 14)};
    if (page.body > bytes.size()) {
        return std::nullopt;
    }
    for (std::size_t segment = 0; segment < segments; ++segment) {
        page.size += byteAt(bytes, at + kPageHeader + segment);
    }
    if (page.size > bytes.size() - page.body) {
        return std::nullopt;  // cut short: the decoder stops before it too
    }
    return page;
}

/// @return The sample rate and channels the first packet of an Ogg stream gives (Vorbis or FLAC), its frames 0.
[[nodiscard]] SoundProbe identifyOgg(Bytes packet) {
    constexpr std::size_t kOggFlacHeader = 9;    // 0x7F, "FLAC", the mapping's version and its header count
    constexpr std::size_t kIdentification = 16;  // packet type, "vorbis", version, channels, sample rate
    if (hasTag(packet, 0, "OpusHead")) {
        return fail("an Ogg Opus file, which the engine does not decode (use Ogg Vorbis)");
    }
    if (packet.size() >= kOggFlacHeader && byteAt(packet, 0) == 0x7F && hasTag(packet, 1, "FLAC")) {
        // Ogg FLAC: the first packet holds the STREAMINFO block; the length comes from the granules.
        const std::optional<SoundInfo> info = readStreamInfo(packet.subspan(kOggFlacHeader));
        if (!info) {
            return fail("an Ogg FLAC without its STREAMINFO block");
        }
        return found(info->sampleRate, info->channels, 0);
    }
    if (packet.size() >= kIdentification && byteAt(packet, 0) == 1 && hasTag(packet, 1, "vorbis")) {
        return found(readLe32(packet, 12), byteAt(packet, 11), 0);
    }
    return fail("an Ogg file that is neither Vorbis nor FLAC");
}

[[nodiscard]] SoundProbe probeOgg(Bytes bytes) {
    constexpr std::uint64_t kNoGranule = ~std::uint64_t{0};
    const std::optional<OggPage> first = oggPageAt(bytes, 0);
    if (!first) {
        return fail("a truncated Ogg file");
    }
    SoundProbe probe = identifyOgg(bytes.subspan(first->body, first->size));
    if (!probe.info) {
        return probe;
    }
    if (probe.info->channels == 0 || probe.info->sampleRate == 0) {
        return fail("an Ogg stream with no channel or no sample rate");
    }
    std::optional<std::uint64_t> last;
    std::size_t at = first->body + first->size;
    while (const std::optional<OggPage> page = oggPageAt(bytes, at)) {
        if (page->serial == first->serial && page->granule != kNoGranule) {
            last = page->granule;  // a granule position counts the frames decoded up to the end of the page
        }
        at = page->body + page->size;
    }
    if (!last) {
        return fail("an Ogg stream with no audio");
    }
    probe.info->frames = *last;
    return probe;
}

/// @brief One MPEG audio frame's header.
struct MpegFrame {
    std::uint32_t sampleRate{0};
    std::uint32_t channels{0};
    std::uint32_t samples{0};  ///< Frames of sound it decodes to.
    std::size_t length{0};     ///< Its size in bytes, header included.
    std::size_t sideInfo{0};   ///< Bytes after the header before a Xing tag can start (layer III), 0 for the others.
};

/// @return The row of the bitrate table for an MPEG-1 (@p mpeg1) or MPEG-2/2.5 frame of layer @p layer (1 to 3).
[[nodiscard]] std::size_t bitrateTable(bool mpeg1, std::uint32_t layer) {
    if (mpeg1) {
        return layer - 1;
    }
    return layer == 1 ? 3 : 4;
}

/// @return The side information's size of a layer III frame, in bytes, after its header and its CRC if it has one.
[[nodiscard]] std::size_t sideInfoSize(bool mpeg1, bool mono, bool crc) {
    std::size_t size = 0;
    if (mpeg1) {
        size = mono ? 17 : 32;
    } else {
        size = mono ? 9 : 17;
    }
    return size + (crc ? 2 : 0);
}

/// @return The MPEG audio frame starting at @p at, or nothing if there is none.
[[nodiscard]] std::optional<MpegFrame> mpegFrameAt(Bytes bytes, std::size_t at) {
    constexpr std::array<std::array<std::uint32_t, 15>, 5> kBitrates{{
        {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448},  // MPEG-1 layer I
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},     // MPEG-1 layer II
        {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320},      // MPEG-1 layer III
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},     // MPEG-2 and 2.5 layer I
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},          // MPEG-2 and 2.5 layers II, III
    }};
    constexpr std::array<std::uint32_t, 3> kSampleRates{44100, 48000,
                                                        32000};  // MPEG-1; halved for 2, quartered for 2.5
    if (at > bytes.size() || bytes.size() - at < 4) {
        return std::nullopt;
    }
    const std::uint32_t header = readBe32(bytes, at);
    const std::uint32_t version = (header >> 19U) & 3U;    // 0: MPEG-2.5, 1: reserved, 2: MPEG-2, 3: MPEG-1
    const std::uint32_t layerBits = (header >> 17U) & 3U;  // 1: layer III, 2: II, 3: I, 0: reserved
    const std::uint32_t bitrateIndex = (header >> 12U) & 0x0FU;
    const std::uint32_t rateIndex = (header >> 10U) & 3U;
    if ((header >> 21U) != 0x7FFU || version == 1 || layerBits == 0 || bitrateIndex == 0 || bitrateIndex == 15 ||
        rateIndex == 3) {
        return std::nullopt;  // no sync word, a reserved value, or a free bitrate (no frame size in the header)
    }
    const bool mpeg1 = version == 3;
    const std::uint32_t layer = 4 - layerBits;
    const std::size_t bitrate =
        static_cast<std::size_t>(kBitrates.at(bitrateTable(mpeg1, layer)).at(bitrateIndex)) * 1000;
    constexpr std::array<std::uint32_t, 4> kRateShifts{2, 0, 1, 0};  // by version: MPEG-2.5, reserved, MPEG-2, MPEG-1
    const std::uint32_t sampleRate = kSampleRates.at(rateIndex) >> kRateShifts.at(version);
    const std::uint32_t padding = (header >> 9U) & 1U;
    const bool mono = ((header >> 6U) & 3U) == 3;
    MpegFrame frame{.sampleRate = sampleRate, .channels = mono ? 1U : 2U};
    if (layer == 1) {
        frame.samples = 384;
        frame.length = ((12 * bitrate / sampleRate) + padding) * 4;
    } else {
        frame.samples = (layer == 3 && !mpeg1) ? 576 : 1152;
        frame.length = (frame.samples / 8 * bitrate / sampleRate) + padding;
        if (layer == 3) {
            frame.sideInfo = sideInfoSize(mpeg1, mono, ((header >> 16U) & 1U) == 0);  // bit 16 clear: a CRC follows
        }
    }
    return frame;
}

/// @brief What the Xing or Info tag of an MP3's first frame says, as dr_mp3 (miniaudio's decoder) reads it.
struct XingTag {
    std::uint64_t delay{0};    ///< Frames the encoder added at the start, which the decoder skips.
    std::uint64_t padding{0};  ///< Frames it added at the end, which the decoder drops.
};

/// @return The Xing or Info tag of the MP3 frame @p first starting @p bytes, or nothing if it has none: then that frame
/// is sound like the others.
[[nodiscard]] std::optional<XingTag> readXingTag(Bytes bytes, const MpegFrame& first) {
    constexpr std::size_t kHeader = 4;
    constexpr std::size_t kTocSize = 100;
    constexpr std::size_t kLameDelayOffset = 21;  // from the start of the LAME tag to its delay and padding
    constexpr std::size_t kLameEnd = 14;
    constexpr std::uint64_t kDecoderDelay = 528 + 1;  // what the LAME tag counts on top of the encoder's own
    std::size_t at = kHeader + first.sideInfo;
    if (first.sideInfo == 0 || first.length > bytes.size() ||
        !(hasTag(bytes, at, "Xing") || hasTag(bytes, at, "Info"))) {
        return std::nullopt;
    }
    XingTag tag;
    const std::uint32_t flags = at + 8 <= first.length ? byteAt(bytes, at + 7) : 0;
    at += 8;
    at += (flags & 0x01U) != 0 ? 4 : 0;         // frame count
    at += (flags & 0x02U) != 0 ? 4 : 0;         // byte count
    at += (flags & 0x04U) != 0 ? kTocSize : 0;  // seek table
    at += (flags & 0x08U) != 0 ? 4 : 0;         // quality
    if (at < first.length && byteAt(bytes, at) != 0 && at + kLameDelayOffset + kLameEnd < first.length) {
        at += kLameDelayOffset;
        const std::uint64_t delay = (byteAt(bytes, at) << 4U) | (byteAt(bytes, at + 1) >> 4U);
        const std::uint64_t padding = ((byteAt(bytes, at + 1) & 0x0FU) << 8U) | byteAt(bytes, at + 2);
        tag.delay = delay + kDecoderDelay;
        tag.padding = padding > kDecoderDelay ? padding - kDecoderDelay : 0;
    }
    return tag;
}

[[nodiscard]] SoundProbe probeMp3(Bytes bytes, const MpegFrame& first) {
    // A Xing or Info frame describes the file and decodes to no sound.
    const std::optional<XingTag> tag = readXingTag(bytes, first);
    std::uint64_t frames = 0;
    std::size_t position = 0;
    bool firstFrame = true;
    while (const std::optional<MpegFrame> frame = mpegFrameAt(bytes, position)) {
        if (frame->length > bytes.size() - position) {
            break;  // cut short
        }
        if (!(firstFrame && tag)) {
            frames += frame->samples;
        }
        firstFrame = false;
        position += frame->length;
    }
    if (tag) {
        // The encoder's delay and padding are not played (gapless playback).
        frames -= std::min(frames, tag->delay);
        frames -= std::min(frames, tag->padding);
    }
    if (frames == 0) {
        return fail("an MP3 with no audio frame");
    }
    return found(first.sampleRate, first.channels, frames);
}
}  // namespace

SoundProbe probeSound(std::span<const std::byte> encoded) {
    if (hasTag(encoded, 0, "RIFF")) {
        return probeWav(encoded);
    }
    if (hasTag(encoded, 0, "OggS")) {
        return probeOgg(encoded);
    }
    const Bytes tagged = encoded.subspan(skipId3(encoded));
    if (hasTag(tagged, 0, "fLaC")) {
        return probeFlac(tagged);
    }
    if (const std::optional<MpegFrame> first = mpegFrameAt(tagged, 0)) {
        return probeMp3(tagged, *first);
    }
    return fail("not a sound file the engine decodes (WAV, OGG Vorbis, MP3, FLAC)");
}
}  // namespace aylo::plugins::audio
