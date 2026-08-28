#pragma once

#include "geometry/model/Curve3D.h"
#include "vehicle/model/VehicleMvp25Evidence.h"

#include <array>
#include <string>
#include <utility>
#include <vector>

enum class PatchContinuityLevel
{
	G0,
	G1,
	G2,
	G3,
	Crease,
	PanelGap,
	Trimmed
};

class AutomotiveCharacterCurve
{
public:
	AutomotiveCharacterCurve(
		std::string identifier,
		std::string semantic_role,
		int degree,
		std::vector<glm::vec3> control_points,
		std::vector<std::string> observation_identifiers,
		std::vector<std::string> continuity_target_identifiers)
		: identifier_(std::move(identifier)),
		  semantic_role_(std::move(semantic_role)),
		  degree_(degree),
		  control_points_(std::move(control_points)),
		  observation_identifiers_(std::move(observation_identifiers)),
		  continuity_target_identifiers_(std::move(continuity_target_identifiers))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &semanticRole() const { return semantic_role_; }
	int degree() const { return degree_; }
	const std::vector<glm::vec3> &controlPoints() const { return control_points_; }
	const std::vector<std::string> &observationIdentifiers() const
	{
		return observation_identifiers_;
	}
	const std::vector<std::string> &continuityTargetIdentifiers() const
	{
		return continuity_target_identifiers_;
	}

private:
	std::string identifier_;
	std::string semantic_role_;
	int degree_ = 1;
	std::vector<glm::vec3> control_points_;
	std::vector<std::string> observation_identifiers_;
	std::vector<std::string> continuity_target_identifiers_;
};

class ClassASurfacePatch
{
public:
	ClassASurfacePatch(
		std::string identifier,
		std::array<std::string, 4> boundary_curve_identifiers,
		std::vector<std::string> internal_guide_identifiers,
		int degree_u,
		int degree_v,
		int span_count_u,
		int span_count_v,
		std::size_t control_point_columns,
		std::size_t control_point_rows,
		std::vector<glm::vec3> control_points)
		: identifier_(std::move(identifier)),
		  boundary_curve_identifiers_(std::move(boundary_curve_identifiers)),
		  internal_guide_identifiers_(std::move(internal_guide_identifiers)),
		  degree_u_(degree_u),
		  degree_v_(degree_v),
		  span_count_u_(span_count_u),
		  span_count_v_(span_count_v),
		  control_point_columns_(control_point_columns),
		  control_point_rows_(control_point_rows),
		  control_points_(std::move(control_points))
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
	int degreeU() const { return degree_u_; }
	int degreeV() const { return degree_v_; }
	int spanCountU() const { return span_count_u_; }
	int spanCountV() const { return span_count_v_; }
	std::size_t controlPointColumns() const { return control_point_columns_; }
	std::size_t controlPointRows() const { return control_point_rows_; }
	const std::vector<glm::vec3> &controlPoints() const { return control_points_; }

private:
	std::string identifier_;
	std::array<std::string, 4> boundary_curve_identifiers_;
	std::vector<std::string> internal_guide_identifiers_;
	int degree_u_ = 1;
	int degree_v_ = 1;
	int span_count_u_ = 1;
	int span_count_v_ = 1;
	std::size_t control_point_columns_ = 0u;
	std::size_t control_point_rows_ = 0u;
	std::vector<glm::vec3> control_points_;
};

