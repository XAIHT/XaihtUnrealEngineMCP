"""
Asset Management Tools for Unreal MCP.

Tools for importing, duplicating, renaming, deleting and saving assets.
"""

import logging
from typing import Dict, Any
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")


def register_asset_tools(mcp: FastMCP):
    """Register asset-management tools with the MCP server."""

    def _send(command: str, params: Dict[str, Any]) -> Dict[str, Any]:
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command(command, params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            logger.info(f"{command} response: {response}")
            return response
        except Exception as e:
            error_msg = f"Error in {command}: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def import_asset(
        ctx: Context,
        source_file: str,
        destination_path: str
    ) -> Dict[str, Any]:
        """
        Import an external file (FBX, texture, audio, etc.) into the project.

        Args:
            source_file: Absolute path to the file on disk to import.
            destination_path: Content path to import into, e.g. "/Game/Meshes".

        Returns:
            Dict with `imported` (list of created asset object paths).
        """
        return _send("import_asset", {
            "source_file": source_file,
            "destination_path": destination_path
        })

    @mcp.tool()
    def duplicate_asset(ctx: Context, source: str, destination: str) -> Dict[str, Any]:
        """
        Duplicate an asset to a new path.

        Args:
            source: Source asset path, e.g. "/Game/M_Base".
            destination: Destination asset path, e.g. "/Game/M_Base_Copy".
        """
        return _send("duplicate_asset", {"source": source, "destination": destination})

    @mcp.tool()
    def rename_asset(ctx: Context, source: str, destination: str) -> Dict[str, Any]:
        """
        Rename / move an asset.

        Args:
            source: Current asset path.
            destination: New asset path.
        """
        return _send("rename_asset", {"source": source, "destination": destination})

    @mcp.tool()
    def delete_asset(ctx: Context, path: str) -> Dict[str, Any]:
        """
        Delete an asset.

        Args:
            path: Asset path to delete, e.g. "/Game/Unused".
        """
        return _send("delete_asset", {"path": path})

    @mcp.tool()
    def save_asset(ctx: Context, path: str) -> Dict[str, Any]:
        """
        Save an asset to disk.

        Args:
            path: Asset path to save.
        """
        return _send("save_asset", {"path": path})

    @mcp.tool()
    def create_folder(ctx: Context, path: str) -> Dict[str, Any]:
        """
        Create a content-browser folder.

        Args:
            path: Folder path to create, e.g. "/Game/MyNewFolder".
        """
        return _send("create_folder", {"path": path})

    logger.info("Asset tools registered successfully")
