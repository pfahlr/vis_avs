# Neo4j Knowledge Graph Memory Server

## Overview

**Neo4j Memory Server** stores and queries relationships between entities in a graph database, useful for tracking dependencies, relationships, and complex data structures.

## Tools

### `create_node`
Create a new entity node in the knowledge graph.

### `add_relationship`
Define a relationship between two nodes.

### `query_graph`
Query entities and their relationships using Cypher-like syntax.

### `get_node_details`
Retrieve full information about a specific node.

## Use Cases for vis_avs

- **Effect dependency graph**: Store and query effect dependencies and parameter relationships
- **Audio processing chain**: Model audio processing component relationships
- **Rendering pipeline**: Track rendering stage dependencies and data flow
- **Preset compatibility**: Model which presets work with which effect versions

## Integration with vis_avs

vis_avs' component-based architecture can benefit from Neo4j:
1. Store effect dependencies and parameter relationships
2. Model audio processing chain component relationships
3. Track rendering pipeline dependencies
4. Enable complex queries (e.g., "find all effects that depend on spectrum analyzer")

## Example Usage

```bash
mcp sylweriusz-mcp-neo4j-memory-server create_node '{"label": "Effect", "properties": {"name": "spectrum_analyzer", "type": "audio", "version": "1.0"}}'
```

## When to Use

- During effect development, to verify dependency structures
- For complex queries involving effect relationships
- To track rendering pipeline dependencies
- When implementing preset compatibility checking

## References

- See effect interface documentation for dependency patterns
- See rendering pipeline documentation for component relationships
