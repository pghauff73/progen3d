#pragma once

#include "vehicle/mcsmv2/model/McsMv21SemanticVariantDefinition.h"
#include "vehicle/mcsmv2/model/ModernCarSemanticValidationReport.h"
#include "vehicle/mcsmv2/model/ModernCarSemanticVariant.h"
#include "vehicle/mcsmv2/model/RegisteredVehicleSemanticSurface.h"
#include "vehicle/parametric/model/ParametricModelGenerationPolicy.h"

#include <cstddef>
#include <optional>

class RegisteredVehicleSemanticSurfaceResult
{
public:
	RegisteredVehicleSemanticSurfaceResult(
		std::optional<RegisteredVehicleSemanticSurface> registered_surface,
		ModernCarSemanticValidationReport validation_report)
		: registered_surface_(std::move(registered_surface)),
		  validation_report_(std::move(validation_report))
	{
	}

	bool succeeded() const
	{
		return registered_surface_.has_value() && validation_report_.isValid();
	}
	const std::optional<RegisteredVehicleSemanticSurface> &registeredSurface() const
	{
		return registered_surface_;
	}
	const ModernCarSemanticValidationReport &validationReport() const
	{
		return validation_report_;
	}

private:
	std::optional<RegisteredVehicleSemanticSurface> registered_surface_;
	ModernCarSemanticValidationReport validation_report_;
};

class VehicleSemanticSurfaceRegistrationService
{
public:
	RegisteredVehicleSemanticSurfaceResult registerSurface(
		const ModernCarSemanticVariant &variant,
		const McsMv21SemanticVariantDefinition &semantic_definition,
		const ParametricModelGenerationPolicy &scaffold_policy,
		std::size_t longitudinal_section_count = 96u,
		std::size_t half_section_sample_count = 48u,
		std::size_t maximum_correspondence_samples = 6000u) const;
};
