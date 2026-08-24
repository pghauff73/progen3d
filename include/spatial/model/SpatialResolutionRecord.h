#pragma once

#include "spatial/model/SpatialConstraintId.h"
#include "spatial/model/SpatialObjectId.h"
#include "spatial/model/SpatialResolutionStatus.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

class SpatialResolutionRecord {
public:
	SpatialResolutionRecord(SpatialConstraintId constraint_id,
	                        SpatialObjectId source_object_id,
	                        SpatialObjectId target_object_id,
	                        glm::mat4 initial_world_transform,
	                        glm::mat4 final_world_transform,
	                        SpatialResolutionAlgorithm algorithm,
	                        int broad_phase_step_count,
	                        int refinement_iteration_count,
	                        int collision_query_count,
	                        float tolerance,
	                        glm::vec3 contact_point,
	                        glm::vec3 contact_normal,
	                        float resulting_clearance,
	                        float residual_error,
	                        SpatialResolutionStatus status,
	                        std::vector<std::string> warnings,
	                        std::uint64_t evidence_hash)
		: constraint_id_(std::move(constraint_id)),
		  source_object_id_(std::move(source_object_id)),
		  target_object_id_(std::move(target_object_id)),
		  initial_world_transform_(initial_world_transform),
		  final_world_transform_(final_world_transform),
		  algorithm_(algorithm),
		  broad_phase_step_count_(broad_phase_step_count),
		  refinement_iteration_count_(refinement_iteration_count),
		  collision_query_count_(collision_query_count),
		  tolerance_(tolerance),
		  contact_point_(contact_point),
		  contact_normal_(contact_normal),
		  resulting_clearance_(resulting_clearance),
		  residual_error_(residual_error),
		  status_(status),
		  warnings_(std::move(warnings)),
		  evidence_hash_(evidence_hash) {}

	const SpatialConstraintId &constraintId() const { return constraint_id_; }
	const SpatialObjectId &sourceObjectId() const { return source_object_id_; }
	const SpatialObjectId &targetObjectId() const { return target_object_id_; }
	const glm::mat4 &initialWorldTransform() const { return initial_world_transform_; }
	const glm::mat4 &finalWorldTransform() const { return final_world_transform_; }
	SpatialResolutionAlgorithm algorithm() const { return algorithm_; }
	int broadPhaseStepCount() const { return broad_phase_step_count_; }
	int refinementIterationCount() const { return refinement_iteration_count_; }
	int collisionQueryCount() const { return collision_query_count_; }
	float tolerance() const { return tolerance_; }
	const glm::vec3 &contactPoint() const { return contact_point_; }
	const glm::vec3 &contactNormal() const { return contact_normal_; }
	float resultingClearance() const { return resulting_clearance_; }
	float residualError() const { return residual_error_; }
	SpatialResolutionStatus status() const { return status_; }
	const std::vector<std::string> &warnings() const { return warnings_; }
	std::uint64_t evidenceHash() const { return evidence_hash_; }

	SpatialResolutionRecord withEvidenceHash(std::uint64_t evidence_hash) const
	{
		return SpatialResolutionRecord(
			constraint_id_,
			source_object_id_,
			target_object_id_,
			initial_world_transform_,
			final_world_transform_,
			algorithm_,
			broad_phase_step_count_,
			refinement_iteration_count_,
			collision_query_count_,
			tolerance_,
			contact_point_,
			contact_normal_,
			resulting_clearance_,
			residual_error_,
			status_,
			warnings_,
			evidence_hash);
	}

private:
	SpatialConstraintId constraint_id_;
	SpatialObjectId source_object_id_;
	SpatialObjectId target_object_id_;
	glm::mat4 initial_world_transform_{1.0f};
	glm::mat4 final_world_transform_{1.0f};
	SpatialResolutionAlgorithm algorithm_ = SpatialResolutionAlgorithm::ValidationOnly;
	int broad_phase_step_count_ = 0;
	int refinement_iteration_count_ = 0;
	int collision_query_count_ = 0;
	float tolerance_ = 0.0f;
	glm::vec3 contact_point_{0.0f};
	glm::vec3 contact_normal_{0.0f};
	float resulting_clearance_ = 0.0f;
	float residual_error_ = 0.0f;
	SpatialResolutionStatus status_ = SpatialResolutionStatus::Failed;
	std::vector<std::string> warnings_;
	std::uint64_t evidence_hash_ = 0;
};
