# SpecForged MCP

## Overview

**SpecForged** is a specification/requirements management tool that helps structure requirements, design, and implementation tasks using a formal approach.

## Tools

### `create_spec`
Create a new specification with requirements, design, and tasks.

### `set_current_spec`
Set the active specification for subsequent operations.

### `add_requirement`
Add user stories with EARS-formatted acceptance criteria.

### `update_design`
Update technical design documentation (architecture, components, data models, diagrams).

### `list_specifications`
List all specifications and their current status.

### `get_specification_details`
Retrieve detailed info about a specification.

### `start_wizard_mode`
Interactive wizard for comprehensive specification creation.

### `check_initialization_status`
Check if SpecForge is initialized and guide next steps.

### `classify_mode`
Classify user input to determine routing mode (chat, do, spec).

### `wizard_next_step`
Get guidance for the next step in the wizard.

## Use Cases for vis_avs

- **Effect specifications**: Structure requirements for new visual effects
- **Performance requirements**: Formalize latency and rendering benchmarks
- **Audio processing specifications**: Define audio pipeline requirements and algorithms
- **Rendering requirements**: Specify GPU rendering and visual fidelity requirements
- **Real-time system specifications**: Document end-to-end system requirements

## Integration with vis_avs

vis_avs has codex/jobs with 30+ tasks. SpecForge can:
1. Organize tasks into coherent specifications (e.g., "Audio Processing", "Rendering Engine", "Effects System")
2. Track requirements vs. implementation status
3. Link design decisions to tasks
4. Provide structured hand-off between components

## Example Usage

```bash
mcp whit3rabbit-specforged create_spec '{"name": "Real-Time Spectrum Analyzer", "description": "Audio spectrum analyzer with < 5ms latency", "spec_id": "spectrum-analyzer"}'
```

```bash
mcp whit3rabbit-specforged add_requirement '{"as_a": "user", "i_want": "see real-time frequency spectrum", "so_that": "I can visualize audio content", "spec_id": "spectrum-analyzer", "ears_requirements": [...]}'
```

## When to Use

- Planning major effect implementations
- Formalizing performance and latency requirements
- Transitioning from task list to structured development
- Tracking cross-component requirements and implementation

## References

- `codex/jobs/` for current task structure
- `CLAUDE.md` for vis_avs invariants to include in requirements
- `docs/` for architecture and performance guidelines
- Effect interface specifications
