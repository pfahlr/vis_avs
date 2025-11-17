#pragma once

#include <avs/preset/ir.h>

#include <string>
#include <string_view>

namespace avs::preset {

/**
 * Serialize an IRPreset to JSON format with pretty printing.
 *
 * @param preset The preset to serialize
 * @param indent Number of spaces for indentation (default: 2)
 * @return JSON string representation
 */
std::string serializeToJson(const IRPreset& preset, int indent = 2);

/**
 * Deserialize a JSON string into an IRPreset.
 *
 * @param json The JSON string to parse
 * @return The parsed IRPreset
 * @throws std::runtime_error if JSON is malformed or invalid
 */
IRPreset deserializeFromJson(std::string_view json);

/**
 * Check if the given data appears to be JSON format.
 * Useful for auto-detecting preset format.
 *
 * @param data The data to check (first few bytes are sufficient)
 * @return true if data looks like JSON, false otherwise
 */
bool isJsonFormat(std::string_view data);

}  // namespace avs::preset
