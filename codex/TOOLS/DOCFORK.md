# Docfork MCP

## Overview

**Docfork** helps manage, generate, and maintain documentation. It can create, version, and organize docs in a structured way.

## Tools

See the MCP schema for exact tools, but typically:
- `create_doc` — Create a new documentation file
- `update_doc` — Update existing documentation
- `fork_doc` — Create a variant of documentation
- `list_docs` — List all managed documentation

## Use Cases for vis_avs

- **Effect API documentation**: Maintain effect interface specifications and examples
- **Audio processing docs**: Document audio pipeline algorithms and optimizations
- **Rendering documentation**: Maintain GPU programming and shader documentation
- **Performance guides**: Keep optimization and benchmarking documentation current
- **Testing documentation**: Maintain deterministic testing and golden hash procedures

## Integration with vis_avs

vis_avs' technical complexity requires comprehensive documentation:
1. Keep effect API specs in sync with interface changes
2. Document audio processing algorithms and optimizations
3. Maintain GPU programming and shader documentation
4. Track performance optimization strategies and benchmarks

## Example Usage

```bash
mcp docfork create_doc '{"path": "docs/effects/standard-interface.md", "title": "Effect Standard Interface", "content": "..."}'
```

## When to Use

- When effect interfaces change
- To document new audio processing algorithms
- When updating GPU/shader code
- To maintain performance optimization guides
- When updating testing procedures

## References

- `docs/` for existing documentation structure
- Effect interface header files for API details
- Audio processing source for algorithm documentation
