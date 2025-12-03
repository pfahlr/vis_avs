# Natural Context Provider (NCP)

## Overview

**Natural Context Provider** helps discover and orchestrate tools across all connected MCP servers using natural language queries.

## Tools

### `find`
Discover tools across MCP servers using natural language description.

### `run`
Execute a tool on a specific MCP server.

## Use Cases for vis_avs

- **Tool discovery**: "Find me a tool to optimize GPU shader performance"
- **Cross-MCP orchestration**: Chain tools for complex audio-visual processing
- **Natural language routing**: Ask "how do I reduce audio latency?" and let NCP find the right tool
- **Real-time system coordination**: Coordinate between audio, rendering, and effect tools

## Integration with vis_avs

NCP acts as a **meta-orchestrator** for the MCP ecosystem in vis_avs:
1. Discover which server handles a given task (audio vs. rendering vs. effects)
2. Route complex workflows across multiple tools
3. Provide natural language interface to technical operations
4. Help coordinate changes across the real-time visualization system

## Example Usage

```bash
mcp portel-dev-ncp find '{"query": "I need to test and optimize a real-time audio processing algorithm"}'
```

```bash
mcp portel-dev-ncp run '{"server": "dravidsajinraj-iex-code-runner-mcp", "tool": "execute_code", "args": {...}}'
```

## When to Use

- When unsure which MCP tool to use for a specific task
- To chain operations across multiple components (audio, rendering, effects)
- To provide natural language interface to complex technical operations
- For coordinating changes across real-time systems

## References

- See all other codex/TOOLS/*.md files for available servers
- `CLAUDE.md` Section 6 for MCP integration strategy
- `AGENTS.md` for project architecture and agent responsibilities
