#pragma once

#include "building/model/BuildingEvidenceReference.h"
#include "building/model/BuildingRoleId.h"
#include "spatial/model/SpatialObjectId.h"

#include <utility>

class BuildingObjectRoleAssignment {
public:
	BuildingObjectRoleAssignment(SpatialObjectId object_id,
	                             BuildingRoleId role_id,
	                             BuildingEvidenceReference evidence)
		: object_id_(std::move(object_id)),
		  role_id_(std::move(role_id)),
		  evidence_(std::move(evidence)) {}

	const SpatialObjectId &objectId() const { return object_id_; }
	const BuildingRoleId &roleId() const { return role_id_; }
	const BuildingEvidenceReference &evidence() const { return evidence_; }

private:
	SpatialObjectId object_id_;
	BuildingRoleId role_id_;
	BuildingEvidenceReference evidence_;
};
