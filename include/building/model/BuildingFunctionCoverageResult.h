#pragma once

#include "building/model/BuildingFunctionId.h"

#include <cstddef>
#include <utility>

class BuildingFunctionCoverageResult {
public:
	BuildingFunctionCoverageResult(BuildingFunctionId function_id,
	                               std::size_t allocation_count,
	                               bool covered)
		: function_id_(std::move(function_id)),
		  allocation_count_(allocation_count),
		  covered_(covered) {}

	const BuildingFunctionId &functionId() const { return function_id_; }
	std::size_t allocationCount() const { return allocation_count_; }
	bool covered() const { return covered_; }

private:
	BuildingFunctionId function_id_;
	std::size_t allocation_count_ = 0;
	bool covered_ = false;
};
