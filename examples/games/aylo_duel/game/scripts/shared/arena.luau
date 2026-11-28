-- The rules both sides share: the arena's constants and two helpers. A module that needs them requires this one; the
-- table it returns is frozen, so no module can change the rules for the others.

local Duel = {
	viewWidth = 800,
	viewHeight = 600,
	middleX = 400, -- each ship stays on its side of it
	shipSpeed = 300, -- world units per second
	laserSpeed = 600,
	shipHalfSize = { x = 24, y = 24 },
	laserHalfSize = { x = 30, y = 6 },
	playerCount = 2,
	misfireOdds = 10, -- one shot in ten misfires
	rematchDelay = 4, -- seconds between a hit and the next round
	joinTimeout = 5, -- seconds a client looks for the server before it gives up

	-- The collision categories: each Collider belongs to some (layer) and collides with some (mask).
	obstacleLayer = 1,
	laserLayer = 2,
	leftShipLayer = 4,
	rightShipLayer = 8,

	-- The capsules (assets/): the arena, what the server spawns in it, and what the clients spawn for the show.
	arena = "scenes/arena.scene",
	ships = { [0] = "actors/ship_left.actor", [1] = "actors/ship_right.actor" },
	misfire = "actors/misfire.actor",
	explosion = "actors/explosion.actor",
	planets = { "actors/planet_ring.actor", "actors/planet_big.actor" },
	lasers = { [0] = "actors/laser.actor", [1] = "actors/laser_left.actor" }, -- by shooter: flying right, flying left
	screens = { Menu = "ui/menu.capsule", Joining = "ui/joining.capsule" }, -- the menu's, by game.Screen

	-- Whether players join through the network. Run alone (`aylo-editor`), the server and the client are one
	-- process with no network: there are no players, and the scripts that need them stay out.
	networked = not (engine.isServer and engine.isClient),
}

--- +1 for the left player (0), who fires right; -1 for the right one (1).
function Duel.directionOf(player: number): number
	return if player == 0 then 1 else -1
end

--- The collision layer of a player's ship.
function Duel.shipLayerOf(player: number): number
	return if player == 0 then Duel.leftShipLayer else Duel.rightShipLayer
end

return table.freeze(Duel)
