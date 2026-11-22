#pragma once

// Synthetic demonstration defaults, not a calibrated human tissue material.
// The response model is a plane-stress membrane with elastic skull support.
struct FacialElasticMaterial
{
	float young_modulus_pa = 50000.0f;
	float poisson_ratio = 0.35f;
	float thickness_m = 0.001f;
	float support_stiffness_n_per_m3 = 1000000.0f;
};
