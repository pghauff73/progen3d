#pragma once

#include "mechanics/model/CoaxialDriveStage.h"
#include <optional>
#include <string>

class SpatialBuildingModel;
class SpatialBuildingObject;

class CoaxialDriveAnalysis {
public:
	CoaxialDriveAnalysis(const SpatialBuildingModel &model, const SpatialBuildingObject &selected);
	const std::optional<CoaxialDriveResponse> &response() const { return response_; }
	const std::string &diagnostic() const { return diagnostic_; }

private:
	std::optional<CoaxialDriveResponse> response_;
	std::string diagnostic_;
};
