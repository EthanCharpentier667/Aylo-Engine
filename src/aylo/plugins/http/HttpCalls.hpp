/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** HttpCalls
*/

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/call/CallTraits.hpp"
#include "aylo/ecs/type/String.hpp"

namespace aylo::plugins::http {

inline constexpr std::string_view kRequestCall = "aylo.http.request";  ///< HttpClient::send().
inline constexpr std::string_view kCancelCall = "aylo.http.cancel";    ///< HttpClient::cancel().

/// @brief The arguments of `aylo.http.request`: one request, answered later by an HttpResponse.
struct RequestArguments {
    ecs::String method{"GET"};  ///< GET, POST, PUT, PATCH, DELETE or HEAD, any case.
    std::string url;            ///< `https://host/path`, to a host of http.allowedHosts.
    std::string headers;        ///< One `Name: value` per line; `{{env:NAME}}` is a secret of http.secrets.
    std::string body;           ///< Sent as is (a JSON text, for instance); empty for none.
    float timeout{0.0F};        ///< Seconds from the call, queue included; 0: http.timeout, also the most it can be.
    float priority{0.0F};       ///< A whole number; higher is sent first among the requests waiting for a slot.
};
AYLO_ECS_CALL_DATA(RequestArguments, "aylo.http.RequestArguments", method, url, headers, body, timeout, priority);

/// @brief The result of `aylo.http.request`.
struct RequestResult {
    std::uint32_t request{0};  ///< Its id, which its HttpResponse carries.
};
AYLO_ECS_CALL_DATA(RequestResult, "aylo.http.RequestResult", request);

/// @brief The arguments of `aylo.http.cancel`.
struct CancelArguments {
    std::uint32_t request{0};  ///< An id `aylo.http.request` gave.
};
AYLO_ECS_CALL_DATA(CancelArguments, "aylo.http.CancelArguments", request);

/// @brief The result of `aylo.http.cancel`.
struct CancelResult {
    bool cancelled{false};  ///< False when the request had already ended, or no request has this id.
};
AYLO_ECS_CALL_DATA(CancelResult, "aylo.http.CancelResult", cancelled);

/// @brief Registers `aylo.http.request` and `aylo.http.cancel` on @p app. A request that cannot be sent (an invalid
/// URL or header, a host that is not allowed, an unknown secret) throws an HttpException naming why, which a script
/// gets as a Luau error. Also registers the forms (addHttpForms()). Called by the HttpPlugin.
void addHttpCalls(ecs::App& app);

/// @brief Registers the Luau helper `engine.http.request(ctx, { url, method, headers, body, timeout, priority })` on @p
/// registry: it checks its table, writes the headers table as lines, and makes the call. Its library does it on every
/// side, and so does building the plugin, for a program without libraries.
void addHttpForms(ecs::PluginRegistry& registry);

}  // namespace aylo::plugins::http