class PatchConnection
{
public:
	PatchConnection(
		std::string identifier,
		std::string first_patch_identifier,
		std::string first_edge_identifier,
		std::string second_patch_identifier,
		std::string second_edge_identifier,
		PatchContinuityLevel continuity,
		float tolerance)
		: identifier_(std::move(identifier)),
		  first_patch_identifier_(std::move(first_patch_identifier)),
		  first_edge_identifier_(std::move(first_edge_identifier)),
		  second_patch_identifier_(std::move(second_patch_identifier)),
		  second_edge_identifier_(std::move(second_edge_identifier)),
		  continuity_(continuity),
		  tolerance_(tolerance)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &firstPatchIdentifier() const { return first_patch_identifier_; }
	const std::string &firstEdgeIdentifier() const { return first_edge_identifier_; }
	const std::string &secondPatchIdentifier() const { return second_patch_identifier_; }
	const std::string &secondEdgeIdentifier() const { return second_edge_identifier_; }
	PatchContinuityLevel continuity() const { return continuity_; }
	float tolerance() const { return tolerance_; }

private:
	std::string identifier_;
	std::string first_patch_identifier_;
	std::string first_edge_identifier_;
	std::string second_patch_identifier_;
	std::string second_edge_identifier_;
	PatchContinuityLevel continuity_ = PatchContinuityLevel::G0;
	float tolerance_ = 0.0f;
};

class ClassASurfaceGraph
{
public:
	ClassASurfaceGraph(
		std::vector<AutomotiveCharacterCurve> curves,
		std::vector<ClassASurfacePatch> patches,
		std::vector<PatchConnection> connections)
		: curves_(std::move(curves)),
		  patches_(std::move(patches)),
		  connections_(std::move(connections))
	{
	}

	const std::vector<AutomotiveCharacterCurve> &curves() const { return curves_; }
	const std::vector<ClassASurfacePatch> &patches() const { return patches_; }
	const std::vector<PatchConnection> &connections() const { return connections_; }
	const AutomotiveCharacterCurve *findCurve(const std::string &identifier) const
	{
		for (const AutomotiveCharacterCurve &curve : curves_) {
			if (curve.identifier() == identifier) return &curve;
		}
		return nullptr;
	}
	const ClassASurfacePatch *findPatch(const std::string &identifier) const
	{
		for (const ClassASurfacePatch &patch : patches_) {
			if (patch.identifier() == identifier) return &patch;
		}
		return nullptr;
	}

private:
	std::vector<AutomotiveCharacterCurve> curves_;
	std::vector<ClassASurfacePatch> patches_;
	std::vector<PatchConnection> connections_;
};

class HighlightFlowResidual
{
public:
	HighlightFlowResidual(
		std::string connection_identifier,
		float position_error,
		float tangent_error,
		float curvature_error,
		float normal_flow_error)
		: connection_identifier_(std::move(connection_identifier)),
		  position_error_(position_error),
		  tangent_error_(tangent_error),
		  curvature_error_(curvature_error),
		  normal_flow_error_(normal_flow_error)
	{
	}

	const std::string &connectionIdentifier() const { return connection_identifier_; }
	float positionError() const { return position_error_; }
	float tangentError() const { return tangent_error_; }
	float curvatureError() const { return curvature_error_; }
	float normalFlowError() const { return normal_flow_error_; }

private:
	std::string connection_identifier_;
	float position_error_ = 0.0f;
	float tangent_error_ = 0.0f;
	float curvature_error_ = 0.0f;
	float normal_flow_error_ = 0.0f;
};

class HighlightFlowReport
{
public:
	HighlightFlowReport(
		std::vector<HighlightFlowResidual> residuals,
		std::size_t patch_count,
		std::size_t control_point_count,
		std::size_t span_count)
		: residuals_(std::move(residuals)),
		  patch_count_(patch_count),
		  control_point_count_(control_point_count),
		  span_count_(span_count)
	{
	}

	const std::vector<HighlightFlowResidual> &residuals() const { return residuals_; }
	std::size_t patchCount() const { return patch_count_; }
	std::size_t controlPointCount() const { return control_point_count_; }
	std::size_t spanCount() const { return span_count_; }

private:
	std::vector<HighlightFlowResidual> residuals_;
	std::size_t patch_count_ = 0u;
	std::size_t control_point_count_ = 0u;
	std::size_t span_count_ = 0u;
};
