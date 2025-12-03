# Sequential Thinking MCP

## Overview

**Sequential Thinking** helps break down complex problems into step-by-step reasoning, maintaining a clear chain of logic.

## Tools

### `create_thinking_session`
Start a new structured thinking session for a problem.

### `add_thinking_step`
Add a reasoning step to an active session.

### `retrieve_session`
Get the full chain of reasoning from a session.

## Use Cases for vis_avs

- **Audio processing pipeline design**: Breaking down audio capture → processing → visualization flow
- **Effect implementation planning**: Planning complex visual effects step-by-step
- **Performance optimization**: Planning GPU optimization strategies
- **Real-time system coordination**: Understanding how audio, rendering, and effects interact

## Integration with vis_avs

vis_avs' real-time nature benefits from sequential thinking:
1. Plan the audio processing pipeline with latency considerations
2. Work through GPU rendering optimization strategies
3. Document reasoning for effect architecture decisions
4. Coordinate changes across audio, rendering, and effect systems

## Example Usage

```bash
mcp smithery-ai-server-sequential-thinking create_thinking_session '{"problem": "How to implement a real-time audio spectrum analyzer with < 5ms latency"}'
```

## When to Use

- Planning complex audio processing features
- Optimizing GPU rendering performance
- Implementing new visual effects
- Coordinating changes across real-time systems
