#pragma once

#include "spatial/model/SpatialObjectId.h"
#include "spatial/model/SpatialRelationKind.h"

#include <utility>

#include <glm/glm.hpp>

class SpatialRelationResult {
public:
	SpatialRelationResult(SpatialObjectId first_object_id,
	                      SpatialObjectId second_object_id,
	                      SpatialRelationKind relation,
	                      float distance,
	                      float penetration,
	                      glm::vec3 contact_point,
	                      glm::vec3 contact_normal,
	                      glm::vec3 closest_point_on_first,
	                      glm::vec3 closest_point_on_second,
	                      float confidence)
		: first_object_id_(std::move(first_object_id)),
		  second_object_id_(std::move(second_object_id)),
		  relation_(relation),
		  distance_(distance),
		  penetration_(penetration),
		  contact_point_(contact_point),
		  contact_normal_(contact_normal),
		  closest_point_on_first_(closest_point_on_first),
		  closest_point_on_second_(closest_point_on_second),
		  confidence_(confidence) {}

	const SpatialObjectId &firstObjectId() const { return first_object_id_; }
	const SpatialObjectId &secondObjectId() const { return second_object_id_; }
	SpatialRelationKind relation() const { return relation_; }
	float distance() const { return distance_; }
	float penetration() const { return penetration_; }
	const glm::vec3 &contactPoint() const { return contact_point_; }
	const glm::vec3 &contactNormal() const { return contact_normal_; }
	const glm::vec3 &closestPointOnFirst() const { return closest_point_on_first_; }
	const glm::vec3 &closestPointOnSecond() const { return closest_point_on_second_; }
	float confidence() const { return confidence_; }

private:
	SpatialObjectId first_object_id_;
	SpatialObjectId second_object_id_;
	SpatialRelationKind relation_ = SpatialRelationKind::Separated;
	float distance_ = 0.0f;
	float penetration_ = 0.0f;
	glm::vec3 contact_point_{0.0f};
	glm::vec3 contact_normal_{0.0f};
	glm::vec3 closest_point_on_first_{0.0f};
	glm::vec3 closest_point_on_second_{0.0f};
	float confidence_ = 0.0f;
};
