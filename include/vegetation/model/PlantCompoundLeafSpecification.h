#pragma once

#include "vegetation/model/PhyllotaxisMode.h"

class PlantCompoundLeafSpecification
{
public:
	PlantCompoundLeafSpecification() = default;

	PlantCompoundLeafSpecification(
		int leaflet_node_count,
		float rachis_length,
		float rachis_base_radius,
		float rachis_tip_radius,
		PhyllotaxisMode pattern,
		float divergence_degrees,
		float orientation_up_bias,
		float leaflet_scale)
		: enabled_(true),
		  leaflet_node_count_(leaflet_node_count),
		  rachis_length_(rachis_length),
		  rachis_base_radius_(rachis_base_radius),
		  rachis_tip_radius_(rachis_tip_radius),
		  pattern_(pattern),
		  divergence_degrees_(divergence_degrees),
		  orientation_up_bias_(orientation_up_bias),
		  leaflet_scale_(leaflet_scale)
	{
	}

	bool isEnabled() const { return enabled_; }
	int leafletNodeCount() const { return leaflet_node_count_; }
	float rachisLength() const { return rachis_length_; }
	float rachisBaseRadius() const { return rachis_base_radius_; }
	float rachisTipRadius() const { return rachis_tip_radius_; }
	PhyllotaxisMode pattern() const { return pattern_; }
	float divergenceDegrees() const { return divergence_degrees_; }
	float orientationUpBias() const { return orientation_up_bias_; }
	float leafletScale() const { return leaflet_scale_; }

private:
	bool enabled_ = false;
	int leaflet_node_count_ = 0;
	float rachis_length_ = 0.0f;
	float rachis_base_radius_ = 0.0f;
	float rachis_tip_radius_ = 0.0f;
	PhyllotaxisMode pattern_ = PhyllotaxisMode::Alternate;
	float divergence_degrees_ = 180.0f;
	float orientation_up_bias_ = 0.15f;
	float leaflet_scale_ = 1.0f;
};
