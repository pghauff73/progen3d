#pragma once

#include "vehicle/parametric/model/GeneratedVehicleRealization.h"
#include "vehicle/parametric/model/ModernCarVariantDefinition.h"
#include "vehicle/parametric/model/ParametricModelGenerationPolicy.h"
#include "vehicle/parametric/model/ParametricVehicleValidationReport.h"

#include <optional>

class GeneratedBodyMeshResult
{
public:
	GeneratedBodyMeshResult(
		std::optional<GeneratedBodyMesh> body_mesh,
		ParametricVehicleValidationReport validation_report)
		: body_mesh_(std::move(body_mesh)),
		  validation_report_(std::move(validation_report))
	{
	}

	const std::optional<GeneratedBodyMesh> &bodyMesh() const { return body_mesh_; }
	const ParametricVehicleValidationReport &validationReport() const
	{
		return validation_report_;
	}
	bool succeeded() const
	{
		return body_mesh_.has_value() && validation_report_.isValid();
	}

private:
	std::optional<GeneratedBodyMesh> body_mesh_;
	ParametricVehicleValidationReport validation_report_;
};

class ModernCarIsoSurfaceGenerationService
{
public:
	GeneratedBodyMeshResult generate(
		const ModernCarVariantDefinition &variant,
		const ParametricModelGenerationPolicy &policy) const;
};
