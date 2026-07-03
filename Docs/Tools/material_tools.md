# Unreal MCP Material Tools

This document describes the **material** tools in the Unreal MCP integration:
creating materials and material instances, setting instance parameters, and
assigning materials to level actors.

> Backend: `FUnrealMCPMaterialCommands` (`UnrealMCPMaterialCommands.{h,cpp}`) ·
> Python: `Python/tools/material_tools.py` (`register_material_tools`)
>
> **Two naming layers.** The MCP tools (Python) accept friendly argument names
> (`material`, `parameter`, `actor`, `slot`); on the wire, the C++ handlers read
> `material_path`, `parameter_name`, `actor_name`, `slot_index`. The raw-command
> examples below show the **wire keys** the plugin actually reads.

## Overview

| Tool | Purpose |
|------|---------|
| `create_material` | Create a new Material asset |
| `create_material_instance` | Create a Material Instance Constant from a parent material |
| `set_material_parameter` | Set a scalar, vector/color, or texture parameter on a Material Instance |
| `assign_material` | Assign a material to a level actor's mesh components at a slot |
| `set_material_color` | Convenience one-call vector/color setter |
| `get_material_info` | Query a material's name, path and class |
| `assign_material_to_all_slots` | Assign a material to every slot of every mesh component |

---

## create_material

Create a new Material asset.

**Wire parameters:**
- `name` (string) - Name of the material asset.
- `path` (string) - Content path to create it in (Python tool default: `"/Game/Materials"`).

**Example:**
```json
{
  "command": "create_material",
  "params": {
    "name": "M_Base",
    "path": "/Game/Materials"
  }
}
```

---

## create_material_instance

Create a Material Instance Constant from a parent material.

**Wire parameters:**
- `name` (string) - Name of the material instance asset.
- `parent_material` (string) - Path of the parent material, e.g. `"/Game/Materials/M_Base"`.
- `path` (string) - Content path to create the instance in (Python tool default: `"/Game/Materials"`).

**Example:**
```json
{
  "command": "create_material_instance",
  "params": {
    "name": "MI_Base_Red",
    "parent_material": "/Game/Materials/M_Base",
    "path": "/Game/Materials"
  }
}
```

---

## set_material_parameter

Set a parameter on a **Material Instance**. The value type selects the parameter
kind:
- a **number** sets a scalar parameter,
- a **3- or 4-element array** sets a vector/color parameter,
- a **string** is treated as a texture asset path and sets a texture parameter.

**Wire parameters:**
- `material_path` (string) - Path of the material instance, e.g. `"/Game/Materials/MI_Base_Red"`.
- `parameter_name` (string) - Parameter name as defined in the parent material.
- `value` (number | array | string) - Scalar, `[r, g, b(, a)]`, or texture path.

**Example (scalar):**
```json
{
  "command": "set_material_parameter",
  "params": {
    "material_path": "/Game/Materials/MI_Base_Red",
    "parameter_name": "Roughness",
    "value": 0.4
  }
}
```

**Example (vector / color):**
```json
{
  "command": "set_material_parameter",
  "params": {
    "material_path": "/Game/Materials/MI_Base_Red",
    "parameter_name": "BaseColor",
    "value": [1.0, 0.0, 0.0, 1.0]
  }
}
```

---

## assign_material

Assign a material to a level actor's mesh components at a given slot index. The
actor is matched by internal name **or editor label**; the material is applied to
the given slot on **every** mesh component of the actor.

**Wire parameters:**
- `actor_name` (string) - Name (or label) of the level actor.
- `material_path` (string) - Path of the material / instance to assign.
- `slot_index` (integer, default: `0`) - Material slot index.

**Example:**
```json
{
  "command": "assign_material",
  "params": {
    "actor_name": "MyCube",
    "material_path": "/Game/Materials/MI_Base_Red",
    "slot_index": 0
  }
}
```

---

## set_material_color

Convenience: set a vector/color parameter on a Material Instance in one call.

**Wire parameters:**
- `material_path` (string) - Path of the material instance.
- `parameter_name` (string) - Parameter name to set (Python tool default: `"BaseColor"`).
- `color` (array) - `[r, g, b]` or `[r, g, b, a]` (0–1 range).

**Example:**
```json
{
  "command": "set_material_color",
  "params": {
    "material_path": "/Game/Materials/MI_Base_Red",
    "parameter_name": "BaseColor",
    "color": [1.0, 0.0, 0.0, 1.0]
  }
}
```

---

## get_material_info

Query basic information about a material or material instance.

> Parameter enumeration (scalar/vector/texture lists) is **not implemented** in
> the C++ handler yet — use [`execute_python`](system_tools.md#execute_python)
> for that.

**Wire parameters:**
- `material_path` (string) - Path of the material to inspect.

**Returns:**
- `name` (string), `path` (string), `class` (string)

**Example:**
```json
{
  "command": "get_material_info",
  "params": {
    "material_path": "/Game/Materials/MI_Base_Red"
  }
}
```

---

## assign_material_to_all_slots

Assign a material to **every slot of every mesh component** on a level actor.
The actor is matched by internal name or editor label.

**Wire parameters:**
- `actor_name` (string) - Name (or label) of the level actor.
- `material_path` (string) - Path of the material / instance to assign.

**Example:**
```json
{
  "command": "assign_material_to_all_slots",
  "params": {
    "actor_name": "MyCube",
    "material_path": "/Game/Materials/MI_Base_Red"
  }
}
```

---

## Error Handling

Bridge-level errors come back as a `status`/`error` pair.

```json
{
  "status": "error",
  "error": "Parent material not found: /Game/Materials/M_Base"
}
```

## Implementation Notes

- `set_material_parameter` and `set_material_color` target **Material
  Instances** (`UMaterialInstanceConstant`), not base materials. The parameter
  must be exposed (a named parameter) in the parent material.
- `assign_material` / `assign_material_to_all_slots` iterate **all** mesh
  components of the matched actor.
- Requires the `MaterialEditor` build dependency (declared in `UnrealMCP.Build.cs`).
