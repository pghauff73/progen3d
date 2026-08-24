#pragma once

#include "geometry/model/Curve3D.h"
#include "vehicle/model/VehicleFittingEvidence.h"

#include <array>
#include <string>
#include <utility>
#include <vector>

enum class SurfaceEdgeRelationshipType
{
	ContinuousC0,
	TangentG1,
	CurvatureG2,
	Crease,
	ShutLine,
	Aperture
};

class PatchNormalConstraint
{
public:
	PatchNormalConstraint(glm::vec3 direction, float weight)
		: direction_(direction), weight_(weight)
	{
	}

	const glm::vec3 &direction() const { return direction_; }
	float weight() const { return weight_; }

private:
	glm::vec3 direction_{0.0f, 1.0f, 0.0f};
	float weight_ = 0.0f;
};

class PatchProjectionConstraint
{
public:
	PatchProjectionConstraint(
		VehicleReferenceView view,
		std::string target_identifier,
		float weight,
		float residual)
		: view_(view),
		  target_identifier_(std::move(target_identifier)),
		  weight_(weight),
		  residual_(residual)
	{
	}

	VehicleReferenceView view() const { return view_; }
	const std::string &targetIdentifier() const { return target_identifier_; }
	float weight() const { return weight_; }
	float residual() const { return residual_; }

private:
	VehicleReferenceView view_ = VehicleReferenceView::Front;
	std::string target_identifier_;
	float weight_ = 0.0f;
	float residual_ = 0.0f;
};

class BoundaryPatch
{
public:
	BoundaryPatch(
		std::string identifier,
		std::array<std::string, 4> boundary_curve_identifiers,
		std::vector<std::string> internal_guide_identifiers,
		std::vector<PatchNormalConstraint> normal_constraints)
		: identifier_(std::move(identifier)),
		  boundary_curve_identifiers_(std::move(boundary_curve_identifiers)),
		  internal_guide_identifiers_(std::move(internal_guide_identifiers)),
		  normal_constraints_(std::move(normal_constraints))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::array<std::string, 4> &boundaryCurveIdentifiers() const
	{
		return boundary_curve_identifiers_;
	}
	const std::vector<std::string> &internalGuideIdentifiers() const
	{
		return internal_guide_identifiers_;
	}
	const std::vector<PatchNormalConstraint> &normalConstraints() const
	{
		return normal_constraints_;
	}

private:
	std::string identifier_;
	std::array<std::string, 4> boundary_curve_identifiers_;
	std::vector<std::string> internal_guide_identifiers_;
	std::vector<PatchNormalConstraint> normal_constraints_;
};

class ProjectionConstrainedPatch
{
public:
	ProjectionConstrainedPatch(
		BoundaryPatch initial_patch,
		std::vector<PatchProjectionConstraint> projection_constraints,
		std::vector<std::string> fixed_curve_identifiers,
		std::vector<std::string> free_landmark_identifiers,
		float continuity_weight,
		float complexity_weight)
		: initial_patch_(std::move(initial_patch)),
		  projection_constraints_(std::move(projection_constraints)),
		  fixed_curve_identifiers_(std::move(fixed_curve_identifiers)),
		  free_landmark_identifiers_(std::move(free_landmark_identifiers)),
		  continuity_weight_(continuity_weight),
		  complexity_weight_(complexity_weight)
	{
	}

	const BoundaryPatch &initialPatch() const { return initial_patch_; }
	const std::vector<PatchProjectionConstraint> &projectionConstraints() const
	{
		return projection_constraints_;
	}
	const std::vector<std::string> &fixedCurveIdentifiers() const
	{
		return fixed_curve_identifiers_;
	}
	const std::vector<std::string> &freeLandmarkIdentifiers() const
	{
		return free_landmark_identifiers_;
	}
	float continuityWeight() const { return continuity_weight_; }
	float complexityWeight() const { return complexity_weight_; }

private:
	BoundaryPatch initial_patch_;
	std::vector<PatchProjectionConstraint> projection_constraints_;
	std::vector<std::string> fixed_curve_identifiers_;
	std::vector<std::string> free_landmark_identifiers_;
	float continuity_weight_ = 0.0f;
	float complexity_weight_ = 0.0f;
};

