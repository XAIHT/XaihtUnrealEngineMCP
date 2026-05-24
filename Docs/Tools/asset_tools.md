# Unreal MCP Asset Tools

This document describes the **asset management** tools in the Unreal MCP
integration: importing, duplicating, renaming, deleting and saving assets, and
creating content-browser folders.

> Backend: `FUnrealMCPAssetCommands` (`UnrealMCPAssetCommands.{h,cpp}`) ·
> Python: `Python/tools/asset_tools.py` (`register_asset_tools`)
>
> To **enumerate** existing assets, use [`list_assets`](system_tools.md#list_assets)
> from the System tools.

## Overview

| Tool | Purpose |
|------|---------|
| `import_asset` | Import an external file (FBX, texture, audio, …) |
| `duplicate_asset` | Duplicate an asset to a new path |
| `rename_asset` | Rename / move an asset |
| `delete_asset` | Delete an asset |
| `save_asset` | Save an asset to disk |
| `create_folder` | Create a content-browser folder |

---

## import_asset

Import an external file (FBX, texture, audio, etc.) into the project.

**Parameters:**
- `source_file` (string) - Absolute path to the file on disk to import.
- `destination_path` (string) - Content path to import into, e.g. `"/Game/Meshes"`.

**Returns:**
- `imported` (array) - list of created asset object paths.

**Example:**
```json
{
  "command": "import_asset",
  "params": {
    "source_file": "C:/Assets/Chair.fbx",
    "destination_path": "/Game/Meshes"
  }
}
```

---

## duplicate_asset

Duplicate an asset to a new path.

**Parameters:**
- `source` (string) - Source asset path, e.g. `"/Game/M_Base"`.
- `destination` (string) - Destination asset path, e.g. `"/Game/M_Base_Copy"`.

**Example:**
```json
{
  "command": "duplicate_asset",
  "params": {
    "source": "/Game/M_Base",
    "destination": "/Game/M_Base_Copy"
  }
}
```

---

## rename_asset

Rename / move an asset.

**Parameters:**
- `source` (string) - Current asset path.
- `destination` (string) - New asset path.

**Example:**
```json
{
  "command": "rename_asset",
  "params": {
    "source": "/Game/Old/M_Base",
    "destination": "/Game/New/M_Base"
  }
}
```

---

## delete_asset

Delete an asset.

**Parameters:**
- `path` (string) - Asset path to delete, e.g. `"/Game/Unused"`.

**Example:**
```json
{
  "command": "delete_asset",
  "params": {
    "path": "/Game/Unused"
  }
}
```

---

## save_asset

Save an asset to disk.

**Parameters:**
- `path` (string) - Asset path to save.

**Example:**
```json
{
  "command": "save_asset",
  "params": {
    "path": "/Game/Materials/M_Base"
  }
}
```

---

## create_folder

Create a content-browser folder.

**Parameters:**
- `path` (string) - Folder path to create, e.g. `"/Game/MyNewFolder"`.

**Example:**
```json
{
  "command": "create_folder",
  "params": {
    "path": "/Game/MyNewFolder"
  }
}
```

---

## Error Handling

All tools return a `success` flag (or `status`) and a `message` on failure.

```json
{
  "success": false,
  "message": "Source file not found: C:/Assets/Chair.fbx"
}
```

## Implementation Notes

- `source_file` for `import_asset` must be an **absolute path on the machine
  running the editor**, not a content path.
- Asset paths use the content-browser convention (`/Game/...`, `/Engine/...`); do
  not include the file extension.
- `rename_asset` moves the asset and updates references where possible.
