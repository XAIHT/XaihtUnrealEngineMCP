# Unreal MCP Material Tools

This document describes the **material** tools in the Unreal MCP integration:
creating materials and material instances, setting instance parameters, and
assigning materials to level actors.

> Backend: `FUnrealMCPMaterialCommands` (`UnrealMCPMaterialCommands.{h,cpp}`) ·
> Python: `Python/tools/material_tools.py` (`register_material_tools`)

## Overview

| Tool | Purpose |
|------|---------|
| `create_material` | Create a new Material asset |
| `create_material_instance` | Create a Material Instance Constant from a parent material |
| `set_material_parameter` | Set a scalar or vector/color parameter on a Material Instance |
| `assign_material` | Assign a material to a level actor's mesh slot |

---

## create_material

Create a new Material asset.

**Parameters:**
- `name` (string) - Name of the material asset.
- `path` (string, default: `"/Game/Materials"`) - Content path to create it in.

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

**Parameters:**
- `name` (string) - Name of the material instance asset.
- `parent_material` (string) - Path of the parent material, e.g. `"/Game/Materials/M_Base"`.
- `path` (string, default: `"/Game/Materials"`) - Content path to create the instance in.

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

Set a parameter on a Material Instance.

**Parameters:**
- `material` (string) - Path of the material instance, e.g. `"/Game/Materials/MI_Base_Red"`.
- `parameter` (string) - Parameter name as defined in the parent material.
- `value` (number or array) - A number for a **scalar** parameter, or
  `[r, g, b]` / `[r, g, b, a]` for a **vector/color** parameter.

**Example (scalar):**
```json
{
  "command": "set_material_parameter",
  "params": {
    "material": "/Game/Materials/MI_Base_Red",
    "parameter": "Roughness",
    "value": 0.4
  }
}
```

**Example (vector / color):**
```json
{
  "command": "set_material_parameter",
  "params": {
    "material": "/Game/Materials/MI_Base_Red",
    "parameter": "BaseColor",
    "value": [1.0, 0.0, 0.0, 1.0]
  }
}
```

---

## assign_material

Assign a material to a level actor's mesh component slot.

**Parameters:**
- `actor` (string) - Name of the level actor.
- `material` (string) - Path of the material / instance to assign.
- `slot` (integer, default: `0`) - Material slot index.

**Example:**
```json
{
  "command": "assign_material",
  "params": {
    "actor": "MyCube",
    "material": "/Game/Materials/MI_Base_Red",
    "slot": 0
  }
}
```

---

## Error Handling

All tools return a `success` flag (or `status`) and a `message` on failure.

```json
{
  "success": false,
  "message": "Parent material '/Game/Materials/M_Base' not found"
}
```

## Implementation Notes

- `set_material_parameter` targets **Material Instances**, not base materials.
  The parameter must be exposed (a named parameter) in the parent material.
- A scalar value is a single number; a vector/color value is a 3- or 4-element list.
- `assign_material` resolves the actor's first mesh component (e.g. a
  `StaticMeshComponent`) and sets the material on the given slot index.
- Requires the `MaterialEditor` build dependency (declared in `UnrealMCP.Build.cs`).
