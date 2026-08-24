#pragma once

#include "building/model/BuildingEvidenceReference.h"
#include "building/model/BuildingMeasuredValue.h"
#include "building/model/BuildingRequiredValue.h"
#include "building/model/BuildingRequirementEvaluationStatus.h"
#include "building/model/BuildingRequirementId.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class BuildingRequirementEvaluationRecord {
public:
	BuildingRequirementEvaluationRecord(
		BuildingRequirementId requirement_id,
		BuildingRequirementEvaluationStatus status,
		BuildingMeasuredValue measured_value,
		BuildingRequiredValue required_value,
		double tolerance,
		std::vector<BuildingEvidenceReference> evidence_references,
		std::vector<std::string> diagnostics,
		std::uint64_t evidence_hash)
		: requirement_id_(std::move(requirement_id)),
		  status_(status),
		  measured_value_(std::move(measured_value)),
		  required_value_(std::move(required_value)),
		  tolerance_(tolerance),
		  evidence_references_(std::move(evidence_references)),
		  diagnostics_(std::move(diagnostics)),
		  evidence_hash_(evidence_hash) {}

	const BuildingRequirementId &requirementId() const { return requirement_id_; }
	BuildingRequirementEvaluationStatus status() const { return status_; }
	const BuildingMeasuredValue &measuredValue() const { return measured_value_; }
	const BuildingRequiredValue &requiredValue() const { return required_value_; }
	double tolerance() const { return tolerance_; }
	const std::vector<BuildingEvidenceReference> &evidenceReferences() const
	{
		return evidence_references_;
	}
	const std::vector<std::string> &diagnostics() const { return diagnostics_; }
	std::uint64_t evidenceHash() const { return evidence_hash_; }

	BuildingRequirementEvaluationRecord withEvidenceHash(std::uint64_t evidence_hash) const
	{
		return BuildingRequirementEvaluationRecord(
			requirement_id_, status_, measured_value_, required_value_, tolerance_,
			evidence_references_, diagnostics_, evidence_hash);
	}

private:
	BuildingRequirementId requirement_id_;
	BuildingRequirementEvaluationStatus status_ = BuildingRequirementEvaluationStatus::NotEvaluated;
	BuildingMeasuredValue measured_value_{"not measured"};
	BuildingRequiredValue required_value_{"not specified"};
	double tolerance_ = 0.0;
	std::vector<BuildingEvidenceReference> evidence_references_;
	std::vector<std::string> diagnostics_;
	std::uint64_t evidence_hash_ = 0;
};
