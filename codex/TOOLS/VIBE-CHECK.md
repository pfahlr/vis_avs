# Vibe Check MCP

## Overview

**Vibe Check** is an MCP server designed to assess, evaluate, and verify the "vibe" or quality/consistency of code, documentation, and system behavior.

## Tools

### `check_vibe`
Assess the overall quality and consistency of code or documentation.

**Use Cases for vis_avs:**
- Verify that real-time audio processing code follows performance patterns
- Check consistency of GPU rendering code across effects
- Validate that visual effects maintain quality standards
- Assess code quality before merging performance-sensitive changes

**When to Use:**
- Before merging changes that affect audio latency or rendering performance
- When reviewing effect implementations for visual quality
- To ensure real-time processing patterns are consistent
- During code reviews for core engine changes

## Integration with vis_avs

vis_avs relies on:
1. **Real-time performance patterns** — Vibe Check can verify low-latency code patterns
2. **Visual quality consistency** — Ensure effects maintain visual fidelity
3. **GPU programming standards** — Verify shader and rendering code quality
4. **Memory management patterns** — Ensure no leaks or resource issues

## Example Usage

```bash
mcp vibe-check check_vibe '{"subject": "libs/avs-core/src/effects/radial_blur.cpp", "criteria": "realtime_performance"}'
```

## References

- See `CLAUDE.md` Section 5 for vis_avs invariants to verify
- See `docs/` for performance and rendering guidelines
