# Clear Thought 1.5 MCP

## Overview

**Clear Thought 1.5** is a reasoning/reflection tool that helps break through confusion, re-frame problems, and think more clearly about complex scenarios.

## Tools

### `reset_session`
Reset the current reasoning session to clear state.

### `reflect_on_problem`
Reflect on a problem from multiple angles to gain clarity.

## Use Cases for vis_avs

- **Performance debugging**: Understand why audio latency is high or rendering is slow
- **Visual quality issues**: Think through rendering artifacts or visual inconsistencies
- **Real-time system coordination**: Debug audio-visual synchronization issues
- **Effect implementation problems**: Reason through complex effect algorithm issues
- **GPU programming challenges**: Evaluate shader optimization and rendering strategies

## Integration with vis_avs

vis_avs' real-time constraints make Clear Thought essential:
1. Think through audio processing latency optimization strategies
2. Reason about GPU rendering performance and visual quality trade-offs
3. Evaluate effect implementation approaches and algorithms
4. Debug complex real-time audio-visual synchronization

## Example Usage

```bash
mcp waldzellai-clear-thought reflect_on_problem '{"problem": "Why is audio latency > 20ms in the spectrum analyzer effect?", "angles": ["buffer sizes", "processing chain", "thread scheduling", "GPU synchronization"]}'
```

## When to Use

- When debugging audio latency issues
- Before implementing complex visual effects
- When optimizing GPU rendering performance
- To reason through real-time system trade-offs
- When troubleshooting audio-visual synchronization

## References

- `CLAUDE.md` Section 5 for performance invariants
- Performance benchmarking results and profiling data
- Audio processing and rendering pipeline documentation
