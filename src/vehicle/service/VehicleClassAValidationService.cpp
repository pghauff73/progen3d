#include "vehicle/service/VehicleClassAValidationService.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_set>

namespace {

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

std::vector<glm::vec3> edge_points(
	const ClassASurfacePatch &patch,
	const std::string &edge_identifier)
{
	std::vector<glm::vec3> points;
	const std::size_t columns = patch.controlPointColumns();
	const std::size_t rows = patch.controlPointRows();
	if (columns == 0u || rows == 0u ||
	    patch.controlPoints().size() != columns * rows) {
		return points;
	}
	if (edge_identifier == "U0") {
		for (std::size_t row = 0u; row < rows; ++row) {
			points.push_back(patch.controlPoints()[row * columns]);
		}
	}
	else if (edge_identifier == "U1") {
		for (std::size_t row = 0u; row < rows; ++row) {
			points.push_back(patch.controlPoints()[row * columns + columns - 1u]);
		}
	}
	else if (edge_identifier == "V0") {
		points.insert(
			points.end(), patch.controlPoints().begin(),
			patch.controlPoints().begin() + static_cast<std::ptrdiff_t>(columns));
	}
	else if (edge_identifier == "V1") {
		points.insert(
			points.end(),
			patch.controlPoints().end() - static_cast<std::ptrdiff_t>(columns),
			patch.controlPoints().end());
	}
	return points;
}

float average_distance(
	const std::vector<glm::vec3> &first,
	const std::vector<glm::vec3> &second,
	bool reverse_second)
{
	const std::size_t count = std::min(first.size(), second.size());
	if (count == 0u) return 1.0e9f;
	float total = 0.0f;
	for (std::size_t index = 0u; index < count; ++index) {
		const std::size_t second_index = reverse_second
			? second.size() - 1u - index
			: index;
		total += glm::length(first[index] - second[second_index]);
	}
	return total / static_cast<float>(count);
}

glm::vec3 normalized_or_zero(const glm::vec3 &value)
{
	const float length = glm::length(value);
	return length > 1.0e-7f ? value / length : glm::vec3(0.0f);
}

glm::vec3 patch_normal(const ClassASurfacePatch &patch)
{
	if (patch.controlPointColumns() < 2u || patch.controlPointRows() < 2u ||
	    patch.controlPoints().size() < patch.controlPointColumns() + 1u) {
		return glm::vec3(0.0f);
	}
	const glm::vec3 u = patch.controlPoints()[1u] - patch.controlPoints()[0u];
	const glm::vec3 v = patch.controlPoints()[patch.controlPointColumns()] -
	                    patch.controlPoints()[0u];
	return normalized_or_zero(glm::cross(u, v));
}

float tangent_error(
	const std::vector<glm::vec3> &first,
	const std::vector<glm::vec3> &second,
	bool reverse_second)
{
	if (first.size() < 2u || second.size() < 2u) return 1.0f;
	const glm::vec3 first_tangent = normalized_or_zero(first.back() - first.front());
	const glm::vec3 second_tangent = reverse_second
		? normalized_or_zero(second.front() - second.back())
		: normalized_or_zero(second.back() - second.front());
	return 1.0f - std::fabs(glm::dot(first_tangent, second_tangent));
}

float curvature_magnitude(const std::vector<glm::vec3> &points)
{
	if (points.size() < 3u) return 0.0f;
	float total = 0.0f;
	for (std::size_t index = 1u; index + 1u < points.size(); ++index) {
		total += glm::length(points[index - 1u] - 2.0f * points[index] +
		                     points[index + 1u]);
	}
	return total / static_cast<float>(points.size() - 2u);
}

} // namespace

HighlightFlowReport HighlightFlowValidator::validate(
	const ClassASurfaceGraph &graph) const
{
	std::vector<HighlightFlowResidual> residuals;
	std::size_t control_point_count = 0u;
	std::size_t span_count = 0u;
	for (const ClassASurfacePatch &patch : graph.patches()) {
		control_point_count += patch.controlPoints().size();
		span_count += static_cast<std::size_t>(
			std::max(0, patch.spanCountU()) + std::max(0, patch.spanCountV()));
	}
	for (const PatchConnection &connection : graph.connections()) {
		const ClassASurfacePatch *first =
			graph.findPatch(connection.firstPatchIdentifier());
		const ClassASurfacePatch *second =
			graph.findPatch(connection.secondPatchIdentifier());
		if (first == nullptr || second == nullptr) {
			residuals.emplace_back(connection.identifier(), 1.0e9f, 1.0f, 1.0f, 1.0f);
			continue;
		}
		const std::vector<glm::vec3> first_edge =
			edge_points(*first, connection.firstEdgeIdentifier());
		const std::vector<glm::vec3> second_edge =
			edge_points(*second, connection.secondEdgeIdentifier());
		const float direct_distance = average_distance(first_edge, second_edge, false);
		const float reversed_distance = average_distance(first_edge, second_edge, true);
		const bool reverse_second = reversed_distance < direct_distance;
		const float position = std::min(direct_distance, reversed_distance);
		const float tangent = tangent_error(first_edge, second_edge, reverse_second);
		const float curvature = std::fabs(
			curvature_magnitude(first_edge) - curvature_magnitude(second_edge));
		const glm::vec3 first_normal = patch_normal(*first);
		const glm::vec3 second_normal = patch_normal(*second);
		const float normal_flow =
			1.0f - std::fabs(glm::dot(first_normal, second_normal));
		residuals.emplace_back(
			connection.identifier(), position, tangent, curvature, normal_flow);
	}
	return HighlightFlowReport(
		std::move(residuals), graph.patches().size(), control_point_count, span_count);
}

