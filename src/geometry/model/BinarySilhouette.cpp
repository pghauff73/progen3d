#include "geometry/model/BinarySilhouette.h"

#include <algorithm>

std::size_t BinarySilhouette::occupiedPixelCount() const
{
	return static_cast<std::size_t>(
		std::count(pixels_.begin(), pixels_.end(), static_cast<std::uint8_t>(1u)));
}

std::uint64_t BinarySilhouette::deterministicHash() const
{
	std::uint64_t hash = 1469598103934665603ULL;
	const auto append_byte = [&hash](std::uint8_t value) {
		hash ^= value;
		hash *= 1099511628211ULL;
	};
	for (std::size_t shift = 0u; shift < sizeof(width_); ++shift) {
		append_byte(static_cast<std::uint8_t>((width_ >> (shift * 8u)) & 0xffu));
		append_byte(static_cast<std::uint8_t>((height_ >> (shift * 8u)) & 0xffu));
	}
	for (const std::uint8_t pixel : pixels_) append_byte(pixel);
	return hash;
}
