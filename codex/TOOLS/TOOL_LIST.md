# MCP Tools for vis_avs Project

This file documents all MCP tools configured and available for this project, with their status, priorities, and usage guidance.

## Tool Inventory

### VIBE-CHECK

- **Status**: Primary (use frequently for performance and quality)
- **Quick Guidance**:
  - Use before merging real-time performance changes
  - Verify audio processing latency patterns
  - Check GPU rendering code consistency
  - Validate effect visual quality standards
- **Key Tools**: `check_vibe`
- **When**: Code review, performance verification, visual quality checks

### SEQUENTIAL-THINKING

- **Status**: Primary (use for planning complex real-time systems)
- **Quick Guidance**:
  - Start with this for breaking down audio-visual processing
  - Use to plan effect implementation strategies
  - Document reasoning for performance optimizations
  - Plan coordination between audio, rendering, and effects
- **Key Tools**: `create_thinking_session`, `add_thinking_step`, `retrieve_session`
- **When**: Feature planning, performance optimization, system coordination

### NEO4J-MEMORY

- **Status**: Primary (use for component relationship modeling)
- **Quick Guidance**:
  - Model effect dependencies and parameter relationships
  - Store audio processing chain component relationships
  - Track rendering pipeline dependencies
  - Enable complex effect compatibility queries
- **Key Tools**: `create_node`, `add_relationship`, `query_graph`, `get_node_details`
- **When**: Effect development, dependency modeling, compatibility tracking

### HI-AI

- **Status**: Primary (use at session start for context)
- **Quick Guidance**:
  - Always load project context first (real-time visualization system)
  - Reference performance requirements (latency < 10ms, 60+ FPS)
  - Check effect interface specifications before implementing
  - Understand visual quality and rendering fidelity requirements
- **Key Tools**: `get_project_context`, `set_project_context`, `get_decisions`, `log_decision`
- **When**: Session start, effect planning, architectural decisions

### DOCFORK

- **Status**: Primary (use for documentation maintenance)
- **Quick Guidance**:
  - Update effect API documentation when interfaces change
  - Document new audio processing algorithms and optimizations
  - Maintain GPU programming and shader documentation
  - Keep performance optimization guides current
- **Key Tools**: `create_doc`, `update_doc`, `fork_doc`, `list_docs`
- **When**: Documentation updates, API changes, algorithm documentation

### CLEAR-THOUGHT

- **Status**: Primary (use for debugging and optimization)
- **Quick Guidance**:
  - Start here if debugging audio latency or rendering performance
  - Think through visual quality and rendering issues
  - Evaluate effect implementation approaches and algorithms
  - Reason through real-time system trade-offs
- **Key Tools**: `reflect_on_problem`, `reset_session`
- **When**: Debugging, optimization, architectural decisions

### CODE-RUNNER

- **Status**: Secondary (use for testing and validation)
- **Quick Guidance**:
  - Test mathematical algorithms before C++ implementation
  - Validate signal processing transformations and FFT
  - Test color space and visual processing algorithms
  - Prototype audio effect algorithms
- **Key Tools**: `execute_code`, `execute_code_with_variables`, `validate_code`, `get_capabilities`
- **When**: Algorithm testing, signal processing validation, mathematical prototyping

### NATURAL-CONTEXT

- **Status**: Secondary (use for tool discovery and coordination)
- **Quick Guidance**:
  - Use when unsure which tool handles a specific task
  - Chain operations across audio, rendering, and effect components
  - Provide natural language interface to technical operations
  - Coordinate changes across real-time systems
- **Key Tools**: `find`, `run`
- **When**: Tool discovery, cross-component coordination, complex workflows

### SPECFORGED

- **Status**: Secondary (use for formal specification)
- **Quick Guidance**:
  - Structure codex/jobs/ into coherent specifications
  - Formalize performance and latency requirements with benchmarks
  - Define audio processing and rendering requirements
  - Track cross-component implementation status
