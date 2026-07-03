# Unreal Engine MCP Documentation

Welcome to the documentation for the Unreal Engine Model Context Protocol (MCP)
integration. This documentation will help you understand, set up, and use the MCP
system with Unreal Engine.

## Getting started

- **[Complete Setup Guide](SETUP_GUIDE.md)** — from nothing to a working setup:
  building the plugin, the Python server, configuring your MCP client, and
  troubleshooting.

## Reference

- **[Tools](Tools/README.md)** — index of all 63 tools across 10 categories, with
  per-category pages (parameters, return values, examples).

## Project analysis & roadmap

These documents track how the tool surface evolved and what is planned next:

- **[IMPROVEMENT_PROPOSAL.md](IMPROVEMENT_PROPOSAL.md)** — capability analysis vs.
  other Unreal MCP servers, plus the P0/P1/P2/P3 roadmap and what has been
  implemented.
- **[NEW_OPERATIONS_PROPOSAL.md](NEW_OPERATIONS_PROPOSAL.md)** — post-P1 comparison
  and proposed new operations.
- **[COMPARISON_V3_AND_NEXT_OPERATIONS.md](COMPARISON_V3_AND_NEXT_OPERATIONS.md)** —
  current (post P0/P1/P2/P3) comparison and the prioritized next steps.

## Architecture in one line

**AI client → Python MCP server (`unreal_mcp_server.py`, stdio) → TCP socket on
`127.0.0.1:55557` → C++ `UnrealMCP` plugin running inside the Unreal Editor.**
