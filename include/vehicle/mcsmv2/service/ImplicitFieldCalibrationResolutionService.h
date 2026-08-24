#pragma once

#include "geometry/model/ImplicitScalarField.h"
#include "geometry/model/ImplicitSurfaceGenerationRequest.h"
#include "vehicle/mcsmv2/model/ImplicitFieldCalibration.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

class ImplicitFieldCalibrationResolutionResult
{
public:
	ImplicitFieldCalibrationResolutionResult(
		std::optional<ImplicitFieldCalibration> calibration,
		std::string diagnostic)
		: calibration_(std::move(calibration)),
		  diagnostic_(std::move(diagnostic))
	{
	}

	bool succeeded() const { return calibration_.has_value(); }
	const std::optional<ImplicitFieldCalibration> &calibration() const
	{
		return calibration_;
	}
	const std::string &diagnostic() const { return diagnostic_; }

private:
	std::optional<ImplicitFieldCalibration> calibration_;
	std::string diagnostic_;
};

class ImplicitFieldCalibrationResolutionService
{
public:
	ImplicitFieldCalibrationResolutionResult resolve(
		std::shared_ptr<const ImplicitScalarField> source_field,
		const ImplicitFieldCalibration &accepted_calibration,
		const ImplicitSurfaceGenerationRequest &request) const;
};
