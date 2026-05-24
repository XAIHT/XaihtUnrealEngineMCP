# Unreal MCP UMG Tools

This document describes the **UMG (Widget Blueprint)** tools in the Unreal MCP
integration: creating Widget Blueprints, adding Text Block and Button widgets,
binding events and properties, and adding widgets to the viewport.

> Backend: `FUnrealMCPUMGCommands` (`UnrealMCPUMGCommands.{h,cpp}`) ·
> Python: `Python/tools/umg_tools.py` (`register_umg_tools`)
>
> Widget creation currently supports **Text Block** and **Button** widget types.

## Overview

| Tool | Purpose |
|------|---------|
| `create_umg_widget_blueprint` | Create a new UMG Widget Blueprint |
| `add_text_block_to_widget` | Add a Text Block with customizable styling |
| `add_button_to_widget` | Add a Button with text and styling |
| `bind_widget_event` | Bind a widget event (e.g. `OnClicked`) to a function |
| `add_widget_to_viewport` | Add a widget instance to the game viewport |
| `set_text_block_binding` | Set up a dynamic property binding for a Text Block |

---

## create_umg_widget_blueprint

Create a new UMG Widget Blueprint.

**Parameters:**
- `widget_name` (string) - Name of the widget blueprint to create.
- `parent_class` (string, default: `"UserWidget"`) - Parent class for the widget.
- `path` (string, default: `"/Game/UI"`) - Content path where the widget is created.

**Example:**
```json
{
  "command": "create_umg_widget_blueprint",
  "params": {
    "widget_name": "WBP_MainMenu",
    "parent_class": "UserWidget",
    "path": "/Game/UI"
  }
}
```

---

## add_text_block_to_widget

Add a Text Block widget to a UMG Widget Blueprint.

**Parameters:**
- `widget_name` (string) - Name of the target Widget Blueprint.
- `text_block_name` (string) - Name to give the new Text Block.
- `text` (string, default: `""`) - Initial text content.
- `position` (array, default: `[0, 0]`) - `[X, Y]` position in the canvas panel.
- `size` (array, default: `[200, 50]`) - `[Width, Height]` of the text block.
- `font_size` (integer, default: `12`) - Font size in points.
- `color` (array, default: `[1, 1, 1, 1]`) - `[R, G, B, A]` color values (0.0–1.0).

**Example:**
```json
{
  "command": "add_text_block_to_widget",
  "params": {
    "widget_name": "WBP_MainMenu",
    "text_block_name": "TitleText",
    "text": "My Game",
    "position": [100, 50],
    "size": [400, 80],
    "font_size": 36,
    "color": [1, 1, 1, 1]
  }
}
```

---

## add_button_to_widget

Add a Button widget to a UMG Widget Blueprint.

**Parameters:**
- `widget_name` (string) - Name of the target Widget Blueprint.
- `button_name` (string) - Name to give the new Button.
- `text` (string, default: `""`) - Text to display on the button.
- `position` (array, default: `[0, 0]`) - `[X, Y]` position in the canvas panel.
- `size` (array, default: `[200, 50]`) - `[Width, Height]` of the button.
- `font_size` (integer, default: `12`) - Font size for button text.
- `color` (array, default: `[1, 1, 1, 1]`) - `[R, G, B, A]` text color values.
- `background_color` (array, default: `[0.1, 0.1, 0.1, 1]`) - `[R, G, B, A]` button background color.

**Example:**
```json
{
  "command": "add_button_to_widget",
  "params": {
    "widget_name": "WBP_MainMenu",
    "button_name": "PlayButton",
    "text": "Play",
    "position": [100, 200],
    "size": [200, 60]
  }
}
```

---

## bind_widget_event

Bind an event on a widget component to a function.

**Parameters:**
- `widget_name` (string) - Name of the target Widget Blueprint.
- `widget_component_name` (string) - Name of the widget component (button, etc.).
- `event_name` (string) - Name of the event to bind (e.g. `OnClicked`).
- `function_name` (string, optional) - Function to create/bind to. Defaults to
  `"{widget_component_name}_{event_name}"` when omitted.

**Example:**
```json
{
  "command": "bind_widget_event",
  "params": {
    "widget_name": "WBP_MainMenu",
    "widget_component_name": "PlayButton",
    "event_name": "OnClicked",
    "function_name": "OnPlayClicked"
  }
}
```

---

## add_widget_to_viewport

Add a Widget Blueprint instance to the viewport.

**Parameters:**
- `widget_name` (string) - Name of the Widget Blueprint to add.
- `z_order` (integer, default: `0`) - Z-order (higher numbers appear on top).

**Example:**
```json
{
  "command": "add_widget_to_viewport",
  "params": {
    "widget_name": "WBP_MainMenu",
    "z_order": 0
  }
}
```

---

## set_text_block_binding

Set up a property binding for a Text Block widget so its content updates
dynamically instead of being set directly.

**Parameters:**
- `widget_name` (string) - Name of the target Widget Blueprint.
- `text_block_name` (string) - Name of the Text Block to bind.
- `binding_property` (string) - Name of the property to bind to.
- `binding_type` (string, default: `"Text"`) - Type of binding (Text, Visibility, etc.).

**Example:**
```json
{
  "command": "set_text_block_binding",
  "params": {
    "widget_name": "WBP_HUD",
    "text_block_name": "ScoreText",
    "binding_property": "CurrentScore",
    "binding_type": "Text"
  }
}
```

---

## Error Handling

All tools return a `success` flag (or `status`) and a `message` on failure.

```json
{
  "success": false,
  "message": "Widget Blueprint 'WBP_MainMenu' not found"
}
```

## Implementation Notes

- Only **Text Block** and **Button** widget types are supported for direct
  addition today. Other widget types can be authored via `execute_python`.
- Use `set_text_block_binding` for dynamic values rather than re-setting text.
- Bind events to functions with `bind_widget_event`, then implement the function
  body with the Blueprint node tools.
