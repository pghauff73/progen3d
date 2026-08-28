#pragma once

#include <string>
#include <utility>
#include <vector>

class PlantSemanticAnnotationProfile
{
public:
	PlantSemanticAnnotationProfile(
		std::string ontology_identifier,
		std::string ontology_release_identifier,
		std::vector<std::string> anatomical_entity_identifiers,
		std::vector<std::string> development_stage_identifiers,
		std::vector<std::string> evidence_identifiers = {})
		: ontology_identifier_(std::move(ontology_identifier)),
		  ontology_release_identifier_(
			  std::move(ontology_release_identifier)),
		  anatomical_entity_identifiers_(
			  std::move(anatomical_entity_identifiers)),
		  development_stage_identifiers_(
			  std::move(development_stage_identifiers)),
		  evidence_identifiers_(std::move(evidence_identifiers))
	{
	}

	static PlantSemanticAnnotationProfile uncalibrated()
	{
		return PlantSemanticAnnotationProfile(
			std::string(), std::string(), {}, {});
	}

	const std::string &ontologyIdentifier() const
	{
		return ontology_identifier_;
	}
	const std::string &ontologyReleaseIdentifier() const
	{
		return ontology_release_identifier_;
	}
	const std::vector<std::string> &anatomicalEntityIdentifiers() const
	{
		return anatomical_entity_identifiers_;
	}
	const std::vector<std::string> &developmentStageIdentifiers() const
	{
		return development_stage_identifiers_;
	}
	const std::vector<std::string> &evidenceIdentifiers() const
	{
		return evidence_identifiers_;
	}
	bool hasCalibratedSemanticAnnotations() const
	{
		return !ontology_identifier_.empty() &&
		       !ontology_release_identifier_.empty() &&
		       !anatomical_entity_identifiers_.empty() &&
		       !development_stage_identifiers_.empty() &&
		       !evidence_identifiers_.empty();
	}

private:
	std::string ontology_identifier_;
	std::string ontology_release_identifier_;
	std::vector<std::string> anatomical_entity_identifiers_;
	std::vector<std::string> development_stage_identifiers_;
	std::vector<std::string> evidence_identifiers_;
};
