"""
Level / World Tools for Unreal MCP.

Tools for loading, creating and saving levels (maps).
"""

import logging
from typing import Dict, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")


def register_level_tools(mcp: FastMCP):
    """Register level/world tools with the MCP server."""

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
    def open_level(ctx: Context, path: str) -> Dict[str, Any]:
        """
        Open (load) a level/map by content path.

        Args:
            path: Package path of the map, e.g. "/Game/Maps/MyLevel".
        """
        return _send("open_level", {"path": path})

    @mcp.tool()
    def save_current_level(ctx: Context) -> Dict[str, Any]:
        """Save the level that is currently being edited."""
        return _send("save_current_level", {})

    @mcp.tool()
    def save_all(ctx: Context) -> Dict[str, Any]:
        """Save all dirty map and content packages."""
        return _send("save_all", {})

    @mcp.tool()
    def new_level(ctx: Context, path: Optional[str] = None) -> Dict[str, Any]:
        """
        Create a new blank level.

        Args:
            path: Optional package path to save the new level to (e.g. "/Game/Maps/New").
                  If omitted, the level is created but not saved.
        """
        params = {}
        if path:
            params["path"] = path
        return _send("new_level", params)

    @mcp.tool()
    def get_current_level(ctx: Context) -> Dict[str, Any]:
        """Return the current level's name, package path and actor count."""
        return _send("get_current_level", {})

    logger.info("Level tools registered successfully")
