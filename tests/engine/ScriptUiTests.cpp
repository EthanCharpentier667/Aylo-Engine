/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptUiTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <utility>

#include "FakeBackend.hpp"
#include "ScriptProbe.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/time/TimePlugin.hpp"
#include "aylo/scripting/ScriptHost.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/format/CapsuleText.hpp"
#include "plugins/ui/UiPlugin.hpp"

namespace {
/// @brief A client App with the drawing plugins over the fake backend, the UI plugin, 0.1 s frames, and a script host.
struct ClientScripts {
    std::shared_ptr<aylo::test::plugins::FakeBackendState> state =
        std::make_shared<aylo::test::plugins::FakeBackendState>();
    aylo::ecs::App app{aylo::ecs::RunMode::kClient};
    std::unique_ptr<aylo::scripting::ScriptHost> host;

    ClientScripts() {
        app.addPlugin(aylo::ecs::TimePlugin::manual(std::chrono::milliseconds{100}));
        aylo::test::plugins::addDrawingPlugins(app, aylo::test::plugins::makeFakeRegistry(state),
                                               aylo::test::plugins::makeFakeSettings());
        app.addPlugin(aylo::plugins::ui::UiPlugin{});
        app.getPluginRegistry().addScriptForms("test.Probe", aylo::tests::kProbeForms);
        auto created = aylo::scripting::ScriptHost::create(app, {.isClient = true});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("ui-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }
};

// The marker of the manual (ui.md, "Moving nodes from a system"), as it is written there.
constexpr const char* kManualMarker = R"(
engine.ecs.component("game.Marker", { fields = { speed = "f32" } })

local spawned = false
engine.ecs.system("game.SpawnMarker", {
	phase = "Update",
	side = "client",
	run = function(ctx)
		if spawned then
			return
		end
		spawned = true
		local marker = engine.ui.spawn(ctx.commands, {
			style = { position = "absolute", left = 0, top = 20, width = 40, height = 12 },
			background = { color = { 1, 0.8, 0.2, 1 }, radius = 2 },
		})
		ctx.commands:add(marker, "game.Marker", { speed = 60 })
	end,
})

engine.ecs.system("game.MoveMarker", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Marker" }, write = { "aylo.UiNode", "aylo.UiBackground" } },
	after = { "game.SpawnMarker" },
	run = function(ctx)
		for _, marker, node, background in ctx.query do
			node.left = node.left + marker.speed * ctx.delta -- pixels per second
			node.width = "10%"
			background.cornerRadius = 6
		end
	end,
})
)";
}  // namespace

TEST(ScriptUi, TheManualsMarkerMovesAndTheLayoutFollows) {
    ClientScripts client;
    ASSERT_EQ(client.run(kManualMarker), "");
    ASSERT_EQ(client.run(R"(
        engine.ecs.system("test.SeeMarker", {
            phase = "Update",
            side = "client",
            query = { read = { "game.Marker", "aylo.UiNode", "aylo.UiComputed", "aylo.UiBackground" } },
            after = { "game.MoveMarker" },
            run = function(ctx)
                probe.markers = 0
                for _, _, node, computed, background in ctx.query do
                    probe.markers += 1
                    probe.left = node.left
                    probe.width = node.width
                    probe.x = computed.position.x
                    probe.sizeX = computed.size.x
                    probe.radius = background.cornerRadius
                end
            end,
        })
    )"),
              "");

    for (int frame = 0; frame < 6; ++frame) {
        client.app.runFrame();
    }

    // Read back as written: a number of pixels and a percentage. The layout of the frame before put the marker where
    // its left said then (6 pixels less: 60 px/s over a frame of 0.1 s), 10% of the 800 pixels of the view wide.
    EXPECT_EQ(client.run(R"(
        assert(probe.markers == 1, tostring(probe.markers))
        assert(type(probe.left) == "number" and probe.left > 6, tostring(probe.left))
        assert(probe.width == "10%", tostring(probe.width))
        assert(math.abs(probe.x - (probe.left - 6)) < 0.5, tostring(probe.x) .. " vs " .. tostring(probe.left))
        assert(math.abs(probe.sizeX - 80) < 0.5, tostring(probe.sizeX))
        assert(probe.radius == 6, tostring(probe.radius))
    )"),
              "");
}

