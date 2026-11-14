/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TimeCalls
*/

#include "aylo/replication/timesync/TimeCalls.hpp"

#include <optional>
#include <string>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/time/Time.hpp"
#include "aylo/replication/client/ClientState.hpp"
#include "aylo/replication/server/ServerState.hpp"
#include "aylo/replication/standalone/LocalSession.hpp"

namespace aylo::replication::timesync {
namespace {
ServerTime clientServerTime(ecs::ResMut<client::ClientState> state) {
    const std::optional<double> time = state->serverTime();
    return ServerTime{.synced = time.has_value(), .time = time.value_or(0.0)};
}

Rtt serverRtt(ecs::Res<server::ServerState> state, const RttArguments& arguments) {
    const std::optional<double> rtt = state->rtt(arguments.client);
    return Rtt{.connected = rtt.has_value(), .rtt = rtt.value_or(0.0)};
}

ServerTime standaloneServerTime(ecs::Res<ecs::Time> time) { return ServerTime{.synced = true, .time = time->elapsed}; }

Rtt standaloneRtt(const RttArguments& arguments) {
    return Rtt{.connected = arguments.client == standalone::LocalSession::kClient, .rtt = 0.0};
}
}  // namespace

void addClientTimeCalls(ecs::App& app) { app.addCall(std::string{kServerTimeCall}, &clientServerTime); }

void addServerTimeCalls(ecs::App& app) { app.addCall(std::string{kRttCall}, &serverRtt); }

void addStandaloneTimeCalls(ecs::App& app) {
    app.addCall(std::string{kServerTimeCall}, &standaloneServerTime);
    app.addCall(std::string{kRttCall}, &standaloneRtt);
}
}  // namespace aylo::replication::timesync
