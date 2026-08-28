#pragma once

#include <array>

class ConstraintDegreeOfFreedomState {
public:
	ConstraintDegreeOfFreedomState(
		std::array<bool, 3> free_translation_axes = {true, true, true},
		std::array<bool, 3> free_rotation_axes = {true, true, true})
		: free_translation_axes_(free_translation_axes),
		  free_rotation_axes_(free_rotation_axes) {}

	const std::array<bool, 3> &freeTranslationAxes() const
	{
		return free_translation_axes_;
	}

	const std::array<bool, 3> &freeRotationAxes() const
	{
		return free_rotation_axes_;
	}

private:
	std::array<bool, 3> free_translation_axes_;
	std::array<bool, 3> free_rotation_axes_;
};