TEST(ScriptUi, AnInterfaceWrittenInACapsuleFileIsLaidOut) {
    // The lengths of a capsule file carry their unit, so an interface is a file written by hand (#168).
    ClientScripts client;
    client.app.addPlugin(aylo::plugins::capsule::CapsulePlugin{});
    auto& capsules = *client.app.getWorld().getResources().getMut<aylo::plugins::capsule::Capsules>();
    // The example of the manual (ui.md, "An interface in a capsule file"), as it is written there.
    capsules.define(aylo::plugins::capsule::format::readCapsule("ui/pause.capsule", R"([capsule format=1]

# ui/pause.capsule: a panel in the middle of the view, with a title and a button.
[node path="."]
aylo.UiNode = { position = "absolute", left = "30%", top = "25%", width = "40%", paddingLeft = 16, paddingTop = 16, paddingRight = 16, paddingBottom = 16 }
aylo.UiBackground = { color = (0.1, 0.1, 0.2, 0.9), cornerRadius = 8 }

[node path="Title" side=client]
aylo.UiNode = { marginBottom = 12 }
aylo.UiText = { text = "Paused", font = "fonts/orbitron.json", size = 32, align = "center" }

[node path="Resume" side=client]
aylo.UiNode = { height = 40, paddingLeft = 16, paddingRight = 16, justify = "center" }
aylo.UiBackground = { color = (0.2, 0.2, 0.4, 1), cornerRadius = 6 }
aylo.UiButton = { action = "pause.resume", hoveredColor = (0.3, 0.3, 0.6, 1) }
aylo.UiInteraction = {}
aylo.UiText = { text = "Resume", font = "fonts/orbitron.json", size = 20, align = "center" }
)",
                                                                client.app.getWorld().getComponents(), capsules));
    ASSERT_EQ(client.run(R"(
        local spawned = false
        engine.ecs.system("test.SpawnPanel", {
            phase = "Update",
            side = "client",
            query = {},
            calls = { "aylo.capsule.spawn" },
            run = function(ctx)
                if not spawned then
                    spawned = true
                    engine.capsule.spawn(ctx, "ui/pause.capsule")
                end
            end,
        })
        local function see(name, query)
            engine.ecs.system("test.See" .. name, {
                phase = "Update",
                side = "client",
                query = query,
                run = function(ctx)
                    for _, node, computed in ctx.query do
                        probe[name] = { x = computed.position.x, y = computed.position.y, width = computed.size.x,
                                        height = computed.size.y, declared = node.width }
                    end
                end,
            })
        end
        see("panel", { read = { "aylo.UiNode", "aylo.UiComputed" }, without = { "aylo.UiText" } })
        see("button", { read = { "aylo.UiNode", "aylo.UiComputed" }, with = { "aylo.UiButton" } })
    )"),
              "");

    for (int frame = 0; frame < 3; ++frame) {
        client.app.runFrame();
    }

    // 40% of the 800 pixels of the view, 30% from its left; the button inside its padding, as wide as it can be.
    EXPECT_EQ(client.run(R"(
        local panel, button = probe.panel, probe.button
        assert(panel and button, "the panel and its button were not laid out")
        assert(panel.declared == "40%", tostring(panel.declared))
        assert(math.abs(panel.x - 240) < 0.5 and math.abs(panel.width - 320) < 0.5, panel.x .. ", " .. panel.width)
        assert(math.abs(button.width - 288) < 0.5 and math.abs(button.height - 40) < 0.5, button.width .. "x" .. button.height)
    )"),
              "");
}
