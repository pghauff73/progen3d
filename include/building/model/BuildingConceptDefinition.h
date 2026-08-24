#pragma once

#include "building/model/BuildingConceptId.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

class BuildingConceptDefinition {
public:
	BuildingConceptDefinition(BuildingConceptId concept_id,
	                          std::string canonical_name,
	                          std::optional<BuildingConceptId> broader_concept_id,
	                          std::vector<std::string> accepted_aliases,
	                          bool geometry_expected,
	                          bool container_expected)
		: concept_id_(std::move(concept_id)),
		  canonical_name_(std::move(canonical_name)),
		  broader_concept_id_(std::move(broader_concept_id)),
		  accepted_aliases_(std::move(accepted_aliases)),
		  geometry_expected_(geometry_expected),
		  container_expected_(container_expected) {}

	const BuildingConceptId &conceptId() const { return concept_id_; }
	const std::string &canonicalName() const { return canonical_name_; }
	const std::optional<BuildingConceptId> &broaderConceptId() const
	{
		return broader_concept_id_;
	}
	const std::vector<std::string> &acceptedAliases() const { return accepted_aliases_; }
	bool geometryExpected() const { return geometry_expected_; }
	bool containerExpected() const { return container_expected_; }

private:
	BuildingConceptId concept_id_;
	std::string canonical_name_;
	std::optional<BuildingConceptId> broader_concept_id_;
	std::vector<std::string> accepted_aliases_;
	bool geometry_expected_ = false;
	bool container_expected_ = false;
};
