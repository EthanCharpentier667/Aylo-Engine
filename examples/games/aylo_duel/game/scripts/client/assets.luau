-- What the client's scripts use and no capsule can load by itself: the sprite atlases, the font, the sounds, the
-- background map and the menu's pieces. The forms load them; the module returns their ids, for the client modules that
-- use them. The images a capsule names (the planets: assets/actors/planet_*.actor) are loaded by the capsule, and a
-- capsule names an atlas or the font by the name given here (actors/explosion.actor, scenes/arena.scene).

-- The menu's pieces are pixel art (assets/ui, from a UI pack): an image is drawn without smoothing by default.
local pixelArt = engine.assets.texture

return {
	-- The menu: a panel with a roof, its buttons at rest, under the pointer and held, a loading wheel (11 frames of
	-- 16x16 in a row) and a planet for the roof.
	ui = {
		titlePanel = pixelArt("ui/title_panel.png"),
		panel = pixelArt("ui/panel.png"),
		button = {
			normal = pixelArt("ui/button_normal.png"),
			hovered = pixelArt("ui/button_hover.png"),
			pressed = pixelArt("ui/button_clicked.png"),
		},
		loadingWheel = pixelArt("ui/loading_wheel.png"),
		planet = pixelArt("ui/planet.png"),
	},

	-- The left sheet: 5 frames of 48x48, every 65 pixels.
	leftShip = engine.assets.atlas({
		name = "game.leftShip",
		texture = "spritesheet24.gif",
		grid = { frameSize = { x = 48, y = 48 }, count = 5, spacing = { x = 17, y = 0 }, offset = { x = 9, y = 9 } },
		animations = { fly = { frames = { 0, 1, 2, 3, 4 }, frameDuration = 0.1 } },
	}),
	-- The right sheet: 3 frames of 63x48 facing left, every 65 pixels.
	rightShip = engine.assets.atlas({
		name = "game.rightShip",
		texture = "spritesheet26.gif",
		grid = { frameSize = { x = 63, y = 48 }, count = 3, spacing = { x = 2, y = 0 }, offset = { x = 1, y = 1 } },
		animations = { fly = { frames = { 0, 1, 2 }, frameDuration = 0.1, direction = "pingPong" } },
	}),
	-- Two frames of 80x16 flying right from x 108, then two flying left from x 271, every 81 pixels.
	laser = engine.assets.atlas({
		name = "game.laser",
		texture = "bullet.gif",
		frames = {
			{ position = { x = 108, y = 151 }, size = { x = 80, y = 16 } },
			{ position = { x = 189, y = 151 }, size = { x = 80, y = 16 } },
			{ position = { x = 271, y = 151 }, size = { x = 80, y = 16 } },
			{ position = { x = 352, y = 151 }, size = { x = 80, y = 16 } },
		},
		animations = {
			right = { frames = { 0, 1 }, frameDuration = 0.08 },
			left = { frames = { 2, 3 }, frameDuration = 0.08 },
		},
	}),
	-- The big explosions, the bottom row: 10 frames of 64x64, every 65 pixels; the first six burst and fade, once.
	-- actors/explosion.actor plays it, by its name.
	explosion = engine.assets.atlas({
		name = "game.explosion",
		texture = "fx-explosions.gif",
		grid = { frameSize = { x = 64, y = 64 }, count = 10, spacing = { x = 1, y = 0 }, offset = { x = 1, y = 99 } },
		animations = { explode = { frames = { 0, 1, 2, 3, 4, 5 }, frameDuration = 0.07, plays = 1 } },
	}),

	-- The texts of scenes/arena.scene name it by its path.
	font = engine.assets.font("fonts/orbitron.json"),

	music = engine.assets.sound("maint.ogg", { stream = true }),
	laserSound = engine.assets.sound("laser.mp3"),
	misfireSound = engine.assets.sound("laser-no.mp3"),

	background = engine.assets.tilemap("levels/space-background.tmj"),
}
