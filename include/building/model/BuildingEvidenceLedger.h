#pragma once

#include "building/model/BuildingEvidenceRecord.h"

#include <utility>
#include <vector>

class BuildingEvidenceLedger {
public:
	BuildingEvidenceLedger() = default;
	explicit BuildingEvidenceLedger(std::vector<BuildingEvidenceRecord> records)
		: records_(std::move(records)) {}

	const std::vector<BuildingEvidenceRecord> &records() const { return records_; }

	const BuildingEvidenceRecord *find(const BuildingEvidenceId &evidence_id) const
	{
		for (const BuildingEvidenceRecord &record : records_) {
			if (record.evidenceId() == evidence_id) return &record;
		}
		return nullptr;
	}

private:
	std::vector<BuildingEvidenceRecord> records_;
};
