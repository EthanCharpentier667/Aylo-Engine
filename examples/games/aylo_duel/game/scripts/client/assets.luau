-- The ids of the assets the client's scripts use: the systems that draw and play need them. Each form loads its asset
-- (a second load of the same file does nothing). What only the capsules name needs no line here: they load it
-- themselves (the planets' images, the explosion's atlas, the arena's font).

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

	-- The ships and the lasers the server spawns get their look from these (client/sprites.luau): their frames and
	-- animations are in the .atlas files.
	leftShip = engine.assets.atlas({ path = "atlases/ship_left.atlas" }),
	rightShip = engine.assets.atlas({ path = "atlases/ship_right.atlas" }),
	laser = engine.assets.atlas({ path = "atlases/laser.atlas" }),

	-- The winner's texts (client/hud.luau) and the menu's.
	font = engine.assets.font("fonts/orbitron.json"),

	-- The music streams: it is loaded here, with its option, before anything else asks for it.
	music = engine.assets.sound("maint.ogg", { stream = true }),
	laserSound = engine.assets.sound("laser.mp3"),
	misfireSound = engine.assets.sound("laser-no.mp3"),

	background = engine.assets.tilemap("levels/space-background.tmj"),
}
