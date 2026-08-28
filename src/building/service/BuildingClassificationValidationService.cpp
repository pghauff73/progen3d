#include "building/service/BuildingClassificationValidationService.h"

#include "spatial/model/SpatialBuildingModel.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <string>

namespace {

bool is_safe_identifier(const std::string &value)
{
	if (value.empty()) return false;
	return std::all_of(value.begin(), value.end(), [](unsigned char character) {
		return std::isalnum(character) || character == '_' || character == '-' ||
		       character == '.';
	});
}

bool concept_cycle_exists(
	const BuildingConceptId &concept_id,
	const std::map<BuildingConceptId, const BuildingConceptDefinition *> &concepts,
	std::set<BuildingConceptId> *visiting,
	std::set<BuildingConceptId> *visited)
{
	if (visited->find(concept_id) != visited->end()) return false;
	if (!visiting->insert(concept_id).second) return true;
	const auto found = concepts.find(concept_id);
	if (found != concepts.end() && found->second->broaderConceptId().has_value()) {
		if (concept_cycle_exists(
				*found->second->broaderConceptId(), concepts, visiting, visited)) {
			return true;
		}
	}
	visiting->erase(concept_id);
	visited->insert(concept_id);
	return false;
}

} // namespace

BuildingModelValidationReport BuildingClassificationValidationService::validate(
	const BuildingClassificationModel &classification_model,
	const SpatialBuildingModel &spatial_model,
	const BuildingModelSafetyLimits &limits) const
{
	BuildingModelValidationReport report;
	if (classification_model.concepts().size() > limits.maximum_concepts ||
	    classification_model.roles().size() > limits.maximum_roles ||
	    classification_model.objectProfiles().size() > limits.maximum_profiles) {
		report.addIssue(BuildingModelValidationIssue(
			BuildingModelValidationCode::SafetyCeilingExceeded,
			"Building classification model exceeds a configured safety ceiling."));
	}

	std::map<BuildingConceptId, const BuildingConceptDefinition *> concepts;
	std::set<std::string> concept_names;
	for (const BuildingConceptDefinition &concept_definition : classification_model.concepts()) {
		if (!is_safe_identifier(concept_definition.conceptId().value()) ||
		    concept_definition.canonicalName().empty()) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::InvalidIdentifier,
				"Building concept ID and canonical name must be nonempty and safe."));
		}
		if (!concepts.emplace(concept_definition.conceptId(), &concept_definition).second) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::DuplicateConceptId,
				"Duplicate building concept ID '" +
					concept_definition.conceptId().value() + "'."));
		}
		if (!concept_names.insert(concept_definition.canonicalName()).second) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::DuplicateConceptName,
				"Duplicate building concept name '" +
					concept_definition.canonicalName() + "'."));
		}
	}
	for (const BuildingConceptDefinition &concept_definition : classification_model.concepts()) {
		if (concept_definition.broaderConceptId().has_value() &&
		    concepts.find(*concept_definition.broaderConceptId()) == concepts.end()) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::UndefinedBroaderConcept,
				"Building concept '" + concept_definition.conceptId().value() +
					"' references an undefined broader concept."));
		}
	}
	std::set<BuildingConceptId> visiting;
	std::set<BuildingConceptId> visited;
	for (const auto &[concept_id, concept_definition] : concepts) {
		(void)concept_definition;
		if (concept_cycle_exists(concept_id, concepts, &visiting, &visited)) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::ConceptHierarchyCycle,
				"Building concept hierarchy contains a cycle at '" +
					concept_id.value() + "'."));
			break;
		}
	}

	std::set<BuildingRoleId> role_ids;
	std::set<std::string> role_names;
	for (const BuildingRoleDefinition &role : classification_model.roles()) {
		if (!is_safe_identifier(role.roleId().value()) || role.canonicalName().empty()) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::InvalidIdentifier,
				"Building role ID and canonical name must be nonempty and safe."));
		}
		if (!role_ids.insert(role.roleId()).second) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::DuplicateRoleId,
				"Duplicate building role ID '" + role.roleId().value() + "'."));
		}
		if (!role_names.insert(role.canonicalName()).second) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::DuplicateRoleName,
				"Duplicate building role name '" + role.canonicalName() + "'."));
		}
	}

	std::set<std::pair<SpatialObjectId, BuildingRoleId>> assignment_keys;
	for (const BuildingObjectRoleAssignment &assignment :
	     classification_model.roleAssignments()) {
		if (spatial_model.objects().find(assignment.objectId()) == nullptr) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::UndefinedProfileObject,
				"Building role assignment references an undefined spatial object.",
				{assignment.objectId()}));
		}
		if (role_ids.find(assignment.roleId()) == role_ids.end()) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::UndefinedRole,
				"Building role assignment references undefined role '" +
					assignment.roleId().value() + "'.",
				{assignment.objectId()}));
		}
		if (!assignment_keys.emplace(assignment.objectId(), assignment.roleId()).second) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::DuplicateRoleAssignment,
				"Duplicate building role assignment.", {assignment.objectId()}));
		}
	}

	std::set<SpatialObjectId> profile_object_ids;
	for (const BuildingObjectSemanticProfile &profile :
	     classification_model.objectProfiles()) {
		const SpatialBuildingObject *spatial_object =
			spatial_model.objects().find(profile.objectId());
		if (spatial_object == nullptr) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::UndefinedProfileObject,
				"Building semantic profile references an undefined spatial object.",
				{profile.objectId()}));
			continue;
		}
		if (!profile_object_ids.insert(profile.objectId()).second) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::DuplicateObjectProfile,
				"Duplicate semantic profile for object '" + profile.objectId().value() + "'.",
				{profile.objectId()}));
		}
		if (concepts.find(profile.conceptId()) == concepts.end()) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::UndefinedProfileConcept,
				"Building semantic profile references undefined concept '" +
					profile.conceptId().value() + "'.",
				{profile.objectId()}));
		}
		const auto assigned_concept = concepts.find(profile.conceptId());
		for (const BuildingRoleId &role_id : profile.roleIds()) {
			if (role_ids.find(role_id) == role_ids.end()) {
				report.addIssue(BuildingModelValidationIssue(
					BuildingModelValidationCode::UndefinedRole,
					"Building semantic profile references undefined role '" +
						role_id.value() + "'.",
					{profile.objectId()}));
			}
		}
		const bool has_geometry = !spatial_model.bindingsFor(profile.objectId()).empty();
		const bool declares_geometry =
			profile.applicability().geometry() == BuildingModelApplicability::Applicable;
		if (has_geometry != declares_geometry) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::GeometryApplicabilityMismatch,
				"Building semantic profile geometry applicability does not match spatial bindings.",
				{profile.objectId()}));
		}
		if (has_geometry && assigned_concept != concepts.end() &&
		    !assigned_concept->second->geometryExpected()) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::GeometryApplicabilityMismatch,
				"Geometry-bearing object is classified by a semantic or container-only concept.",
				{profile.objectId()}));
		}
	}
	for (const SpatialBuildingObject &object : spatial_model.objects().objects()) {
		if (profile_object_ids.find(object.identity().objectId()) == profile_object_ids.end()) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::MissingObjectProfile,
				"Spatial object '" + object.identity().objectId().value() +
					"' has no building semantic profile.",
				{object.identity().objectId()}));
		}
	}
	return report;
}
