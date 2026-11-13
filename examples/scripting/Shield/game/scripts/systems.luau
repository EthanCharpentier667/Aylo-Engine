-- main.luau requires shield.luau first, which declares "game.Shield" before this one uses it.

-- Startup runs once: it makes one shielded ship. It visits no entity, so it has no query.
engine.ecs.system("game.SpawnShip", {
    phase = "Startup",
    side = "server", -- gameplay: only the server spawns
    run = function(ctx)
        ctx.commands:spawn({ ["game.Shield"] = { strength = 10, regen = 5 } })
        print("spawned a ship")
    end,
})

engine.ecs.system("game.ShieldRegen", {
    phase = "FixedUpdate",
    side = "server", -- authoritative gameplay: only the server regenerates
    query = { write = { "game.Shield" } },
    run = function(ctx)
        for entity, shield in ctx.query do
            shield.strength = math.min(shield.strength + shield.regen * ctx.fixedDelta, 100)
            print(string.format("shield strength: %.1f", shield.strength))
        end
    end,
})
