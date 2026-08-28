#pragma once

#include "building/model/BuildingConceptDefinition.h"
#include "building/model/BuildingObjectSemanticProfile.h"
#include "building/model/BuildingRoleDefinition.h"
#include "building/relationship/BuildingObjectRoleAssignment.h"

#include <utility>
#include <vector>

class BuildingClassificationModel {
public:
	BuildingClassificationModel() = default;
	BuildingClassificationModel(
		std::vector<BuildingConceptDefinition> concepts,
		std::vector<BuildingRoleDefinition> roles,
		std::vector<BuildingObjectRoleAssignment> role_assignments,
		std::vector<BuildingObjectSemanticProfile> object_profiles)
		: concepts_(std::move(concepts)),
		  roles_(std::move(roles)),
		  role_assignments_(std::move(role_assignments)),
		  object_profiles_(std::move(object_profiles)) {}

	const std::vector<BuildingConceptDefinition> &concepts() const { return concepts_; }
	const std::vector<BuildingRoleDefinition> &roles() const { return roles_; }
	const std::vector<BuildingObjectRoleAssignment> &roleAssignments() const
	{
		return role_assignments_;
	}
	const std::vector<BuildingObjectSemanticProfile> &objectProfiles() const
	{
		return object_profiles_;
	}

	const BuildingConceptDefinition *findConcept(const BuildingConceptId &concept_id) const
	{
		for (const BuildingConceptDefinition &concept_definition : concepts_) {
			if (concept_definition.conceptId() == concept_id) return &concept_definition;
		}
		return nullptr;
	}

	const BuildingRoleDefinition *findRole(const BuildingRoleId &role_id) const
	{
		for (const BuildingRoleDefinition &role : roles_) {
			if (role.roleId() == role_id) return &role;
		}
		return nullptr;
	}

	const BuildingObjectSemanticProfile *findProfile(const SpatialObjectId &object_id) const
	{
		for (const BuildingObjectSemanticProfile &profile : object_profiles_) {
			if (profile.objectId() == object_id) return &profile;
		}
		return nullptr;
	}

private:
	std::vector<BuildingConceptDefinition> concepts_;
	std::vector<BuildingRoleDefinition> roles_;
	std::vector<BuildingObjectRoleAssignment> role_assignments_;
	std::vector<BuildingObjectSemanticProfile> object_profiles_;
};
