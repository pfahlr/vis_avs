# vis_avs MCP Tools Integration Guide

This directory contains usage guides for all configured MCP servers integrated with the vis_avs project.

## Available MCP Servers

| Tool | Purpose | Key Use Cases for vis_avs |
|------|---------|---------------------------|
| [VIBE-CHECK](./VIBE-CHECK.md) | Quality & consistency assessment | Verify real-time performance, visual quality consistency |
| [SEQUENTIAL-THINKING](./SEQUENTIAL-THINKING.md) | Step-by-step reasoning | Plan audio-visual processing, effect implementation |
| [NEO4J-MEMORY](./NEO4J-MEMORY.md) | Knowledge graph database | Model effect dependencies, audio processing chains |
| [HI-AI](./HI-AI.md) | Project context storage | Store architecture decisions, performance requirements |
| [DOCFORK](./DOCFORK.md) | Documentation management | Maintain effect APIs, audio processing docs |
| [CLEAR-THOUGHT](./CLEAR-THOUGHT.md) | Reasoning & reflection | Debug latency issues, optimize rendering performance |
| [CODE-RUNNER](./CODE-RUNNER.md) | Code execution (Python/JS) | Test mathematical algorithms, signal processing |
| [NATURAL-CONTEXT](./NATURAL-CONTEXT.md) | Tool discovery & orchestration | Coordinate across audio, rendering, effect components |
| [SPECFORGED](./SPECFORGED.md) | Specification & requirements | Structure codex/jobs, track cross-component requirements |

## Quick Integration Guide

### 1. **At Project Start**
- Use `HI-AI` to seed project context (real-time visualization system, performance requirements)
- Use `SPECFORGED` to structure existing `codex/jobs/` into specifications
- Use `DOCFORK` to organize and maintain documentation

### 2. **During Feature Planning**
- Use `SEQUENTIAL-THINKING` to break down audio-visual workflows
- Use `CLEAR-THOUGHT` to think through performance implications
- Use `SPECFORGED` to formalize requirements with latency benchmarks

### 3. **During Implementation (TDD)**
- Use `CODE-RUNNER` to test mathematical algorithms and signal processing
- Use `NEO4J-MEMORY` to model and test effect dependencies
- Use `VIBE-CHECK` to verify real-time performance patterns

### 4. **During Performance Optimization**
- Use `CLEAR-THOUGHT` to analyze bottlenecks
- Use `CODE-RUNNER` to test algorithm optimizations
- Use `VIBE-CHECK` to verify real-time patterns

### 5. **During Real-Time System Coordination**
- Use `SEQUENTIAL-THINKING` to plan cross-component changes
- Use `NATURAL-CONTEXT` to discover and orchestrate tools
- Use `HI-AI` to check architectural decisions

### 6. **When Unsure**
- Use `NATURAL-CONTEXT` to discover the right tool

## vis_avs Integration Points

### Real-Time Audio Processing
- **Optimize latency**: `CLEAR-THOUGHT`, `CODE-RUNNER`, `VIBE-CHECK`
- **Test algorithms**: `CODE-RUNNER`, `SEQUENTIAL-THINKING`
- **Monitor performance**: `CLEAR-THOUGHT`, `VIBE-CHECK`

### Visual Effects & Rendering
- **Model dependencies**: `NEO4J-MEMORY`, `SEQUENTIAL-THINKING`
- **Test algorithms**: `CODE-RUNNER`, `VIBE-CHECK`
- **Verify quality**: `VIBE-CHECK`, `CLEAR-THOUGHT`

### GPU Programming & Shaders
- **Test computations**: `CODE-RUNNER`, `CLEAR-THOUGHT`
- **Optimize performance**: `CLEAR-THOUGHT`, `VIBE-CHECK`
- **Document techniques**: `DOCFORK`, `SEQUENTIAL-THINKING`

### System Architecture & Coordination
- **Coordinate changes**: `SEQUENTIAL-THINKING`, `NATURAL-CONTEXT`
- **Document interactions**: `DOCFORK`, `HI-AI`
- **Plan workflows**: `SEQUENTIAL-THINKING`, `SPECFORGED`

### Documentation & Specs
- **Effect APIs**: `DOCFORK`
- **Formalize requirements**: `SPECFORGED`
- **Track decisions**: `HI-AI`, `DOCFORK`

## References

- See `CLAUDE.md` Section 6 for MCP tool philosophy
- See individual tool docs for detailed usage
- See `README.md` for project overview
- See `AGENTS.md` for project architecture
- See `codex/jobs/` for current task structure
