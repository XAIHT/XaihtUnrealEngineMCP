# Unreal MCP Actor Tools

This document provides detailed information about the actor tools available in the Unreal MCP integration.

## Overview

Actor tools allow you to query and manipulate actors in the current Unreal Engine
level.

> Backend: actor commands are handled by `FUnrealMCPEditorCommands`
> (`UnrealMCPEditorCommands.{h,cpp}`) · Python: `Python/tools/editor_tools.py`
> (`register_editor_tools`). Viewport/screenshot tools from the same module are
> documented in [Editor Tools](editor_tools.md).

## Actor Tools

### get_actors_in_level

Get a list of all actors in the current level.

**Parameters:**
- None

**Returns:**
- List of all actors with their properties

**Example:**
```json
{
  "command": "get_actors_in_level",
  "params": {}
}
```

### find_actors_by_name

Find actors in the current level by name pattern.

**Parameters:**
- `pattern` (string) - Substring to match against actor names (`Contains` match,
  no wildcards)

**Returns:**
- List of matching actors with their properties (same shape as
  `get_actors_in_level`)

**Example:**
```json
{
  "command": "find_actors_by_name",
  "params": {
    "pattern": "Cube"
  }
}
```

### spawn_actor

Create a new actor in the current level.

> The command `create_actor` is kept as a **legacy alias** that routes to the same
> handler; new code should use `spawn_actor`.

**Parameters:**
- `name` (string) - The name for the new actor (must be unique in the level; the
  handler rejects duplicates)
- `type` (string) - The type of actor to create (matched **exactly,
  case-sensitively** — see [Actor Types](#actor-types) below)
- `location` (array, optional) - `[X, Y, Z]` coordinates for the actor's position, defaults to `[0, 0, 0]`
- `rotation` (array, optional) - `[Pitch, Yaw, Roll]` values for the actor's rotation, defaults to `[0, 0, 0]`
- `scale` (array, optional) - `[X, Y, Z]` scale applied after spawning, defaults to `[1, 1, 1]`

**Returns:**
- Information about the created actor

**Example:**
```json
{
  "command": "spawn_actor",
  "params": {
    "name": "KeyLight",
    "type": "PointLight",
    "location": [0, 0, 300],
    "rotation": [0, 0, 0]
  }
}
```

### delete_actor

Delete an actor by name.

**Parameters:**
- `name` (string) - The name of the actor to delete

**Returns:**
- Result of the delete operation

**Example:**
```json
{
  "command": "delete_actor",
  "params": {
    "name": "KeyLight"
  }
}
```

### set_actor_transform

Set the transform (location, rotation, scale) of an actor.

**Parameters:**
- `name` (string) - The name of the actor to modify
- `location` (array, optional) - `[X, Y, Z]` coordinates for the actor's position
- `rotation` (array, optional) - `[Pitch, Yaw, Roll]` values for the actor's rotation
- `scale` (array, optional) - `[X, Y, Z]` values for the actor's scale

**Returns:**
- Result of the transform operation

**Example:**
```json
{
  "command": "set_actor_transform",
  "params": {
    "name": "KeyLight",
    "location": [100, 200, 300],
    "rotation": [0, 90, 0],
    "scale": [2, 2, 2]
  }
}
```

### get_actor_properties

Get all properties of an actor.

**Parameters:**
- `name` (string) - The name of the actor

**Returns:**
- Object containing all actor properties

**Example:**
```json
{
  "command": "get_actor_properties",
  "params": {
    "name": "KeyLight"
  }
}
```

### set_actor_property

Set a single property on an actor.

**Parameters:**
- `name` (string) - The name of the actor
- `property_name` (string) - The name of the property to set
- `property_value` (any) - The value to set the property to

> Tip: use [`get_class_info`](system_tools.md#get_class_info) to discover valid
> property names for an actor's class before calling this.

**Returns:**
- Result of the property setting operation

**Example:**
```json
{
  "command": "set_actor_property",
  "params": {
    "name": "KeyLight",
    "property_name": "bHidden",
    "property_value": false
  }
}
```

## Error Handling

Bridge-level errors come back as a `status`/`error` pair; the Python tools may
also return `success`/`message` for connection problems.

```json
{
  "status": "error",
  "error": "Actor not found: MyCube"
}
```

## Implementation Notes

- All numeric parameters for transforms (location, rotation, scale) must be provided as lists of 3 float values
- Actor `type` matching is **exact and case-sensitive** (`"PointLight"`, not `"pointlight"`)
- The server maintains logging of all operations with detailed information and error messages
- All commands are executed through a connection to the Unreal Engine editor

## Type Reference

### Actor Types

Actor types accepted by the `spawn_actor` command (matched exactly, case-sensitively):

- `StaticMeshActor` - Empty static mesh actor (set its mesh afterwards via
  `set_actor_property` / a Blueprint, or use a Blueprint actor)
- `PointLight` - Point light source
- `SpotLight` - Spot light source
- `DirectionalLight` - Directional light source
- `CameraActor` - Camera actor

To spawn an instance of a Blueprint class instead, use
[`spawn_blueprint_actor`](blueprint_tools.md#spawn_blueprint_actor).

## Related Tools

The following categories — once listed here as "planned" — are now implemented and
documented separately:

- **[Level / World Tools](level_tools.md)** - open/save/create levels
- **[Material Tools](material_tools.md)** - create materials and assign them to actors
- **[Blueprint Tools](blueprint_tools.md)** - create and configure Blueprints
- **[Asset Tools](asset_tools.md)** - import/duplicate/rename/delete assets
- **[Editor Tools](editor_tools.md)** - viewport focus and screenshots
