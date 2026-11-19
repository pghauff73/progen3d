#pragma once

#include "mechanics/model/PinRollerMember.h"
#include <optional>
#include <string>

class SpatialBuildingObject;

class MemberServiceAnalysis {
public:
	explicit MemberServiceAnalysis(const SpatialBuildingObject &object);
	const std::optional<MemberServiceResponse> &response() const { return response_; }
	const std::string &diagnostic() const { return diagnostic_; }

private:
	std::optional<MemberServiceResponse> response_;
	std::string diagnostic_;
};
