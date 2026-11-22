#pragma once

// Zero preserves the membrane-only response. Positive scale weights the
// rest-curvature shell energy derived from the existing elastic material.
struct FacialBendingSettings
{
	float stiffness_scale = 0.0f;
};
