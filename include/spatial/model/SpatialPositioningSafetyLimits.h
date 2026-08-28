#pragma once

class SpatialPositioningSafetyLimits {
public:
	int maximum_broad_phase_steps = 4096;
	int maximum_refinement_iterations = 64;
	float minimum_tolerance = 0.000001f;
	float contact_slop = 0.0005f;
};
