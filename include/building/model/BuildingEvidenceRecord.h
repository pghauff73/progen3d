#pragma once

#include "building/model/BuildingEvidenceAuthority.h"
#include "building/model/BuildingEvidenceId.h"
#include "building/model/BuildingEvidenceKind.h"

#include <cstdint>
#include <string>
#include <utility>

class BuildingEvidenceRecord {
public:
	BuildingEvidenceRecord(BuildingEvidenceId evidence_id,
	                       BuildingEvidenceKind kind,
	                       BuildingEvidenceAuthority authority,
	                       std::string source_identifier,
	                       std::string summary,
	                       std::uint64_t evidence_hash)
		: evidence_id_(std::move(evidence_id)),
		  kind_(kind),
		  authority_(authority),
		  source_identifier_(std::move(source_identifier)),
		  summary_(std::move(summary)),
		  evidence_hash_(evidence_hash) {}

	const BuildingEvidenceId &evidenceId() const { return evidence_id_; }
	BuildingEvidenceKind kind() const { return kind_; }
	BuildingEvidenceAuthority authority() const { return authority_; }
	const std::string &sourceIdentifier() const { return source_identifier_; }
	const std::string &summary() const { return summary_; }
	std::uint64_t evidenceHash() const { return evidence_hash_; }

	BuildingEvidenceRecord withEvidenceHash(std::uint64_t evidence_hash) const
	{
		return BuildingEvidenceRecord(
			evidence_id_, kind_, authority_, source_identifier_, summary_, evidence_hash);
	}

private:
	BuildingEvidenceId evidence_id_;
	BuildingEvidenceKind kind_ = BuildingEvidenceKind::GeneratedManifest;
	BuildingEvidenceAuthority authority_ = BuildingEvidenceAuthority::Pending;
	std::string source_identifier_;
	std::string summary_;
	std::uint64_t evidence_hash_ = 0;
};
