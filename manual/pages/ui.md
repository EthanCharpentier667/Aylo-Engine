# User interface

The UI plugin lets you build menus, HUDs and dialogs from plain tables. Every piece of interface is an entity (a
*node*) laid out with flexbox, drawn on top of the game, in pixels of the view. Nothing has a fixed look: a button is
any node that has a `button` part, whatever else it holds.

The UI exists on the client only. Turn it on in `project.luau`:

```luau
if engine.isClient then
	engine.settings.plugins = { "aylo.Ui" }
end
```

`aylo.Ui` brings `aylo.graphics`, `aylo.Input` and `aylo.Text` with it. You still need a font for any text, see
[Setup forms](setup-forms.md#engineassets).

## A first menu

```luau
local font = engine.assets.font("fonts/orbitron.json")

engine.ecs.system("game.SpawnMenu", {
	phase = "Startup",
	side = "client",
	query = {},
	run = function(ctx)
		engine.ui.spawn(ctx.commands, {
			style = { width = "100%", height = "100%", justify = "center", alignItems = "center" },
			children = {
				{
					style = { padding = 16, alignItems = "center" },
					background = { color = { 0.1, 0.1, 0.2, 1 }, radius = 8 },
					button = { action = "menu.play", hovered = { 0.2, 0.2, 0.4, 1 } },
					children = { { style = { ignorePointer = true }, text = { text = "Play", font = font, size = 24 } } },
				},
			},
		})
	end,
})

engine.ecs.system("game.ReadMenu", {
	phase = "Update",
	side = "client",
	query = {},
	events = { read = { "aylo.UiClicked" } },
	run = function(ctx)
		for click in ctx.events.read("aylo.UiClicked") do
			if click.action == "menu.play" then
				print("let's play")
			end
		end
	end,
})
```

A complete menu with every widget is in `examples/plugins/ui/game/`.

## `engine.ui.spawn`

`engine.ui.spawn(commands, spec, parent)` spawns a tree of nodes and returns the root entity (or `nil` on a server).
`commands` is `ctx.commands`; `parent` is optional. Every key of `spec` adds one component to the node; any other key
is an error that names the form.

| Key | Component | Fields |
|---|---|---|
| `style` | `aylo.UiNode` | the layout, see [Layout](#layout) |
| `background` | `aylo.UiBackground` | `color`, `radius`, `borderColor`, `borderWidth` |
| `image` | `aylo.UiImage` | `texture`, `source`, `color`, `slice`, `sliceScale` |
| `text` | `aylo.UiText` | `text`, `font`, `size`, `color`, `align` (`"left"`, `"center"`, `"right"`), `wrap` |
| `button` | `aylo.UiButton` | `action`, `normal`, `hovered`, `pressed`, `disabled` (colors) |
| `toggle` | `aylo.UiToggle` | `action`, `checked`, `mark` (color), `inset` |
| `slider` | `aylo.UiSlider` | `action`, `min`, `max`, `value`, `step`, `track`, `fill`, `handle` (colors), `trackHeight`, `handleRadius` |
| `input` | `aylo.UiTextInput` | `action`, `placeholder`, `maxLength`, `caret` (color) |
| `scroll` | `aylo.UiScroll` | `speed`; also sets `overflow = "scroll"` |
| `interaction` | `aylo.UiInteraction` | `disabled`, `focusable` |
| `components` | any | your own components, as in `ctx.commands:spawn` |
| `children` | | a list of specs, spawned as children of this node |

Notes:

- Colors are `{ r, g, b, a }` (the alpha may be left out and is then 1). They are linear, like every color of the
  engine: on an sRGB window `0.25` looks lighter than `#404040`.
- `image.slice` is a number (the same border on four sides) or `{ left, top, right, bottom }`, for a nine-patch.
- A button, toggle, slider or input also gets an `aylo.UiInteraction`, so it can be hovered and focused.
- An `input` needs a `text`; it is added for you when you leave it out, and the typed text is read from it.

## Layout

A `style` is a plain table, so sharing one is sharing a table. A **list of styles** is merged in order, the last one
winning:

```luau
local row = { direction = "row", alignItems = "center", gap = 12 }
style = { row, { padding = 16 } }
```

Lengths are a number of pixels, a percentage such as `"50%"`, or `"auto"`.

| Style | Values |
|---|---|
| `direction` | `"column"` (default), `"column-reverse"`, `"row"`, `"row-reverse"` |
| `justify` | `"start"`, `"center"`, `"end"`, `"space-between"`, `"space-around"`, `"space-evenly"` |
| `alignItems`, `alignSelf`, `alignContent` | `"auto"`, `"start"`, `"center"`, `"end"`, `"stretch"`, `"baseline"`, and for `alignContent` the `"space-*"` ones |
| `wrap` | `"nowrap"`, `"wrap"`, `"wrap-reverse"` |
| `grow`, `shrink`, `aspectRatio` | numbers |
| `basis`, `width`, `height`, `minWidth`, `minHeight`, `maxWidth`, `maxHeight` | lengths |
| `position` | `"relative"` (default), `"absolute"`; with `left`, `top`, `right`, `bottom` (lengths) |
| `margin`, `marginX`, `marginY`, `marginLeft`, `marginTop`, `marginRight`, `marginBottom` | lengths |
| `padding`, `paddingX`, `paddingY`, `paddingLeft`, `paddingTop`, `paddingRight`, `paddingBottom` | lengths |
| `gap`, `rowGap`, `columnGap`, `border` | pixels |
| `overflow` | `"visible"` (default), `"hidden"`, `"scroll"` |
| `display` | `"flex"` (default), `"none"` |
| `zIndex` | draw order among siblings |
| `ignorePointer` | `true` lets the pointer through to what is below |

There are no anchors: an absolute node with insets is one. `{ position = "absolute", right = 16, top = 16 }` stays in
the top-right corner of its parent, whatever the size of the window.

A node with no parent node is a root and is laid out over the whole view. The result of the layout (position, size) is
in the node's `aylo.UiComputed` component, which you can read.

## Reacting

A widget reports with the `action` you gave it: the name is yours, and one system can serve a whole menu. The UI runs
in `PreUpdate`, so a system in `Update` reads what happened in the same frame.

| Event | Sent by | Fields |
|---|---|---|
| `aylo.UiClicked` | a button | `entity`, `action` |
| `aylo.UiValueChanged` | a slider, a toggle (`0` or `1`) | `entity`, `action`, `value` |
| `aylo.UiTextChanged` | a text input | `entity`, `action`; the text is in its `aylo.UiText` |
| `aylo.UiTextSubmitted` | a text input, on Enter | `entity`, `action` |

Declare them under `events = { read = {...} }` as in the first menu. For anything the widgets do not cover, any node can
carry an `aylo.UiInteraction` (`hovered`, `pressed`, `focused`, `clicked`) that a system reads.

Two resources help the gameplay get out of the UI's way; read them with `resources = { read = {...} }`:

| Resource | Fields |
|---|---|
| `aylo.UiPointer` | `position` (view pixels), `overUi`, `hovered`, `pressed`, `clicked` |
| `aylo.UiFocus` | `focused`, `typing`, `activated`, `adjust` |

The UI blocks nothing by itself. A game that shoots on a click checks `overUi` first, and ignores the keyboard while
`typing` is `true`.

## Keyboard and gamepad

Every widget works with the pointer, the keyboard and a gamepad.

| Input | Effect |
|---|---|
| Tab, Shift+Tab | The next or previous widget, in draw order |
| Arrows, directional pad | The nearest widget in that direction; left and right adjust a slider |
| Enter, Space, South button | Activates the focused widget |
| Escape, East button | Drops the focus |

## Pitfalls

- Text is drawn with a font atlas: a `text` without a `font` shows nothing. Make the font with `engine.assets.font`.
- A label inside a button swallows the click unless it has `style = { ignorePointer = true }`.
- The UI is drawn on the screen only: not in the world, and not in perspective.
- A text input is a single line, with no selection or clipboard, and wider text overflows the field. A slider is
  horizontal, and a scroll area has no scroll bar. Wrapped text breaks between words only.
- An unknown style name or value is an error such as `unknown justify 'middle'`, raised when the system runs.

## See also

[Systems](systems.md) for phases and events, [Setup forms](setup-forms.md) for fonts and textures,
[Project settings](project-settings.md) for the plugin list.