VehicleValidationReport VehicleClassAValidationService::validateGraph(
	const ClassASurfaceGraph &graph) const
{
	VehicleValidationReport report;
	std::unordered_set<std::string> curve_identifiers;
	for (const AutomotiveCharacterCurve &curve : graph.curves()) {
		if (curve.identifier().empty() || !curve_identifiers.insert(curve.identifier()).second ||
		    curve.degree() < 1 || curve.controlPoints().size() < 2u) {
			report.addIssue(VehicleValidationIssue(
				VehicleDiagnosticCode::InvalidCharacterCurveNetwork,
				"Class-A character curves require unique identifiers, positive degree, and at least two control points.",
				{curve.identifier()}));
		}
		for (const glm::vec3 &point : curve.controlPoints()) {
			if (!finite(point)) {
				report.addIssue(VehicleValidationIssue(
					VehicleDiagnosticCode::InvalidCharacterCurveNetwork,
					"Class-A character curve contains a non-finite control point.",
					{curve.identifier()}));
				break;
			}
		}
	}
	std::unordered_set<std::string> patch_identifiers;
	for (const ClassASurfacePatch &patch : graph.patches()) {
		bool boundaries_resolve = true;
		for (const std::string &boundary : patch.boundaryCurveIdentifiers()) {
			if (curve_identifiers.count(boundary) == 0u) boundaries_resolve = false;
		}
		bool guides_resolve = !patch.internalGuideIdentifiers().empty();
		for (const std::string &guide : patch.internalGuideIdentifiers()) {
			if (curve_identifiers.count(guide) == 0u) guides_resolve = false;
		}
		const bool valid_grid = patch.controlPointColumns() >= 2u &&
			patch.controlPointRows() >= 2u &&
			patch.controlPoints().size() ==
				patch.controlPointColumns() * patch.controlPointRows();
		if (patch.identifier().empty() ||
		    !patch_identifiers.insert(patch.identifier()).second ||
		    patch.degreeU() < 1 || patch.degreeV() < 1 ||
		    patch.spanCountU() < 1 || patch.spanCountV() < 1 ||
		    !valid_grid || !boundaries_resolve || !guides_resolve) {
			report.addIssue(VehicleValidationIssue(
				VehicleDiagnosticCode::InvalidSurfacePatchGraph,
				"Class-A patch requires unique identity, resolved boundaries and guide curves, positive degrees/spans, and a complete control grid.",
				{patch.identifier()}));
		}
	}
	for (const PatchConnection &connection : graph.connections()) {
		if (patch_identifiers.count(connection.firstPatchIdentifier()) == 0u ||
		    patch_identifiers.count(connection.secondPatchIdentifier()) == 0u ||
		    edge_points(*graph.findPatch(connection.firstPatchIdentifier()),
		                connection.firstEdgeIdentifier()).empty() ||
		    edge_points(*graph.findPatch(connection.secondPatchIdentifier()),
		                connection.secondEdgeIdentifier()).empty() ||
		    !std::isfinite(connection.tolerance()) || connection.tolerance() < 0.0f) {
			report.addIssue(VehicleValidationIssue(
				VehicleDiagnosticCode::InvalidSurfacePatchGraph,
				"Patch connection references an invalid patch edge or tolerance.",
				{connection.identifier()}));
		}
	}
	return report;
}

float VehicleClassAValidationService::calculateComplexityPenalty(
	const HighlightFlowReport &report,
	float patch_weight,
	float control_point_weight,
	float span_weight) const
{
	return patch_weight * static_cast<float>(report.patchCount()) +
	       control_point_weight * static_cast<float>(report.controlPointCount()) +
	       span_weight * static_cast<float>(report.spanCount());
}
