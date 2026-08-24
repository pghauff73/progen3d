#pragma once

#include "building/model/BuildingEvidenceReference.h"
#include "building/model/BuildingRelationshipAssertionKind.h"
#include "spatial/model/SpatialConnectionId.h"
#include "spatial/model/SpatialObjectId.h"

#include <utility>

class BuildingRelationshipAssertionReference {
public:
	BuildingRelationshipAssertionReference(
		SpatialObjectId assertion_object_id,
		BuildingRelationshipAssertionKind assertion_kind,
		SpatialConnectionId canonical_fact_id,
		SpatialObjectId source_object_id,
		SpatialObjectId target_object_id,
		BuildingEvidenceReference evidence)
		: assertion_object_id_(std::move(assertion_object_id)),
		  assertion_kind_(assertion_kind),
		  canonical_fact_id_(std::move(canonical_fact_id)),
		  source_object_id_(std::move(source_object_id)),
		  target_object_id_(std::move(target_object_id)),
		  evidence_(std::move(evidence)) {}

	const SpatialObjectId &assertionObjectId() const { return assertion_object_id_; }
	BuildingRelationshipAssertionKind assertionKind() const { return assertion_kind_; }
	const SpatialConnectionId &canonicalFactId() const { return canonical_fact_id_; }
	const SpatialObjectId &sourceObjectId() const { return source_object_id_; }
	const SpatialObjectId &targetObjectId() const { return target_object_id_; }
	const BuildingEvidenceReference &evidence() const { return evidence_; }

private:
	SpatialObjectId assertion_object_id_;
	BuildingRelationshipAssertionKind assertion_kind_ =
		BuildingRelationshipAssertionKind::Containment;
	SpatialConnectionId canonical_fact_id_;
	SpatialObjectId source_object_id_;
	SpatialObjectId target_object_id_;
	BuildingEvidenceReference evidence_;
};
