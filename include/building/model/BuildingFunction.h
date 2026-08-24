#pragma once

#include "building/model/BuildingFunctionCriticality.h"
#include "building/model/BuildingFunctionId.h"

#include <string>
#include <utility>

class BuildingFunction {
public:
	BuildingFunction(BuildingFunctionId function_id,
	                 std::string purpose,
	                 BuildingFunctionCriticality criticality)
		: function_id_(std::move(function_id)),
		  purpose_(std::move(purpose)),
		  criticality_(criticality) {}

	const BuildingFunctionId &functionId() const { return function_id_; }
	const std::string &purpose() const { return purpose_; }
	BuildingFunctionCriticality criticality() const { return criticality_; }

private:
	BuildingFunctionId function_id_;
	std::string purpose_;
	BuildingFunctionCriticality criticality_ = BuildingFunctionCriticality::Informational;
};
