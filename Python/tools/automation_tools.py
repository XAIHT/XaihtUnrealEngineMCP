"""
Automation Tools for Unreal MCP.

P3 differentiators that do NOT go through a dedicated C++ handler:

  - run_macro            : execute a sequence of bridge commands as one workflow,
                           with stop-on-error and structured per-step results.
  - build_project        : headless compile of the project via UnrealEditor-Cmd / UBT.
  - run_automation_tests : headless automation test run via UnrealEditor-Cmd.

The headless tools shell out to the engine and are independent of the live editor
TCP bridge, so they work even when the editor is closed.
"""

import logging
import os
import subprocess
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")

# Optional environment overrides so paths don't have to be passed every call.
ENV_EDITOR_CMD = "UNREAL_EDITOR_CMD"   # full path to UnrealEditor-Cmd(.exe)
ENV_UPROJECT = "UNREAL_UPROJECT"       # full path to the .uproject


def register_automation_tools(mcp: FastMCP):
    """Register automation / orchestration tools with the MCP server."""

    @mcp.tool()
    def run_macro(
        ctx: Context,
        steps: List[Dict[str, Any]],
        stop_on_error: bool = True
    ) -> Dict[str, Any]:
        """
        Execute a sequence of bridge commands as a single workflow.

        Each step is a dict: {"command": <command_name>, "params": {<params>}}.
        Steps run in order against the live editor. Useful for composing a verified
        multi-step operation (e.g. create blueprint -> add component -> compile ->
        spawn) in one call.

        Args:
            steps: Ordered list of {command, params} steps.
            stop_on_error: If True (default), stop at the first failing step.

        Returns:
            Dict with `success`, `completed` (count), and `results` (per-step output).
        """
        from unreal_mcp_server import get_unreal_connection

        results: List[Dict[str, Any]] = []
        overall_success = True

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            for index, step in enumerate(steps):
                command = step.get("command")
                params = step.get("params", {}) or {}

                if not command:
                    step_result = {"step": index, "success": False, "error": "Missing 'command' in step"}
                    results.append(step_result)
                    overall_success = False
                    if stop_on_error:
                        break
                    continue

                response = unreal.send_command(command, params)
                step_failed = (
                    response is None
                    or response.get("status") == "error"
                    or response.get("success") is False
                )

                results.append({
                    "step": index,
                    "command": command,
                    "success": not step_failed,
                    "response": response,
                })

                if step_failed:
                    overall_success = False
                    if stop_on_error:
                        logger.warning(f"run_macro stopped at step {index} ('{command}') due to error")
                        break

            return {
                "success": overall_success,
                "completed": len(results),
                "total": len(steps),
                "results": results,
            }

        except Exception as e:
            error_msg = f"Error running macro: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg, "results": results}

    def _resolve(value: Optional[str], env_key: str, label: str) -> Optional[str]:
        resolved = value or os.environ.get(env_key)
        if not resolved:
            return None
        return resolved

    def _run_subprocess(args: List[str], timeout: int) -> Dict[str, Any]:
        logger.info(f"Running: {' '.join(args)}")
        try:
            proc = subprocess.run(
                args,
                capture_output=True,
                text=True,
                timeout=timeout,
            )
            # Keep the tail of the logs to avoid oversized responses.
            stdout_tail = "\n".join(proc.stdout.splitlines()[-100:]) if proc.stdout else ""
            stderr_tail = "\n".join(proc.stderr.splitlines()[-100:]) if proc.stderr else ""
            return {
                "success": proc.returncode == 0,
                "return_code": proc.returncode,
                "stdout_tail": stdout_tail,
                "stderr_tail": stderr_tail,
            }
        except subprocess.TimeoutExpired:
            return {"success": False, "message": f"Process timed out after {timeout}s"}
        except FileNotFoundError as e:
            return {"success": False, "message": f"Executable not found: {e}"}
        except Exception as e:
            return {"success": False, "message": f"Subprocess error: {e}"}

    @mcp.tool()
    def build_project(
        ctx: Context,
        editor_cmd: Optional[str] = None,
        uproject: Optional[str] = None,
        target: str = "Development",
        timeout_seconds: int = 1800
    ) -> Dict[str, Any]:
        """
        Headless-compile the project (no editor UI required).

        Paths may be passed explicitly or supplied via the UNREAL_EDITOR_CMD and
        UNREAL_UPROJECT environment variables.

        Args:
            editor_cmd: Path to UnrealEditor-Cmd(.exe). Defaults to $UNREAL_EDITOR_CMD.
            uproject: Path to the .uproject. Defaults to $UNREAL_UPROJECT.
            target: Build configuration label (informational).
            timeout_seconds: Max seconds to wait (default 1800).

        Returns:
            Dict with `success`, `return_code` and tail of stdout/stderr.
        """
        editor = _resolve(editor_cmd, ENV_EDITOR_CMD, "editor_cmd")
        project = _resolve(uproject, ENV_UPROJECT, "uproject")
        if not editor or not project:
            return {"success": False, "message":
                    "Provide editor_cmd + uproject (or set UNREAL_EDITOR_CMD / UNREAL_UPROJECT)."}

        # -run=CompileAllBlueprints triggers a compile pass; the editor-cmd also
        # recompiles changed C++/Blueprint as part of loading the project.
        args = [editor, project, "-run=CompileAllBlueprints",
                "-unattended", "-nopause", "-nullrhi", "-stdout", "-FullStdOutLogOutput"]
        result = _run_subprocess(args, timeout_seconds)
        result["target"] = target
        return result

    @mcp.tool()
    def run_automation_tests(
        ctx: Context,
        test_filter: str,
        editor_cmd: Optional[str] = None,
        uproject: Optional[str] = None,
        timeout_seconds: int = 1800
    ) -> Dict[str, Any]:
        """
        Run Unreal automation tests headlessly and return the result.

        Args:
            test_filter: Test name prefix/filter, e.g. "Project." or a full test path.
            editor_cmd: Path to UnrealEditor-Cmd(.exe). Defaults to $UNREAL_EDITOR_CMD.
            uproject: Path to the .uproject. Defaults to $UNREAL_UPROJECT.
            timeout_seconds: Max seconds to wait (default 1800).

        Returns:
            Dict with `success`, `return_code` and tail of stdout/stderr.
        """
        editor = _resolve(editor_cmd, ENV_EDITOR_CMD, "editor_cmd")
        project = _resolve(uproject, ENV_UPROJECT, "uproject")
        if not editor or not project:
            return {"success": False, "message":
                    "Provide editor_cmd + uproject (or set UNREAL_EDITOR_CMD / UNREAL_UPROJECT)."}

        exec_cmds = f"Automation RunTests {test_filter};Quit"
        args = [editor, project,
                f"-ExecCmds={exec_cmds}",
                "-unattended", "-nopause", "-nullrhi", "-stdout", "-FullStdOutLogOutput"]
        result = _run_subprocess(args, timeout_seconds)
        result["test_filter"] = test_filter
        return result

    logger.info("Automation tools registered successfully")
