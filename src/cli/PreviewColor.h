#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace preview_color {

inline glm::vec3 decode_pixel(const std::vector<std::uint8_t> &pixels, std::size_t index)
{
	static const std::array<float, 256> table = [] {
		std::array<float, 256> values{};
		for (std::size_t value = 0; value < values.size(); ++value) {
			values[value] = std::pow(static_cast<float>(value) / 255.0f, 2.2f);
		}
		return values;
	}();
	return {table[pixels[index * 3]], table[pixels[index * 3 + 1]], table[pixels[index * 3 + 2]]};
}

inline void encode_pixel(const glm::vec3 &rgb, std::size_t index, std::vector<std::uint8_t> &pixels)
{
	for (int channel = 0; channel < 3; ++channel) {
		const float linear = std::clamp(rgb[channel], 0.0f, 1.0f);
		pixels[index * 3 + channel] =
			static_cast<std::uint8_t>(std::lround(255.0f * std::pow(linear, 1.0f / 2.2f)));
	}
}

}
