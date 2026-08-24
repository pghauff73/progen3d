#pragma once

#include "building/model/BuildingEvidenceReference.h"
#include "building/model/BuildingFunctionAllocationId.h"
#include "building/model/BuildingFunctionId.h"
#include "spatial/model/SpatialObjectId.h"

#include <utility>

class BuildingFunctionAllocationRelationship {
public:
	BuildingFunctionAllocationRelationship(
		BuildingFunctionAllocationId allocation_id,
		BuildingFunctionId function_id,
		SpatialObjectId object_id,
		BuildingEvidenceReference evidence)
		: allocation_id_(std::move(allocation_id)),
		  function_id_(std::move(function_id)),
		  object_id_(std::move(object_id)),
		  evidence_(std::move(evidence)) {}

	const BuildingFunctionAllocationId &allocationId() const { return allocation_id_; }
	const BuildingFunctionId &functionId() const { return function_id_; }
	const SpatialObjectId &objectId() const { return object_id_; }
	const BuildingEvidenceReference &evidence() const { return evidence_; }

private:
	BuildingFunctionAllocationId allocation_id_;
	BuildingFunctionId function_id_;
	SpatialObjectId object_id_;
	BuildingEvidenceReference evidence_;
};
