# Common MCP Usage Patterns for vis_avs

## Pattern 1: Optimize Audio Processing Latency

**Goal**: Reduce audio processing latency to < 10ms

```
1. Use CLEAR-THOUGHT: Analyze the latency bottleneck
2. Use CODE-RUNNER: Test audio processing algorithms
3. Use SEQUENTIAL-THINKING: Plan optimization steps
4. Use VIBE-CHECK: Verify real-time processing patterns
5. Use HI-AI: Check architectural decisions and constraints
```

## Pattern 2: Implement New Visual Effect

**Goal**: Create a new visual effect with proper interface compliance

```
1. Use SEQUENTIAL-THINKING: Plan effect architecture
2. Use NEO4J-MEMORY: Model effect dependencies and relationships
3. Use CODE-RUNNER: Test mathematical algorithms and transformations
4. Use VIBE-CHECK: Verify visual quality and rendering patterns
5. Use DOCFORK: Document effect interface and implementation
```

## Pattern 3: Optimize GPU Rendering Performance

**Goal**: Improve rendering FPS and GPU utilization

```
1. Use CLEAR-THOUGHT: Analyze rendering performance bottleneck
2. Use CODE-RUNNER: Test shader algorithms and optimizations
3. Use SEQUENTIAL-THINKING: Plan GPU optimization strategy
4. Use VIBE-CHECK: Verify rendering patterns and visual quality
5. Use DOCFORK: Document GPU optimization techniques
```

## Pattern 4: Debug Real-Time Audio-Visual Synchronization

**Goal**: Resolve timing issues between audio capture and visualization

```
1. Use CLEAR-THOUGHT: Reason through the synchronization problem
2. Use NATURAL-CONTEXT: Discover which component handles the issue
3. Use CODE-RUNNER: Test timing hypotheses and buffer management
4. Use SEQUENTIAL-THINKING: Walk through the audio-visual pipeline
5. Use HI-AI: Check architectural timing decisions
```

## Pattern 5: Plan and Implement System Architecture Changes

**Goal**: Modify core architecture while maintaining real-time performance

```
1. Use SEQUENTIAL-THINKING: Break down architectural changes
2. Use SPECFORGED: Formalize requirements with performance benchmarks
3. Use CLEAR-THOUGHT: Think through architectural trade-offs
4. Use CODE-RUNNER: Prototype critical components
5. Use VIBE-CHECK: Verify real-time processing patterns
6. Use DOCFORK: Document architectural decisions
```

## Pattern 6: Coordinate Cross-Component Changes

**Goal**: Implement changes affecting audio, rendering, and effects simultaneously

```
1. Use SEQUENTIAL-THINKING: Plan cross-component workflow
2. Use NATURAL-CONTEXT: Discover and orchestrate appropriate tools
3. Use HI-AI: Check architectural dependencies and constraints
4. Use NEO4J-MEMORY: Model component relationships and dependencies
5. Use DOCFORK: Document coordination patterns and interfaces
```

## Pattern 7: Validate Deterministic Rendering and Testing

**Goal**: Ensure deterministic rendering for golden hash testing

```
1. Use CLEAR-THOUGHT: Analyze deterministic rendering requirements
2. Use CODE-RUNNER: Test mathematical algorithms with seeded data
3. Use VIBE-CHECK: Verify rendering consistency patterns
4. Use SEQUENTIAL-THINKING: Plan testing strategy and validation
5. Use DOCFORK: Document deterministic testing procedures
```

## Pattern 8: Maintain Legacy Preset Compatibility

**Goal**: Ensure changes don't break existing AVS preset compatibility

```
1. Use SEQUENTIAL-THINKING: Plan compatibility preservation strategy
2. Use NEO4J-MEMORY: Model preset-effect dependencies
3. Use CODE-RUNNER: Test preset loading and execution
4. Use VIBE-CHECK: Verify compatibility patterns and visual consistency
5. Use DOCFORK: Document compatibility requirements and testing
```

## When to Use Each Tool

| Scenario | Primary Tools | Secondary Tools |
|----------|---------------|-----------------|
| Audio latency optimization | CLEAR-THOUGHT, CODE-RUNNER, VIBE-CHECK | SEQUENTIAL-THINKING |
| Effect implementation | SEQUENTIAL-THINKING, NEO4J-MEMORY, CODE-RUNNER | VIBE-CHECK |
| Rendering performance | CLEAR-THOUGHT, CODE-RUNNER, VIBE-CHECK | |
| Real-time coordination | SEQUENTIAL-THINKING, NATURAL-CONTEXT, HI-AI | |
| Feature planning | SEQUENTIAL-THINKING, SPECFORGED, CLEAR-THOUGHT | HI-AI |
| Documentation | DOCFORK, SEQUENTIAL-THINKING, VIBE-CHECK | |
| GPU programming | CODE-RUNNER, CLEAR-THOUGHT, VIBE-CHECK | |
| System architecture | SEQUENTIAL-THINKING, SPECFORGED, CLEAR-THOUGHT | HI-AI |
| Compatibility testing | SEQUENTIAL-THINKING, NEO4J-MEMORY, VIBE-CHECK | |
| Code review | VIBE-CHECK, CLEAR-THOUGHT, CODE-RUNNER | |

## Tips

1. **Always start with CLEAR-THOUGHT** for performance issues—analyze first, optimize later
2. **Use HI-AI at session start** to establish real-time system context and constraints
3. **Use NATURAL-CONTEXT** for cross-component coordination—discover the right tool for each component
4. **Use SEQUENTIAL-THINKING** for any change affecting multiple components
5. **Test with CODE-RUNNER** before implementing (mathematical algorithms, signal processing)
6. **Use VIBE-CHECK** regularly to maintain real-time performance and visual quality patterns
7. **Model with NEO4J-MEMORY** when dealing with complex effect dependencies and relationships
8. **Document with DOCFORK** when changing APIs, algorithms, or architectural patterns
9. **Structure with SPECFORGED** for major features involving multiple components

## vis_avs Specific Considerations

- **Real-time performance is critical**: Always test audio latency and rendering FPS
- **Visual quality matters**: Verify changes maintain visual fidelity and consistency
- **Effect compatibility**: Consider impact on existing presets and effect interfaces
- **GPU programming complexity**: Use appropriate tools for C++, shaders, and mathematical contexts
- **Deterministic testing essential**: Ensure changes don't break golden hash testing
- **Legacy compatibility**: Maintain compatibility with existing AVS presets
- **Cross-platform considerations**: Test on different platforms and hardware configurations
