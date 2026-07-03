# Unreal MCP System Tools

This document describes the **system / introspection** tools in the Unreal MCP
integration. These are "escape hatch" tools that let an AI client reach engine
functionality that does not (yet) have a dedicated wrapper.

> Backend: `FUnrealMCPSystemCommands` (`UnrealMCPSystemCommands.{h,cpp}`) ·
> Python: `Python/tools/system_tools.py` (`register_system_tools`)

## Overview

| Tool | Purpose |
|------|---------|
| `execute_python` | Run a Python script inside the editor (full `unreal` API) |
| `execute_python_file` | Run a `.py` file inside the editor (paths resolve against the project dir) |
| `execute_console_command` | Run an editor console command / set a CVar |
| `get_class_info` | Reflect a `UClass`: parent, properties, functions |
| `list_assets` | Enumerate assets under a content path (Asset Registry) |
| `get_supported_commands` | List every command the C++ bridge supports, with categories |
| `call_unreal` | Generic passthrough to any registered bridge command |

`execute_python` is the highest-leverage tool: anything the editor's `unreal`
Python API can do (materials, Niagara, Sequencer, landscape, level I/O, …) is
reachable through it, even when no dedicated MCP tool exists.

---

## execute_python

Execute a Python script inside the Unreal Editor and return its output.

**Requires:** the **Python Editor Script Plugin** to be enabled (the
`UnrealMCP.uplugin` enables it automatically).

**Parameters:**
- `code` (string) - Python source to run. Multi-line scripts are supported. Use
  `unreal.log(...)` or `print(...)` to surface values into the returned `log` array.
  (At the wire level the handler also accepts the source under a `command` key.)

**Returns:**
- `success` (boolean)
- `result` (string) - repr of the last expression, if any
- `log` (array) - list of `{type, output}` entries captured during the run

**Example:**
```json
{
  "command": "execute_python",
  "params": {
    "code": "import unreal\nprint(unreal.SystemLibrary.get_engine_version())"
  }
}
```

---

## execute_python_file

Execute a Python script **file** inside the Unreal Editor. Relative paths are
resolved against the project directory.

**Requires:** the **Python Editor Script Plugin** (same as `execute_python`).

**Parameters:**
- `file_path` (string) - Path to the `.py` file (absolute, or relative to the
  project directory).

**Returns:**
- `success` (boolean), `file` (string, the resolved path)
- `result` (string), `log` (array of `{type, output}` entries)

**Example:**
```json
{
  "command": "execute_python_file",
  "params": {
    "file_path": "Scripts/setup_level.py"
  }
}
```

---

## execute_console_command

Execute an Unreal Engine console command or set a CVar.

**Parameters:**
- `command` (string) - The console command to run (e.g. `"stat fps"`, `"r.ScreenPercentage 50"`).

**Returns:**
- Dict indicating whether the command was executed.

**Example:**
```json
{
  "command": "execute_console_command",
  "params": {
    "command": "stat fps"
  }
}
```

---

## get_class_info

Reflect a `UClass`: its parent, and the properties and functions it declares.
Use this to discover valid property/function names before calling
`set_actor_property`, `set_component_property`, or `add_blueprint_function_node`.

**Parameters:**
- `class_name` (string) - Engine class name without prefix (e.g. `"Actor"`, `"PointLight"`, `"StaticMeshComponent"`).

**Returns:**
- `name` (string), `path` (string), `parent` (string)
- `properties` (array) - `[{name, type}]`
- `functions` (array) - `[name]`

**Example:**
```json
{
  "command": "get_class_info",
  "params": {
    "class_name": "PointLight"
  }
}
```

---

## list_assets

List assets under a content-browser path using the Asset Registry.

**Parameters:**
- `path` (string, default: `"/Game"`) - Content path to search (e.g. `"/Game/UI"`, `"/Engine/BasicShapes"`).
- `recursive` (boolean, default: `true`) - Whether to recurse into sub-paths.

**Returns:**
- `count` (integer)
- `assets` (array) - `[{name, path, class}]`

**Example:**
```json
{
  "command": "list_assets",
  "params": {
    "path": "/Game",
    "recursive": true
  }
}
```

---

## get_supported_commands

Return the full list of commands supported by the connected C++ bridge. This is
the canonical way to discover what the running editor can do — the list is built
from the bridge's own dispatch registry, so it can never drift from reality.

**Parameters:**
- None

**Returns:**
- `commands` (array) - `[{name, category}]`, sorted alphabetically
- `count` (integer)

**Example:**
```json
{
  "command": "get_supported_commands",
  "params": {}
}
```

---

## call_unreal

Generic escape hatch: send **any** command supported by the C++ bridge with an
arbitrary parameter dictionary. This lets you invoke a bridge command that does
not yet have a dedicated Python wrapper, so the C++ and Python surfaces can never
fully desync from the client side. Prefer a dedicated tool when one exists.

**Parameters:**
- `command` (string) - The command type string the bridge expects (e.g. `"spawn_actor"`).
- `params` (object, optional) - The parameter dictionary for that command.

**Returns:**
- The raw response from Unreal Engine.

**Example:**
```json
{
  "command": "call_unreal",
  "params": {
    "command": "spawn_actor",
    "params": {
      "name": "TempLight",
      "type": "PointLight",
      "location": [0, 0, 300]
    }
  }
}
```

---

## Error Handling

All tools return a `success` flag (or `status`) and a `message` on failure.

```json
{
  "success": false,
  "message": "Failed to connect to Unreal Engine"
}
```

## Implementation Notes

- `execute_python` runs in the editor's Python environment; it has full access to
  the project and engine, so treat it like any other code you run in the editor.
- `execute_python` returns "Python scripting is not available" if the Python Editor
  Script Plugin is disabled — enable it under **Edit → Plugins** and restart.
- `call_unreal` performs no validation; malformed `params` are passed straight to
  the bridge and may return a bridge-level error.
