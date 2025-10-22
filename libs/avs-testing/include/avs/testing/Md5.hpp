#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace avs::offscreen {

std::string computeMd5Hex(const std::uint8_t* data, std::size_t size);

}

