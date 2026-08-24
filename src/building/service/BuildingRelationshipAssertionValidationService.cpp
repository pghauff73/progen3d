#include "building/service/BuildingRelationshipAssertionValidationService.h"

#include "spatial/model/SpatialBuildingModel.h"

#include <set>
#include <utility>

namespace {

const SpatialConnection *find_connection(
	const SpatialBuildingModel &spatial_model,
	const SpatialConnectionId &connection_id)
{
	for (const SpatialConnection &connection :
	     spatial_model.connectionGraph().connections()) {
		if (connection.connectionId() == connection_id) return &connection;
	}
	return nullptr;
}

bool connection_endpoints_match(
	const SpatialConnection &connection,
	const SpatialObjectId &source_object_id,
	const SpatialObjectId &target_object_id)
{
	return connection.sourceInterface().objectId() == source_object_id &&
	       connection.targetInterface().objectId() == target_object_id;
}

} // namespace

BuildingModelValidationReport
BuildingRelationshipAssertionValidationService::validate(
	const BuildingRelationshipAssertionModel &assertion_model,
	const SpatialBuildingModel &spatial_model,
	const BuildingModelSafetyLimits &limits) const
{
	BuildingModelValidationReport report;
	if (assertion_model.assertions().size() > limits.maximum_relationship_assertions) {
		report.addIssue(BuildingModelValidationIssue(
			BuildingModelValidationCode::SafetyCeilingExceeded,
			"Building relationship assertion model exceeds the configured safety ceiling."));
	}

	std::set<SpatialObjectId> assertion_object_ids;
	std::set<std::pair<BuildingRelationshipAssertionKind, SpatialConnectionId>> fact_keys;
	for (const BuildingRelationshipAssertionReference &assertion :
	     assertion_model.assertions()) {
		if (!assertion_object_ids.insert(assertion.assertionObjectId()).second) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::DuplicateRelationshipAssertion,
				"Duplicate building relationship assertion object reference.",
				{assertion.assertionObjectId()}));
		}
		if (!fact_keys.emplace(
				assertion.assertionKind(), assertion.canonicalFactId()).second) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::DuplicateCanonicalRelationshipFact,
				"Multiple assertion objects reference the same canonical relationship fact.",
				{assertion.assertionObjectId()}));
		}
		if (assertion.canonicalFactId().empty() || assertion.evidence().empty() ||
		    spatial_model.objects().find(assertion.assertionObjectId()) == nullptr ||
		    spatial_model.objects().find(assertion.sourceObjectId()) == nullptr ||
		    spatial_model.objects().find(assertion.targetObjectId()) == nullptr) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::UndefinedCanonicalRelationshipFact,
				"Building relationship assertion references undefined objects, fact ID, or evidence.",
				{assertion.assertionObjectId()}));
			continue;
		}

		if (assertion.assertionKind() ==
		    BuildingRelationshipAssertionKind::Containment) {
			const SpatialObjectId *parent =
				spatial_model.containmentTree().parentOf(assertion.targetObjectId());
			if (parent == nullptr || *parent != assertion.sourceObjectId()) {
				report.addIssue(BuildingModelValidationIssue(
					BuildingModelValidationCode::CanonicalRelationshipEndpointMismatch,
					"Containment assertion endpoints do not match the canonical containment tree.",
					{assertion.assertionObjectId()}));
			}
			continue;
		}

		const SpatialConnection *connection =
			find_connection(spatial_model, assertion.canonicalFactId());
		if (connection == nullptr) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::UndefinedCanonicalRelationshipFact,
				"Building relationship assertion references an undefined spatial connection.",
				{assertion.assertionObjectId()}));
		} else if (!connection_endpoints_match(
				   *connection, assertion.sourceObjectId(), assertion.targetObjectId())) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::CanonicalRelationshipEndpointMismatch,
				"Building relationship assertion endpoints do not match the canonical spatial connection.",
				{assertion.assertionObjectId()}));
		}
	}
	return report;
}
