# Unreal MCP Project Tools

This document describes the **project** tools in the Unreal MCP integration:
project-wide settings and configuration.

> Backend: `FUnrealMCPProjectCommands` (`UnrealMCPProjectCommands.{h,cpp}`) ·
> Python: `Python/tools/project_tools.py` (`register_project_tools`)

## Overview

| Tool | Purpose |
|------|---------|
| `create_input_mapping` | Create a legacy input action/axis mapping |

> ⚠️ **Legacy input only.** `create_input_mapping` writes to the legacy
> `PlayerInput` action/axis mappings, **not** the Enhanced Input system. Enhanced
> Input (`InputAction` / `InputMappingContext`) is not yet wrapped; author it via
> [`execute_python`](system_tools.md#execute_python) for now.

---

## create_input_mapping

Create a legacy input **Action** mapping for the project and save the input
settings config.

**Parameters:**
- `action_name` (string) - Name of the input action.
- `key` (string) - Key to bind (e.g. `SpaceBar`, `LeftMouseButton`).
- `shift` / `ctrl` / `alt` / `cmd` (boolean, optional) - Modifier flags
  (accepted at the bridge level; not exposed by the Python MCP tool yet).

> The Python tool also accepts an `input_type` argument, but it is **currently
> ignored by the C++ handler** — only Action mappings are created. Axis mappings
> are not supported yet; use `execute_python` for those.

**Returns:**
- The created mapping's `action_name` and `key`.

**Example:**
```json
{
  "command": "create_input_mapping",
  "params": {
    "action_name": "Jump",
    "key": "SpaceBar"
  }
}
```

---

## Error Handling

All command responses include a `success` field and an optional `message` field
with details on failure.

```json
{
  "success": false,
  "message": "Failed to connect to Unreal Engine"
}
```

## Implementation Notes

- This tool is also referenced from the [Node Tools](node_tools.md) documentation
  because it is commonly used alongside `add_blueprint_input_action_node`, but it
  is implemented in `project_tools.py` / `FUnrealMCPProjectCommands`.
- For modern projects, prefer Enhanced Input via `execute_python` until dedicated
  Enhanced Input tools land (see `Docs/NEW_OPERATIONS_PROPOSAL.md`).
