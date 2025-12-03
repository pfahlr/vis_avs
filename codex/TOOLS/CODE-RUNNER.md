# Code Runner MCP (IEX)

## Overview

**Code Runner** (IEX) allows safe execution of JavaScript or Python code snippets, useful for testing logic, validating data, or running examples.

## Tools

### `execute_code`
Execute JavaScript or Python code safely.

### `execute_code_with_variables`
Execute code with dynamic input variables (key-value pairs).

### `validate_code`
Validate code syntax and security without executing.

### `get_capabilities`
Check supported languages and execution features.

## Use Cases for vis_avs

- **Mathematical algorithm testing**: Test audio processing algorithms and transformations
- **Signal processing validation**: Validate FFT, filters, and audio effects
- **Color space conversions**: Test visual processing and color transformations
- **Performance benchmarking**: Test algorithm performance with sample data
- **Shader algorithm prototyping**: Test GPU computation algorithms before shader implementation

## Integration with vis_avs

vis_avs' mathematical complexity benefits from Code Runner:
1. Test audio processing algorithms with sample data
2. Validate signal processing transformations
3. Prototype mathematical functions before C++ implementation
4. Test color space and visual processing algorithms

## Example Usage

```bash
mcp dravidsajinraj-iex-code-runner-mcp execute_code '{"language": "python", "code": "import numpy as np; fft = np.fft.fft([1,2,3,4]); print(fft)"}'
```

## When to Use

- Testing mathematical algorithms before implementation
- Validating signal processing transformations
- Prototyping audio effect algorithms
- Testing color space and visual processing
- Quick validation of mathematical functions

## References

- Audio processing algorithm documentation
- Signal processing theory and references
- Mathematical function specifications
