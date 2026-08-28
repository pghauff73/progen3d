#pragma once

#include "building/model/BuildingRequirement.h"
#include "building/model/BuildingRequirementEvaluationRecord.h"

#include <memory>
#include <utility>
#include <vector>

class BuildingRequirementModel {
public:
	BuildingRequirementModel() = default;
	BuildingRequirementModel(
		std::vector<std::shared_ptr<const BuildingRequirement>> requirements,
		std::vector<BuildingRequirementEvaluationRecord> evaluation_records)
		: requirements_(std::move(requirements)),
		  evaluation_records_(std::move(evaluation_records)) {}

	const std::vector<std::shared_ptr<const BuildingRequirement>> &requirements() const
	{
		return requirements_;
	}
	const std::vector<BuildingRequirementEvaluationRecord> &evaluationRecords() const
	{
		return evaluation_records_;
	}

	const BuildingRequirement *findRequirement(
		const BuildingRequirementId &requirement_id) const
	{
		for (const auto &requirement : requirements_) {
			if (requirement && requirement->requirementId() == requirement_id) {
				return requirement.get();
			}
		}
		return nullptr;
	}

	const BuildingRequirementEvaluationRecord *findEvaluation(
		const BuildingRequirementId &requirement_id) const
	{
		for (const BuildingRequirementEvaluationRecord &record : evaluation_records_) {
			if (record.requirementId() == requirement_id) return &record;
		}
		return nullptr;
	}

private:
	std::vector<std::shared_ptr<const BuildingRequirement>> requirements_;
	std::vector<BuildingRequirementEvaluationRecord> evaluation_records_;
};
