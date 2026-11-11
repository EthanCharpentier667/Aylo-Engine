/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginSignature
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string_view>

#include "core/layout/Layout.hpp"
#include "rtype/engine/Export.hpp"

namespace rtype::engine::plugin {
inline constexpr std::size_t kPublicKeySize = 32;  ///< Bytes of an Ed25519 public key.
inline constexpr std::size_t kSecretKeySize = 64;  ///< Bytes of an Ed25519 secret key.
inline constexpr std::size_t kSignatureSize = 64;  ///< Bytes of an Ed25519 signature.

/// @brief The extension of a library's signature, added after its own: `boss.so.sig`.
inline constexpr std::string_view kSignatureExtension = core::layout::kSignatureExtension;

/// @brief An Ed25519 public key.
using PublicKey = std::array<std::uint8_t, kPublicKeySize>;

/// @brief What a signature file holds: who signed the library, the proof the root key trusts them, and the signature.
///
/// @details A developer holds their own key pair. The root key, kept by the project, signs the developer's public key:
/// that certificate is what lets the engine trust a key it has never seen, with only the root's public key built in.
struct RTYPE_ENGINE_API PluginSignature {
    PublicKey developer{};                                   ///< The key that signed the library.
    std::array<std::uint8_t, kSignatureSize> certificate{};  ///< The root's signature of `developer`.
    std::array<std::uint8_t, kSignatureSize> signature{};    ///< The developer's signature of the library's bytes.
};

/// @return The public key written in hexadecimal in @p text, or nothing if it is not one.
[[nodiscard]] RTYPE_ENGINE_API std::optional<PublicKey> parsePublicKey(std::string_view text);

/// @brief Refuses a library that the root key does not vouch for.
/// @param library The library's file; its signature is next to it, `<file>.sig`.
/// @param root The public key every certificate must be signed with.
/// @throws exceptions::RuntimeError If the signature is missing or malformed, the developer's key has no valid
/// certificate, or the library is not what was signed, naming the file.
RTYPE_ENGINE_API void verifyPluginSignature(const std::filesystem::path& library, const PublicKey& root);

/// @brief What a game's plugin library must pass before it is opened, for PluginLoader::load(): in a release build its
/// signature, checked against the root key built into the engine; nothing in a debug build, where plugins are written.
/// @return The check; empty in a debug build. It throws exceptions::RuntimeError for a library it refuses, or for every
/// library when the build has no root key (`xmake f --plugin_root_key=<hex>`).
[[nodiscard]] RTYPE_ENGINE_API std::function<void(const std::filesystem::path&)> gamePluginVerifier();
}  // namespace rtype::engine::plugin
