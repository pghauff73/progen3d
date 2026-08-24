#pragma once

#include "building/model/BuildingEvidenceRecord.h"
#include "building/model/BuildingRequirementEvaluationRecord.h"

#include <cstdint>

class BuildingEvidenceHashService {
public:
	std::uint64_t calculate(const BuildingEvidenceRecord &record) const;
	std::uint64_t calculate(const BuildingRequirementEvaluationRecord &record) const;
	BuildingEvidenceRecord attachCalculatedHash(const BuildingEvidenceRecord &record) const;
	BuildingRequirementEvaluationRecord attachCalculatedHash(
		const BuildingRequirementEvaluationRecord &record) const;
};
