#pragma once

#include "vehicle/mcsmv2/model/ImplicitFieldCalibration.h"
#include "vehicle/parametric/model/GeneratedVehicleRealization.h"

#include <glm/glm.hpp>

#include <utility>

class GeneratedModernCarSemanticBody
{
public:
	GeneratedModernCarSemanticBody(
		GeneratedBodyMesh body_mesh,
		ImplicitFieldCalibration field_calibration)
		: body_mesh_(std::move(body_mesh)),
		  field_calibration_(std::move(field_calibration))
	{
	}

	const GeneratedBodyMesh &bodyMesh() const { return body_mesh_; }
	const ImplicitFieldCalibration &fieldCalibration() const
	{
		return field_calibration_;
	}
	bool postMeshAffineCorrectionApplied() const { return false; }
	double maximumPostMeshCorrectionFraction() const { return 0.0; }
	const glm::dvec3 &numericalPackageScale() const
	{
		return compatibility_unit_scale_;
	}
	double maximumNumericalCorrectionFraction() const
	{
		return maximumPostMeshCorrectionFraction();
	}

private:
	GeneratedBodyMesh body_mesh_;
	ImplicitFieldCalibration field_calibration_{
		"", "", 0, glm::dvec3(1.0), glm::dvec3(0.0), 0.0, false, 0.0,
		glm::dvec3(0.0), glm::dvec3(0.0), glm::dvec3(0.0), glm::dvec3(0.0),
		0.0};
	glm::dvec3 compatibility_unit_scale_{1.0};
};
