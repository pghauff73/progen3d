#pragma once

#include "geometry/model/ImplicitScalarField.h"
#include "vehicle/mcsmv2/model/ImplicitFieldCalibration.h"

#include <memory>

class InverseAffinePrewarpedField final : public ImplicitScalarField
{
public:
	InverseAffinePrewarpedField(
		std::shared_ptr<const ImplicitScalarField> source_field,
		ImplicitFieldCalibration calibration);

	double evaluateAt(const glm::dvec3 &position) const override;
	ImplicitFieldBounds evaluationBounds() const override;
	std::uint64_t deterministicHash() const override;

	const ImplicitFieldCalibration &calibration() const { return calibration_; }

private:
	std::shared_ptr<const ImplicitScalarField> source_field_;
	ImplicitFieldCalibration calibration_;
};
