# Hi-AI MCP (Project Context)

## Overview

**Hi-AI** is the project-context MCP, typically populated from project-specific metadata files. It stores high-level project information, architecture decisions, and team/context details.

## Tools

Typically provides:
- `get_project_context` — Retrieve stored project metadata
- `set_project_context` — Update project information
- `get_decisions` — Retrieve architectural or design decisions
- `log_decision` — Record a new decision

## Use Cases for vis_avs

- Store the project's **core architectural decisions** (real-time audio processing, GPU rendering)
- Track **performance requirements** and latency benchmarks
- Maintain **visual quality standards** and rendering fidelity requirements
- Store **effect interface specifications** and compatibility rules
- Track **deterministic testing** procedures and golden hash standards

## Integration with vis_avs

On startup, Hi-AI should be seeded with:
1. **Project architecture**: Real-time audio visualization system with C++ core
2. **Performance requirements**: Audio latency < 10ms, rendering 60+ FPS
3. **Visual quality standards**: Color accuracy, resolution support, visual fidelity
4. **Effect interface specifications**: Standardized effect development guidelines
5. **Testing requirements**: Deterministic rendering, golden hash testing, performance benchmarks

## Example Usage

```bash
mcp hi-ai get_project_context '{"project": "vis_avs"}'
```

## When to Use

- At session start, to establish project context
- Before implementing effects, to verify interface compliance
- During code reviews, to check against performance requirements
- To understand effect interface specifications and compatibility rules

## References

- `CLAUDE.md` — full house rules and invariants
- `README.md` — architecture overview
- `docs/` — detailed documentation
- Effect interface specifications
