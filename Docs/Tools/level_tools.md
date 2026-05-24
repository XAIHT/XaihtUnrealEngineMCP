# Unreal MCP Level / World Tools

This document describes the **level / world** tools in the Unreal MCP integration:
loading, creating and saving levels (maps).

> Backend: `FUnrealMCPLevelCommands` (`UnrealMCPLevelCommands.{h,cpp}`) ·
> Python: `Python/tools/level_tools.py` (`register_level_tools`)

## Overview

| Tool | Purpose |
|------|---------|
| `open_level` | Open (load) a map by content path |
| `save_current_level` | Save the level currently being edited |
| `save_all` | Save all dirty map and content packages |
| `new_level` | Create a new blank level (optionally save it) |
| `get_current_level` | Get the current level's name, path and actor count |

---

## open_level

Open (load) a level/map by content path.

**Parameters:**
- `path` (string) - Package path of the map, e.g. `"/Game/Maps/MyLevel"`.

**Returns:**
- Result of the open operation including success status.

**Example:**
```json
{
  "command": "open_level",
  "params": {
    "path": "/Game/Maps/MyLevel"
  }
}
```

---

## save_current_level

Save the level that is currently being edited.

**Parameters:**
- None

**Example:**
```json
{
  "command": "save_current_level",
  "params": {}
}
```

---

## save_all

Save all dirty map and content packages.

**Parameters:**
- None

**Example:**
```json
{
  "command": "save_all",
  "params": {}
}
```

---

## new_level

Create a new blank level.

**Parameters:**
- `path` (string, optional) - Package path to save the new level to (e.g.
  `"/Game/Maps/New"`). If omitted, the level is created but not saved.

**Example:**
```json
{
  "command": "new_level",
  "params": {
    "path": "/Game/Maps/New"
  }
}
```

---

## get_current_level

Return the current level's name, package path and actor count.

**Parameters:**
- None

**Returns:**
- `name`, `path`, and the actor `count` for the current level.

**Example:**
```json
{
  "command": "get_current_level",
  "params": {}
}
```

---

## Error Handling

All tools return a `success` flag (or `status`) and a `message` on failure.

```json
{
  "success": false,
  "message": "Failed to open level '/Game/Maps/Missing'"
}
```

## Implementation Notes

- `open_level` loads the map in the running editor; unsaved changes in the current
  level should be saved first with `save_current_level` to avoid losing work.
- `new_level` without a `path` creates an in-memory level you must later save.
