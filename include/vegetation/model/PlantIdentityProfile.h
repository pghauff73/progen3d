#pragma once

#include "vegetation/model/PlantArchitecture.h"

#include <string>
#include <utility>
#include <vector>

enum class PlantIdentityResolutionKind
{
	ArchitecturalArchetype,
	BotanicalTaxon,
	Cultivar,
	MeasuredSpecimen
};

class PlantIdentityProfile
{
public:
	PlantIdentityProfile(
		PlantIdentityResolutionKind resolution,
		PlantArchitecture architecture,
		std::string architectural_archetype,
		std::string scientific_name = std::string(),
		std::string cultivar_name = std::string(),
		std::vector<std::string> evidence_identifiers = {},
		std::string specimen_identifier = std::string())
		: resolution_(resolution),
		  architecture_(architecture),
		  architectural_archetype_(std::move(architectural_archetype)),
		  scientific_name_(std::move(scientific_name)),
		  cultivar_name_(std::move(cultivar_name)),
		  evidence_identifiers_(std::move(evidence_identifiers)),
		  specimen_identifier_(std::move(specimen_identifier))
	{
	}

	static PlantIdentityProfile architecturalArchetype(
		PlantArchitecture architecture)
	{
		return PlantIdentityProfile(
			PlantIdentityResolutionKind::ArchitecturalArchetype,
			architecture,
			plantArchitectureName(architecture));
	}

	PlantIdentityResolutionKind resolution() const { return resolution_; }
	PlantArchitecture architecture() const { return architecture_; }
	const std::string &architecturalArchetypeName() const
	{
		return architectural_archetype_;
	}
	const std::string &scientificName() const { return scientific_name_; }
	const std::string &cultivarName() const { return cultivar_name_; }
	const std::string &specimenIdentifier() const
	{
		return specimen_identifier_;
	}
	const std::vector<std::string> &evidenceIdentifiers() const
	{
		return evidence_identifiers_;
	}
	bool isTaxonomicallyCalibrated() const
	{
		const bool identity_complete =
			resolution_ != PlantIdentityResolutionKind::ArchitecturalArchetype &&
			!scientific_name_.empty() && !evidence_identifiers_.empty();
		return identity_complete &&
		       (resolution_ != PlantIdentityResolutionKind::MeasuredSpecimen ||
			!specimen_identifier_.empty());
	}

private:
	PlantIdentityResolutionKind resolution_ =
		PlantIdentityResolutionKind::ArchitecturalArchetype;
	PlantArchitecture architecture_ = PlantArchitecture::Tree;
	std::string architectural_archetype_;
	std::string scientific_name_;
	std::string cultivar_name_;
	std::vector<std::string> evidence_identifiers_;
	std::string specimen_identifier_;
};
