#pragma once

// Dimensionless, independently authored left/right controls. Left is +X in the
// character's local frame; +Y is up and +Z points out of the face.
struct FacialExpressionControls
{
	float smile_left = 0.0f;
	float smile_right = 0.0f;
	float jaw_open = 0.0f;
	float blink_left = 0.0f;
	float blink_right = 0.0f;
	float brow_raise_left = 0.0f;
	float brow_raise_right = 0.0f;
};
