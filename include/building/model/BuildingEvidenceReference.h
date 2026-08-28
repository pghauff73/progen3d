#pragma once

#include "building/model/BuildingEvidenceId.h"

#include <utility>

class BuildingEvidenceReference {
public:
	BuildingEvidenceReference() = default;
	explicit BuildingEvidenceReference(BuildingEvidenceId evidence_id)
		: evidence_id_(std::move(evidence_id)) {}

	const BuildingEvidenceId &evidenceId() const { return evidence_id_; }
	bool empty() const { return evidence_id_.empty(); }

private:
	BuildingEvidenceId evidence_id_;
};
