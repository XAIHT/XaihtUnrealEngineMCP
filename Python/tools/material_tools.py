"""
Material Tools for Unreal MCP.

Tools for creating materials and material instances, setting instance parameters,
and assigning materials to level actors.
v2.0 - Added set_material_color, get_material_info, assign_material_to_all_slots
"""

import logging
from typing import Dict, List, Any, Union
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")


def register_material_tools(mcp: FastMCP):
    """Register material tools with the MCP server."""

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
    def create_material(ctx: Context, name: str, path: str = "/Game/Materials") -> Dict[str, Any]:
        """
        Create a new Material asset.

        Args:
            name: Name of the material asset.
            path: Content path to create it in.
        """
        return _send("create_material", {"name": name, "path": path})

    @mcp.tool()
    def create_material_instance(
        ctx: Context,
        name: str,
        parent_material: str,
        path: str = "/Game/Materials"
    ) -> Dict[str, Any]:
        """
        Create a Material Instance Constant from a parent material.

        Args:
            name: Name of the material instance asset.
            parent_material: Path of the parent material, e.g. "/Game/Materials/M_Base".
            path: Content path to create the instance in.
        """
        return _send("create_material_instance", {
            "name": name,
            "parent_material": parent_material,
            "path": path
        })

    @mcp.tool()
    def set_material_parameter(
        ctx: Context,
        material: str,
        parameter: str,
        value: Union[float, List[float]]
    ) -> Dict[str, Any]:
        """
        Set a parameter on a Material Instance.

        Args:
            material: Path of the material instance, e.g. "/Game/Materials/MI_Base".
            parameter: Parameter name as defined in the parent material.
            value: A number for a scalar parameter, [r, g, b] / [r, g, b, a] for a
                   vector/color parameter, or a texture asset path for a texture
                   parameter.
        """
        # C++ handler keys: material_path, parameter_name, value.
        return _send("set_material_parameter", {
            "material_path": material,
            "parameter_name": parameter,
            "value": value
        })

    @mcp.tool()
    def assign_material(
        ctx: Context,
        actor: str,
        material: str,
        slot: int = 0
    ) -> Dict[str, Any]:
        """
        Assign a material to a level actor's mesh components at a given slot.

        The actor is matched by internal name or editor label; the material is
        applied to the given slot index on every mesh component of the actor.

        Args:
            actor: Name (or label) of the level actor.
            material: Path of the material/instance to assign.
            slot: Material slot index (default 0).
        """
        # C++ handler keys: actor_name, material_path, slot_index.
        return _send("assign_material", {
            "actor_name": actor,
            "material_path": material,
            "slot_index": slot
        })

    @mcp.tool()
    def set_material_color(
        ctx: Context,
        material: str,
        color: List[float],
        parameter: str = "BaseColor"
    ) -> Dict[str, Any]:
        """
        Convenience: set a vector/color parameter in one call.

        Args:
            material: Path of the material instance.
            color: [r, g, b] or [r, g, b, a] array (0-1 range).
            parameter: Parameter name to set (default "BaseColor").
        """
        # C++ handler keys: material_path, parameter_name, color.
        return _send("set_material_color", {
            "material_path": material,
            "parameter_name": parameter,
            "color": color
        })

    @mcp.tool()
    def get_material_info(ctx: Context, material: str) -> Dict[str, Any]:
        """
        Query basic information about a material or material instance.

        Args:
            material: Path of the material to inspect.

        Returns:
            Dict with `name`, `path`, and `class`. (Parameter enumeration is not
            implemented in the C++ handler yet — use `execute_python` for that.)
        """
        # C++ handler key: material_path.
        return _send("get_material_info", {"material_path": material})

    @mcp.tool()
    def assign_material_to_all_slots(
        ctx: Context,
        actor: str,
        material: str
    ) -> Dict[str, Any]:
        """
        Assign a material to every slot of every mesh component on a level actor.

        Args:
            actor: Name (or label) of the level actor.
            material: Path of the material/instance to assign.
        """
        # C++ handler keys: actor_name, material_path.
        return _send("assign_material_to_all_slots", {
            "actor_name": actor,
            "material_path": material
        })

    logger.info("Material tools registered successfully")