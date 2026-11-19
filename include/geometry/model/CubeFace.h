#pragma once

#include <array>
#include <cstddef>

enum class CubeFace { NegativeX, PositiveX, NegativeY, PositiveY, NegativeZ, PositiveZ };

inline constexpr std::array<CubeFace, 6> cube_faces{
	CubeFace::NegativeX, CubeFace::PositiveX, CubeFace::NegativeY,
	CubeFace::PositiveY, CubeFace::NegativeZ, CubeFace::PositiveZ};

inline const char *cubeFaceName(CubeFace face)
{
	constexpr std::array<const char *, 6> names{"-X", "+X", "-Y", "+Y", "-Z", "+Z"};
	return names[static_cast<std::size_t>(face)];
}
