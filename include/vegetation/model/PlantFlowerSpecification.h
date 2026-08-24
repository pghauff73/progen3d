#pragma once

#include "geometry/model/BotanicalBladeProfile.h"
#include "vegetation/model/FlowerHeadSpecification.h"
#include "vegetation/model/InflorescenceSpecification.h"
#include "vegetation/model/WhorlSpecification.h"

class PlantFlowerSpecification
{
public:
	PlantFlowerSpecification() = default;

	PlantFlowerSpecification(
		BotanicalBladeProfile petal_profile,
		float petal_length,
		float petal_width,
		float longitudinal_curvature,
		float camber,
		float twist_degrees,
		float thickness,
		float width_power,
		WhorlSpecification petal_whorl,
		FlowerHeadSpecification flower_head = FlowerHeadSpecification(),
		InflorescenceSpecification inflorescence =
			InflorescenceSpecification())
		: enabled_(true),
		  petal_profile_(petal_profile),
		  petal_length_(petal_length),
		  petal_width_(petal_width),
		  longitudinal_curvature_(longitudinal_curvature),
		  camber_(camber),
		  twist_degrees_(twist_degrees),
		  thickness_(thickness),
		  width_power_(width_power),
		  petal_whorl_(petal_whorl),
		  flower_head_(flower_head),
		  inflorescence_(inflorescence)
	{
	}

	bool isEnabled() const { return enabled_; }
	BotanicalBladeProfile petalProfile() const { return petal_profile_; }
	float petalLength() const { return petal_length_; }
	float petalWidth() const { return petal_width_; }
	float longitudinalCurvature() const { return longitudinal_curvature_; }
	float camber() const { return camber_; }
	float twistDegrees() const { return twist_degrees_; }
	float thickness() const { return thickness_; }
	float widthPower() const { return width_power_; }
	const WhorlSpecification &petalWhorl() const { return petal_whorl_; }
	const FlowerHeadSpecification &flowerHead() const { return flower_head_; }
	const InflorescenceSpecification &inflorescence() const
	{
		return inflorescence_;
	}

private:
	bool enabled_ = false;
	BotanicalBladeProfile petal_profile_ = BotanicalBladeProfile::Obovate;
	float petal_length_ = 0.08f;
	float petal_width_ = 0.04f;
	float longitudinal_curvature_ = 0.02f;
	float camber_ = 0.01f;
	float twist_degrees_ = 0.0f;
	float thickness_ = 0.001f;
	float width_power_ = 1.0f;
	WhorlSpecification petal_whorl_{0, 0.0f, 0.0f, 0.0f};
	FlowerHeadSpecification flower_head_;
	InflorescenceSpecification inflorescence_;
};