class SurfaceEdgeRelationship
{
public:
	SurfaceEdgeRelationship(
		std::string source_patch_identifier,
		std::string source_edge_identifier,
		std::string target_patch_identifier,
		std::string target_edge_identifier,
		SurfaceEdgeRelationshipType relationship_type,
		float tolerance)
		: source_patch_identifier_(std::move(source_patch_identifier)),
		  source_edge_identifier_(std::move(source_edge_identifier)),
		  target_patch_identifier_(std::move(target_patch_identifier)),
		  target_edge_identifier_(std::move(target_edge_identifier)),
		  relationship_type_(relationship_type),
		  tolerance_(tolerance)
	{
	}

	const std::string &sourcePatchIdentifier() const { return source_patch_identifier_; }
	const std::string &sourceEdgeIdentifier() const { return source_edge_identifier_; }
	const std::string &targetPatchIdentifier() const { return target_patch_identifier_; }
	const std::string &targetEdgeIdentifier() const { return target_edge_identifier_; }
	SurfaceEdgeRelationshipType relationshipType() const { return relationship_type_; }
	float tolerance() const { return tolerance_; }

private:
	std::string source_patch_identifier_;
	std::string source_edge_identifier_;
	std::string target_patch_identifier_;
	std::string target_edge_identifier_;
	SurfaceEdgeRelationshipType relationship_type_ =
		SurfaceEdgeRelationshipType::ContinuousC0;
	float tolerance_ = 0.0f;
};

class SurfacePatchGraph
{
public:
	SurfacePatchGraph(
		std::vector<ProjectionConstrainedPatch> patches,
		std::vector<SurfaceEdgeRelationship> edge_relationships)
		: patches_(std::move(patches)),
		  edge_relationships_(std::move(edge_relationships))
	{
	}

	const std::vector<ProjectionConstrainedPatch> &patches() const { return patches_; }
	const std::vector<SurfaceEdgeRelationship> &edgeRelationships() const
	{
		return edge_relationships_;
	}

	const ProjectionConstrainedPatch *findPatch(const std::string &identifier) const
	{
		for (const ProjectionConstrainedPatch &patch : patches_) {
			if (patch.initialPatch().identifier() == identifier) return &patch;
		}
		return nullptr;
	}

private:
	std::vector<ProjectionConstrainedPatch> patches_;
	std::vector<SurfaceEdgeRelationship> edge_relationships_;
};

class PanelSeamLoop
{
public:
	PanelSeamLoop(
		std::string identifier,
		std::string host_patch_identifier,
		Curve3D closed_boundary,
		float gap_width,
		float gap_depth)
		: identifier_(std::move(identifier)),
		  host_patch_identifier_(std::move(host_patch_identifier)),
		  closed_boundary_(std::move(closed_boundary)),
		  gap_width_(gap_width),
		  gap_depth_(gap_depth)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &hostPatchIdentifier() const { return host_patch_identifier_; }
	const Curve3D &closedBoundary() const { return closed_boundary_; }
	float gapWidth() const { return gap_width_; }
	float gapDepth() const { return gap_depth_; }

private:
	std::string identifier_;
	std::string host_patch_identifier_;
	Curve3D closed_boundary_{Curve3DType::Polyline, {glm::vec3(0.0f), glm::vec3(1.0f)}};
	float gap_width_ = 0.0f;
	float gap_depth_ = 0.0f;
};

class ExtractedSurfacePanel
{
public:
	ExtractedSurfacePanel(
		std::string identifier,
		std::string source_patch_identifier,
		std::string seam_loop_identifier)
		: identifier_(std::move(identifier)),
		  source_patch_identifier_(std::move(source_patch_identifier)),
		  seam_loop_identifier_(std::move(seam_loop_identifier))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &sourcePatchIdentifier() const { return source_patch_identifier_; }
	const std::string &seamLoopIdentifier() const { return seam_loop_identifier_; }

private:
	std::string identifier_;
	std::string source_patch_identifier_;
	std::string seam_loop_identifier_;
};
