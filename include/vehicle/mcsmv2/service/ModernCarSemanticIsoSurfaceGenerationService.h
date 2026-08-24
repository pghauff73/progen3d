#pragma once

#include "vehicle/mcsmv2/model/GeneratedModernCarSemanticBody.h"
#include "vehicle/mcsmv2/model/ModernCarSemanticValidationReport.h"
#include "vehicle/mcsmv2/model/ModernCarSemanticVariant.h"
#include "vehicle/parametric/model/ParametricModelGenerationPolicy.h"

#include <optional>

class GeneratedModernCarSemanticBodyResult
{
public:
	GeneratedModernCarSemanticBodyResult(
		std::optional<GeneratedModernCarSemanticBody> generated_body,
		ModernCarSemanticValidationReport validation_report)
		: generated_body_(std::move(generated_body)),
		  validation_report_(std::move(validation_report))
	{
	}

	bool succeeded() const
	{
		return generated_body_.has_value() && validation_report_.isValid();
	}
	const std::optional<GeneratedModernCarSemanticBody> &generatedBody() const
	{
		return generated_body_;
	}
	const ModernCarSemanticValidationReport &validationReport() const
	{
		return validation_report_;
	}

private:
	std::optional<GeneratedModernCarSemanticBody> generated_body_;
	ModernCarSemanticValidationReport validation_report_;
};

class ModernCarSemanticIsoSurfaceGenerationService
{
public:
	GeneratedModernCarSemanticBodyResult generate(
		const ModernCarSemanticVariant &variant,
		const ParametricModelGenerationPolicy &policy) const;
};
