#pragma once

#include "vehicle/model/VehicleFitObjective.h"

class VehicleFitMeasurementSet
{
public:
	VehicleFitMeasurementSet(
		float landmark,
		float silhouette,
		float character_curve,
		float package,
		float gap_closure,
		float kinematic,
		float class_a,
		float shape_prior,
		float complexity)
		: landmark_(landmark),
		  silhouette_(silhouette),
		  character_curve_(character_curve),
		  package_(package),
		  gap_closure_(gap_closure),
		  kinematic_(kinematic),
		  class_a_(class_a),
		  shape_prior_(shape_prior),
		  complexity_(complexity)
	{
	}

	float value(VehicleFitResidualTerm term) const;

private:
	float landmark_ = 0.0f;
	float silhouette_ = 0.0f;
	float character_curve_ = 0.0f;
	float package_ = 0.0f;
	float gap_closure_ = 0.0f;
	float kinematic_ = 0.0f;
	float class_a_ = 0.0f;
	float shape_prior_ = 0.0f;
	float complexity_ = 0.0f;
};

class VehicleFitObjectiveService
{
public:
	VehicleFitResidualReport calculate(
		const VehicleFitObjective &objective,
		const VehicleFitMeasurementSet &measurements) const;
};
