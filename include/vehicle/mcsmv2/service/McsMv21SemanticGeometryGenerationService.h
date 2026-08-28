#pragma once

#include "vehicle/mcsmv2/model/McsMv21SemanticGeometry.h"
#include "vehicle/mcsmv2/model/McsMv21SemanticVariantDefinition.h"
#include "vehicle/mcsmv2/model/ModernCarSemanticValidationReport.h"
#include "vehicle/mcsmv2/model/ModernCarSemanticVariant.h"
#include "vehicle/parametric/model/ParametricModelGenerationPolicy.h"

#include <cstddef>
#include <optional>

class GeneratedMcsMv21SemanticGeometryResult
{
public:
	GeneratedMcsMv21SemanticGeometryResult(
		std::optional<McsMv21SemanticGeometry> generated_geometry,
		ModernCarSemanticValidationReport validation_report)
		: generated_geometry_(std::move(generated_geometry)),
		  validation_report_(std::move(validation_report))
	{
	}

	bool succeeded() const
	{
		return generated_geometry_.has_value() && validation_report_.isValid();
	}
	const std::optional<McsMv21SemanticGeometry> &generatedGeometry() const
	{
		return generated_geometry_;
	}
	const ModernCarSemanticValidationReport &validationReport() const
	{
		return validation_report_;
	}

private:
	std::optional<McsMv21SemanticGeometry> generated_geometry_;
	ModernCarSemanticValidationReport validation_report_;
};

class McsMv21SemanticGeometryGenerationService
{
public:
	GeneratedMcsMv21SemanticGeometryResult generate(
		const ModernCarSemanticVariant &variant,
		const McsMv21SemanticVariantDefinition &semantic_definition,
		const ParametricModelGenerationPolicy &final_body_policy,
		const ParametricModelGenerationPolicy &registration_policy,
		std::size_t domain_grid_samples = 201u) const;
};
