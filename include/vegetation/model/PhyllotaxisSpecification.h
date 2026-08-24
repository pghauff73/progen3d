#pragma once

#include "vegetation/model/PhyllotaxisMode.h"

class PhyllotaxisSpecification
{
public:
	PhyllotaxisSpecification(
		PhyllotaxisMode mode,
		float divergence_degrees,
		float internode_length,
		int organs_per_node = 1,
		float phase_degrees = 0.0f,
		float radial_offset = 0.0f,
		float orientation_up_bias = 0.25f)
		: mode_(mode),
		  divergence_degrees_(divergence_degrees),
		  internode_length_(internode_length),
		  organs_per_node_(organs_per_node),
		  phase_degrees_(phase_degrees),
		  radial_offset_(radial_offset),
		  orientation_up_bias_(orientation_up_bias)
	{
	}

	PhyllotaxisMode mode() const { return mode_; }
	float divergenceDegrees() const { return divergence_degrees_; }
	float internodeLength() const { return internode_length_; }
	int organsPerNode() const { return organs_per_node_; }
	float phaseDegrees() const { return phase_degrees_; }
	float radialOffset() const { return radial_offset_; }
	float orientationUpBias() const { return orientation_up_bias_; }

private:
	PhyllotaxisMode mode_ = PhyllotaxisMode::Alternate;
	float divergence_degrees_ = 137.5f;
	float internode_length_ = 0.04f;
	int organs_per_node_ = 1;
	float phase_degrees_ = 0.0f;
	float radial_offset_ = 0.0f;
	float orientation_up_bias_ = 0.25f;
};

