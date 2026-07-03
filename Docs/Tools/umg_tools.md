# Unreal MCP UMG Tools

This document describes the **UMG (Widget Blueprint)** tools in the Unreal MCP
integration: creating Widget Blueprints, adding Text Block and Button widgets,
binding events and properties, and preparing widgets for viewport display.

> Backend: `FUnrealMCPUMGCommands` (`UnrealMCPUMGCommands.{h,cpp}`) ·
> Python: `Python/tools/umg_tools.py` (`register_umg_tools`)
>
> All Widget Blueprints are created and looked up under the fixed path
> **`/Game/Widgets/`**. Widget creation currently supports **Text Block** and
> **Button** widget types; styling (size, font, colors) is not implemented in the
> C++ handlers yet — use [`execute_python`](system_tools.md#execute_python) for that.

## Overview

| Tool | Purpose |
|------|---------|
| `create_umg_widget_blueprint` | Create a new UMG Widget Blueprint (UserWidget + Canvas Panel root) |
| `add_text_block_to_widget` | Add a Text Block at a canvas position |
| `add_button_to_widget` | Add a Button (with text label) at a canvas position |
| `bind_widget_event` | Create the bound event node (e.g. `OnClicked`) in the event graph |
| `add_widget_to_viewport` | Validate the widget class and return its class path |
| `set_text_block_binding` | Create a Text variable + binding function for a Text Block |

> **Two naming layers.** The MCP tools (Python) accept friendly argument names
> like `widget_name` / `text_block_name`; on the wire, the C++ handlers read
> `blueprint_name` (the Widget Blueprint) and `widget_name` (the child widget).
> The raw-command examples below show the **wire keys** the plugin actually reads.

---

## create_umg_widget_blueprint

Create a new UMG Widget Blueprint at `/Game/Widgets/<name>` with `UserWidget` as
the parent class and a Canvas Panel as the root widget. Custom parent classes and
paths are **not supported** by the C++ handler.

**MCP tool arguments:** `widget_name`

**Wire parameters:**
- `name` (string) - Name of the widget blueprint to create.

**Example (raw command):**
```json
{
  "command": "create_umg_widget_blueprint",
  "params": {
    "name": "WBP_MainMenu"
  }
}
```

**Returns:** the created widget's `name` and `path` (`/Game/Widgets/<name>`).

---

## add_text_block_to_widget

Add a Text Block widget to a Widget Blueprint's root Canvas Panel.

**MCP tool arguments:** `widget_name` (target Blueprint), `text_block_name`,
`text`, `position`

**Wire parameters:**
- `blueprint_name` (string) - Name of the target Widget Blueprint (under `/Game/Widgets/`).
- `widget_name` (string) - Name to give the new Text Block.
- `text` (string, default: `"New Text Block"`) - Initial text content.
- `position` (array, default: `[0, 0]`) - `[X, Y]` position in the canvas panel.

**Example (raw command):**
```json
{
  "command": "add_text_block_to_widget",
  "params": {
    "blueprint_name": "WBP_MainMenu",
    "widget_name": "TitleText",
    "text": "My Game",
    "position": [100, 50]
  }
}
```

---

## add_button_to_widget

Add a Button widget (with a nested text label) to a Widget Blueprint's root
Canvas Panel. The blueprint is compiled and saved afterwards.

**MCP tool arguments:** `widget_name` (target Blueprint), `button_name`, `text`,
`position`

**Wire parameters:**
- `blueprint_name` (string) - Name of the target Widget Blueprint.
- `widget_name` (string) - Name to give the new Button.
- `text` (string, **required**) - Text to display on the button.
- `position` (array, optional) - `[X, Y]` position in the canvas panel.

**Example (raw command):**
```json
{
  "command": "add_button_to_widget",
  "params": {
    "blueprint_name": "WBP_MainMenu",
    "widget_name": "PlayButton",
    "text": "Play",
    "position": [100, 200]
  }
}
```

---

## bind_widget_event

Create (or reuse) the standard bound event node for a widget — e.g. `OnClicked`
for a Button — in the Widget Blueprint's event graph. The blueprint is compiled
and saved afterwards. Custom target function names are **not supported**;
implement the event body with the [Blueprint node tools](node_tools.md).

**MCP tool arguments:** `widget_name` (target Blueprint), `widget_component_name`,
`event_name`

**Wire parameters:**
- `blueprint_name` (string) - Name of the target Widget Blueprint.
- `widget_name` (string) - Name of the widget component whose event is bound.
- `event_name` (string) - Name of the event to bind (e.g. `OnClicked`).

**Example (raw command):**
```json
{
  "command": "bind_widget_event",
  "params": {
    "blueprint_name": "WBP_MainMenu",
    "widget_name": "PlayButton",
    "event_name": "OnClicked"
  }
}
```

---

## add_widget_to_viewport

Validate a Widget Blueprint's generated class and return its class path.

> ⚠️ **This does not display the widget.** Adding a widget to a viewport
> requires a game context; the handler returns the class path plus a note telling
> you to use `CreateWidget` + `AddToViewport` Blueprint nodes in game.

**MCP tool arguments:** `widget_name`, `z_order`

**Wire parameters:**
- `blueprint_name` (string) - Name of the Widget Blueprint.
- `z_order` (integer, default: `0`) - Echoed back in the response.

**Example (raw command):**
```json
{
  "command": "add_widget_to_viewport",
  "params": {
    "blueprint_name": "WBP_MainMenu",
    "z_order": 0
  }
}
```

**Returns:** `blueprint_name`, `class_path`, `z_order`, and a usage `note`.

---

## set_text_block_binding

Create a **Text** member variable named after the binding plus a
`Get<binding_name>` function graph wired to return it, for use as a Text Block
property binding. Only Text bindings are supported. The blueprint is compiled and
saved afterwards.

**MCP tool arguments:** `widget_name` (target Blueprint), `text_block_name`,
`binding_property`

**Wire parameters:**
- `blueprint_name` (string) - Name of the target Widget Blueprint.
- `widget_name` (string) - Name of the Text Block to bind.
- `binding_name` (string) - Name of the variable/binding function to create.

**Example (raw command):**
```json
{
  "command": "set_text_block_binding",
  "params": {
    "blueprint_name": "WBP_HUD",
    "widget_name": "ScoreText",
    "binding_name": "CurrentScore"
  }
}
```

---

## Error Handling

Bridge-level errors come back as a `status`/`error` pair. Some UMG handlers also
return a bare `{"error": "..."}` object for missing parameters.

```json
{
  "status": "error",
  "error": "Widget Blueprint 'WBP_MainMenu' already exists"
}
```

## Implementation Notes

- Only **Text Block** and **Button** widget types are supported for direct
  addition today. Other widget types (and all styling) can be authored via
  `execute_python`.
- All UMG handlers load the target Widget Blueprint from `/Game/Widgets/<name>`.
- `add_button_to_widget`, `bind_widget_event` and `set_text_block_binding`
  compile **and save** the Widget Blueprint; `create_umg_widget_blueprint` and
  `add_text_block_to_widget` compile it and mark the package dirty (save with
  [`save_all`](level_tools.md#save_all) or [`save_asset`](asset_tools.md#save_asset)).
