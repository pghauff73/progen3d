#include "vegetation/service/VegetationWoodyPointSegmentationValidationService.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr const char *segmentation_schema =
	"ProGen3D-VegetationWoodyPointSegmentation-v1";

}

VegetationWoodyPointSegmentationValidationReport
VegetationWoodyPointSegmentationValidationService::validate(
	const VegetationWoodyPointSegmentation &segmentation,
	const VegetationPointCloudDataset &dataset) const
{
	std::vector<std::string> rejection_reasons;
	if (segmentation.schemaVersion() != segmentation_schema) {
		rejection_reasons.emplace_back(
			"Woody point segmentation schema is unsupported.");
	}
	if (segmentation.segmentationIdentifier().empty() ||
	    segmentation.segmentationAlgorithmIdentifier().empty() ||
	    segmentation.rootAxisIdentifier().empty()) {
		rejection_reasons.emplace_back(
			"Woody point segmentation identity is incomplete.");
	}
	if (segmentation.datasetIdentifier() != dataset.datasetIdentifier()) {
		rejection_reasons.emplace_back(
			"Woody point segmentation dataset identity does not match.");
	}
	if (segmentation.sourcePayloadSha256() !=
	    dataset.sourceMetadata().sourcePayloadSha256()) {
		rejection_reasons.emplace_back(
			"Woody point segmentation source hash does not match.");
	}

	std::map<std::string, const VegetationWoodyAxisSegmentDefinition *> axes;
	std::size_t root_axis_count = 0u;
	for (const auto &axis : segmentation.axisDefinitions()) {
		if (axis.axisIdentifier().empty() ||
		    !std::isfinite(axis.axialStationSpacingMetres()) ||
		    axis.axialStationSpacingMetres() <= 0.0) {
			rejection_reasons.emplace_back(
				"Woody axis segment definition is invalid.");
		}
		if (!axes.emplace(axis.axisIdentifier(), &axis).second) {
			rejection_reasons.emplace_back(
				"Woody axis segment identifier appears more than once.");
		}
		if (axis.isRootAxis()) {
			++root_axis_count;
			if (axis.branchOrder() != 0u ||
			    axis.axisIdentifier() != segmentation.rootAxisIdentifier()) {
				rejection_reasons.emplace_back(
					"Woody root axis identity or branch order is invalid.");
			}
		}
	}
	if (axes.empty() || root_axis_count != 1u) {
		rejection_reasons.emplace_back(
			"Woody point segmentation requires exactly one root axis.");
	}
	for (const auto &entry : axes) {
		const auto &axis = *entry.second;
		if (axis.isRootAxis()) continue;
		const auto parent = axes.find(axis.parentAxisIdentifier());
		if (parent == axes.end()) {
			rejection_reasons.emplace_back(
				"Woody axis parent identifier does not exist.");
			continue;
		}
		if (axis.branchOrder() != parent->second->branchOrder() + 1u) {
			rejection_reasons.emplace_back(
				"Woody axis branch order must increment its parent by one.");
		}
		std::set<std::string> visited;
		const VegetationWoodyAxisSegmentDefinition *current = &axis;
		while (current != nullptr && !current->isRootAxis()) {
			if (!visited.insert(current->axisIdentifier()).second) {
				rejection_reasons.emplace_back(
					"Woody axis hierarchy contains a parent cycle.");
				break;
			}
			const auto current_parent = axes.find(
				current->parentAxisIdentifier());
			current = current_parent == axes.end() ? nullptr
			                                      : current_parent->second;
		}
	}

	std::map<std::size_t, const VegetationPointCloudPointRecord *> points;
	std::size_t woody_point_count = 0u;
	for (const auto &point : dataset.points()) {
		points.emplace(point.sourceIndex(), &point);
		if (point.organClass() == VegetationPointCloudOrganClass::Woody) {
			++woody_point_count;
		}
	}
	std::set<std::size_t> assigned_indices;
	std::map<std::string, std::size_t> assignments_per_axis;
	double minimum_confidence = std::numeric_limits<double>::infinity();
	for (const auto &assignment : segmentation.pointAssignments()) {
		if (!std::isfinite(assignment.assignmentConfidence()) ||
		    assignment.assignmentConfidence() < 0.0 ||
		    assignment.assignmentConfidence() > 1.0) {
			rejection_reasons.emplace_back(
				"Woody point segment assignment confidence is invalid.");
		}
		const auto point = points.find(assignment.sourcePointIndex());
		if (point == points.end()) {
			rejection_reasons.emplace_back(
				"Woody point segment assignment references an unknown point.");
			continue;
		}
		if (point->second->organClass() != VegetationPointCloudOrganClass::Woody) {
			rejection_reasons.emplace_back(
				"Woody point segmentation assigns a non-woody point.");
		}
		if (axes.count(assignment.axisIdentifier()) == 0u) {
			rejection_reasons.emplace_back(
				"Woody point segment assignment references an unknown axis.");
		}
		if (!assigned_indices.insert(assignment.sourcePointIndex()).second) {
			rejection_reasons.emplace_back(
				"Woody point appears in more than one segment assignment.");
		}
		++assignments_per_axis[assignment.axisIdentifier()];
		minimum_confidence = std::min(
			minimum_confidence, assignment.assignmentConfidence());
	}
	for (const auto &entry : axes) {
		if (assignments_per_axis[entry.first] == 0u) {
			rejection_reasons.emplace_back(
				"Woody axis segment has no assigned source points.");
		}
	}
	const std::size_t assigned_woody_point_count = std::count_if(
		assigned_indices.begin(), assigned_indices.end(),
		[&](std::size_t source_index) {
			const auto point = points.find(source_index);
			return point != points.end() &&
			       point->second->organClass() ==
				       VegetationPointCloudOrganClass::Woody;
		});
	const std::size_t unassigned_woody_point_count =
		woody_point_count >= assigned_woody_point_count
			? woody_point_count - assigned_woody_point_count
			: 0u;
	if (segmentation.completeForObservedWoodyPoints() &&
	    unassigned_woody_point_count != 0u) {
		rejection_reasons.emplace_back(
			"Complete woody segmentation leaves observed woody points unassigned.");
	}
	if (!std::isfinite(minimum_confidence)) minimum_confidence = 0.0;
	return VegetationWoodyPointSegmentationValidationReport(
		woody_point_count, assigned_woody_point_count,
		unassigned_woody_point_count, minimum_confidence,
		std::move(rejection_reasons));
}