- **Key Tools**: `create_spec`, `add_requirement`, `update_design`, `list_specifications`, `start_wizard_mode`
- **When**: Feature planning, requirements formalization, cross-component tracking

## Workflow by Task Type

| Task | Primary Tools | When |
|------|---------------|------|
| Audio processing optimization | SEQUENTIAL-THINKING, CLEAR-THOUGHT, CODE-RUNNER | High latency, audio issues |
| Effect implementation | SEQUENTIAL-THINKING, NEO4J-MEMORY, VIBE-CHECK | New effects, visual quality |
| Rendering performance | CLEAR-THOUGHT, CODE-RUNNER, VIBE-CHECK | Slow rendering, GPU issues |
| Real-time system coordination | SEQUENTIAL-THINKING, NATURAL-CONTEXT, HI-AI | Cross-component changes |
| Performance benchmarking | CLEAR-THOUGHT, CODE-RUNNER, VIBE-CHECK | Performance testing |
| Documentation | DOCFORK, SEQUENTIAL-THINKING, VIBE-CHECK | API docs, algorithm docs |
| Code review | VIBE-CHECK, CLEAR-THOUGHT, CODE-RUNNER | Before merging changes |
| Debugging | CLEAR-THOUGHT, CODE-RUNNER, SEQUENTIAL-THINKING | Issues, performance problems |

## Tool Usage Guidelines

### Primary Tools (Use Frequently)
- **VIBE-CHECK**: Every code review, especially for performance and visual quality
- **SEQUENTIAL-THINKING**: Complex planning and real-time system coordination
- **NEO4J-MEMORY**: Effect dependency modeling and relationship queries
- **HI-AI**: Session initialization and architectural context
- **DOCFORK**: Documentation maintenance and effect API specs
- **CLEAR-THOUGHT**: Debugging, optimization, and decision validation

### Secondary Tools (Use as Needed)
- **CODE-RUNNER**: Mathematical algorithm testing and signal processing validation
- **NATURAL-CONTEXT**: Tool discovery and cross-component coordination
- **SPECFORGED**: Formal specification when structuring requirements

## Quick Reference

### Start a Session
```
1. HI-AI: Load project context (real-time visualization, performance requirements)
2. CLEAR-THOUGHT: Think through the task
3. Choose primary tool based on task type
```

### Optimize Audio Performance
```
1. CLEAR-THOUGHT: Analyze the latency bottleneck
2. CODE-RUNNER: Test audio processing algorithms
3. SEQUENTIAL-THINKING: Plan optimization steps
4. VIBE-CHECK: Verify real-time patterns
```

### Implement New Effect
```
1. SEQUENTIAL-THINKING: Plan effect architecture
2. NEO4J-MEMORY: Model effect dependencies
3. CODE-RUNNER: Test mathematical algorithms
4. VIBE-CHECK: Verify visual quality patterns
```

### Coordinate Real-Time System Changes
```
1. SEQUENTIAL-THINKING: Plan cross-component workflow
2. NATURAL-CONTEXT: Discover appropriate tools
3. HI-AI: Check architectural decisions
4. DOCFORK: Document coordination patterns
```

## Availability & Configuration

All tools listed here are currently available in this workspace:
- Check `./codex/TOOLS/*.md` for project-specific guidance
- Check `~/Development/agent_mcp_guides/*.md` for general reference
- Both locations contain identical tool documentation

To verify tool availability at any time:
```bash
mcp
```

To see a tool's detailed schema:
```bash
mcp <tool-name>
```

## vis_avs Specific Considerations

- **Real-time performance is critical**: Always consider impact on audio latency and rendering FPS
- **Visual quality matters**: Verify changes maintain visual fidelity and consistency
- **Effect compatibility**: Consider impact on existing presets and effect interfaces
- **GPU programming**: Different tools may be needed for C++, shader, and mathematical contexts
- **Deterministic testing**: Ensure changes don't break golden hash testing
