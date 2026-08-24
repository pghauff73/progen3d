#pragma once

#include "geometry/model/ImplicitScalarField.h"
#include "vehicle/mcsmv2/model/ModernCarSemanticVariant.h"
#include "vehicle/mcsmv2/service/VehicleSemanticSectionFieldEvaluationService.h"

class SemanticSectionBodyField final : public ImplicitScalarField
{
public:
	explicit SemanticSectionBodyField(const ModernCarSemanticVariant &variant);

	double evaluateAt(const glm::dvec3 &position) const override;
	ImplicitFieldBounds evaluationBounds() const override;
	std::uint64_t deterministicHash() const override;

	const VehicleSemanticSectionFieldEvaluationService &sectionEvaluation() const
	{
		return section_evaluation_;
	}

private:
	std::string variant_identifier_;
	double package_width_ = 0.0;
	double package_height_ = 0.0;
	VehicleSemanticSectionFieldEvaluationService section_evaluation_;
};
