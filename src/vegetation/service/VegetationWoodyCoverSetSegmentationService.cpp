#include "vegetation/service/VegetationWoodyCoverSetSegmentationService.h"

#include "vegetation/service/VegetationWoodyPointSegmentationValidationService.h"

#include <Eigen/Eigenvalues>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <queue>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

constexpr const char *job_schema =
	"ProGen3D-VegetationPointCloudReconstructionJob-v1";
constexpr const char *ensemble_algorithm =
	"ProGen3D-AutomatedWoodySegmentationEnsemble-v1";
constexpr const char *segmentation_schema =
	"ProGen3D-VegetationWoodyPointSegmentation-v1";
constexpr const char *segmentation_algorithm =
	"ProGen3D-WoodyCoverSetSegmentation-v1";

using IssueCode = VegetationWoodySegmentationCandidateIssueCode;

struct CoverGridIndex
{
	std::int64_t x = 0;
	std::int64_t y = 0;
	std::int64_t z = 0;

	bool operator<(const CoverGridIndex &other) const
	{
		return std::tie(x, y, z) < std::tie(other.x, other.y, other.z);
	}
};

struct CoverPointCollection
{
	std::vector<const VegetationPointCloudPointRecord *> points;
};

struct SegmentationCoverConstruction
{
	CoverGridIndex grid_index;
	VegetationMeasuredPoint3d centroid_metres{0.0, 0.0, 0.0};
	std::vector<std::size_t> source_point_indices;
};

struct SegmentationCoverEdge
{
	std::size_t first_cover_index = 0u;
	std::size_t second_cover_index = 0u;
	double centre_distance_metres = 0.0;
};

struct SegmentationAxisConstruction
{
	std::size_t axis_index = 0u;
	std::optional<std::size_t> parent_axis_index;
	std::size_t branch_order = 0u;
	std::vector<std::size_t> owned_cover_indices;
	std::vector<std::size_t> centreline_cover_indices;
};

struct SegmentationAxisLine
{
	Eigen::Vector3d centroid = Eigen::Vector3d::Zero();
	Eigen::Vector3d direction = Eigen::Vector3d::UnitZ();
	double minimum_projection = 0.0;
	double maximum_projection = 0.0;
};

class CoverSpanningTreeDisjointSet
{
public:
	explicit CoverSpanningTreeDisjointSet(std::size_t element_count)
		: parents_(element_count), ranks_(element_count, 0u)
	{
		std::iota(parents_.begin(), parents_.end(), 0u);
	}

	std::size_t representative(std::size_t element)
	{
		if (parents_[element] != element) {
			parents_[element] = representative(parents_[element]);
		}
		return parents_[element];
	}

	bool connect(std::size_t first, std::size_t second)
	{
		std::size_t first_root = representative(first);
		std::size_t second_root = representative(second);
		if (first_root == second_root) return false;
		if (ranks_[first_root] < ranks_[second_root]) {
			std::swap(first_root, second_root);
		}
		parents_[second_root] = first_root;
		if (ranks_[first_root] == ranks_[second_root]) {
			++ranks_[first_root];
		}
		return true;
	}

private:
	std::vector<std::size_t> parents_;
	std::vector<std::size_t> ranks_;
};

bool fraction(double value)
{
	return std::isfinite(value) && value >= 0.0 && value <= 1.0;
}

bool valid_parameter_set(
	const VegetationWoodySegmentationParameterSet &parameter_set)
{
	return !parameter_set.parameterIdentifier().empty() &&
	       std::isfinite(parameter_set.coverCellSizeMetres()) &&
	       parameter_set.coverCellSizeMetres() > 0.0 &&
	       std::isfinite(parameter_set.neighbourRadiusMultiplier()) &&
	       parameter_set.neighbourRadiusMultiplier() > 0.0 &&
	       parameter_set.maximumCoverIndexDelta() > 0u &&
	       parameter_set.minimumCoverPointCount() > 0u &&
	       parameter_set.maximumCoverSetCount() > 1u &&
	       parameter_set.maximumAxisCount() > 0u &&
	       std::isfinite(parameter_set.minimumContinuationCosine()) &&
	       parameter_set.minimumContinuationCosine() >= -1.0 &&
	       parameter_set.minimumContinuationCosine() <= 1.0 &&
	       fraction(parameter_set.minimumAssignmentConfidence()) &&
	       parameter_set.minimumCylindersPerAxis() > 0u &&
	       std::isfinite(
		       parameter_set.minimumAxialStationSpacingMetres()) &&
	       parameter_set.minimumAxialStationSpacingMetres() > 0.0;
}

VegetationWoodySegmentationCandidateReport report(
	std::vector<VegetationWoodySegmentationCandidateIssue> issues,
	std::optional<VegetationWoodyPointSegmentationValidationReport>
		validation_report = std::nullopt,
	std::optional<VegetationWoodySegmentationCandidate> candidate = std::nullopt)
{
	return VegetationWoodySegmentationCandidateReport(
		std::move(issues), std::move(validation_report), std::move(candidate));
}

double distance(
	const VegetationMeasuredPoint3d &first,
	const VegetationMeasuredPoint3d &second)
{
	const double x = first.x() - second.x();
	const double y = first.y() - second.y();
	const double z = first.z() - second.z();
	return std::sqrt(x * x + y * y + z * z);
}

VegetationMeasuredPoint3d direction(
	const VegetationMeasuredPoint3d &start,
	const VegetationMeasuredPoint3d &end)
{
	const double x = end.x() - start.x();
	const double y = end.y() - start.y();
	const double z = end.z() - start.z();
	const double length = std::sqrt(x * x + y * y + z * z);
	if (length <= 0.0) return VegetationMeasuredPoint3d(0.0, 0.0, 0.0);
	return VegetationMeasuredPoint3d(x / length, y / length, z / length);
}

double dot(
	const VegetationMeasuredPoint3d &first,
	const VegetationMeasuredPoint3d &second)
{
	return first.x() * second.x() + first.y() * second.y() +
	       first.z() * second.z();
}

double point_segment_distance(
	const VegetationMeasuredPoint3d &point,
	const VegetationMeasuredPoint3d &start,
	const VegetationMeasuredPoint3d &end)
{
	const double segment_x = end.x() - start.x();
	const double segment_y = end.y() - start.y();
	const double segment_z = end.z() - start.z();
	const double point_x = point.x() - start.x();
	const double point_y = point.y() - start.y();
	const double point_z = point.z() - start.z();
	const double length_squared = segment_x * segment_x +
	                              segment_y * segment_y +
	                              segment_z * segment_z;
	const double factor = length_squared > 0.0
		                      ? std::clamp(
			                      (point_x * segment_x + point_y * segment_y +
			                       point_z * segment_z) /
				                      length_squared,
			                      0.0, 1.0)
		                      : 0.0;
	const VegetationMeasuredPoint3d closest(
		start.x() + segment_x * factor,
		start.y() + segment_y * factor,
		start.z() + segment_z * factor);
	return distance(point, closest);
}

double point_polyline_distance(
	const VegetationMeasuredPoint3d &point,
	const std::vector<VegetationMeasuredPoint3d> &polyline)
{
	if (polyline.empty()) return std::numeric_limits<double>::infinity();
	if (polyline.size() == 1u) return distance(point, polyline.front());
	double closest_distance = std::numeric_limits<double>::infinity();
	for (std::size_t index = 1u; index < polyline.size(); ++index) {
		closest_distance = std::min(
			closest_distance,
			point_segment_distance(point, polyline[index - 1u], polyline[index]));
	}
	return closest_distance;
}

double median(std::vector<double> values)
{
	if (values.empty()) return 0.0;
	std::sort(values.begin(), values.end());
	const std::size_t middle = values.size() / 2u;
	if (values.size() % 2u != 0u) return values[middle];
	return (values[middle - 1u] + values[middle]) * 0.5;
}

Eigen::Vector3d vector(const VegetationMeasuredPoint3d &point)
{
	return Eigen::Vector3d(point.x(), point.y(), point.z());
}

VegetationMeasuredPoint3d point(const Eigen::Vector3d &value)
{
	return VegetationMeasuredPoint3d(value.x(), value.y(), value.z());
}

std::optional<SegmentationAxisLine> solve_axis_line(
	const std::vector<VegetationMeasuredPoint3d> &points)
{
	if (points.size() < 3u) return std::nullopt;
	Eigen::Vector3d centroid = Eigen::Vector3d::Zero();
	for (const auto &sample : points) centroid += vector(sample);
	centroid /= static_cast<double>(points.size());
	Eigen::Matrix3d covariance = Eigen::Matrix3d::Zero();
	for (const auto &sample : points) {
		const Eigen::Vector3d offset = vector(sample) - centroid;
		covariance += offset * offset.transpose();
	}
	Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> solver(covariance);
	if (solver.info() != Eigen::Success ||
	    !solver.eigenvalues().allFinite() || solver.eigenvalues().z() <= 0.0) {
		return std::nullopt;
	}
	Eigen::Vector3d axis_direction =
		solver.eigenvectors().col(2).normalized();
	double minimum_projection = std::numeric_limits<double>::infinity();
	double maximum_projection = -std::numeric_limits<double>::infinity();
	for (const auto &sample : points) {
		const double projection =
			(vector(sample) - centroid).dot(axis_direction);
		minimum_projection = std::min(minimum_projection, projection);
		maximum_projection = std::max(maximum_projection, projection);
	}
	return SegmentationAxisLine{
		centroid, axis_direction, minimum_projection, maximum_projection};
}

double closest_child_line_projection(
	const SegmentationAxisLine &child,
	const SegmentationAxisLine &parent)
{
	const Eigen::Vector3d offset = child.centroid - parent.centroid;
	const double child_parent_dot = child.direction.dot(parent.direction);
	const double denominator = 1.0 - child_parent_dot * child_parent_dot;
	const double child_offset_dot = child.direction.dot(offset);
	const double parent_offset_dot = parent.direction.dot(offset);
	if (std::abs(denominator) <= 1.0e-9) return -child_offset_dot;
	return (child_parent_dot * parent_offset_dot - child_offset_dot) /
	       denominator;
}

std::int64_t grid_coordinate(double value, double minimum, double cell_size)
{
	return static_cast<std::int64_t>(
		std::floor((value - minimum) / cell_size + 1.0e-9));
}

}

VegetationWoodySegmentationCandidateReport
VegetationWoodyCoverSetSegmentationService::segment(
	const std::string &candidate_identifier,
	const VegetationPointCloudDataset &dataset,
	const VegetationPointCloudReconstructionAdmissionReport &admission_report,
	const VegetationPointCloudReconstructionJob &job,
	const VegetationWoodySegmentationParameterSet &parameter_set) const
{
	std::vector<VegetationWoodySegmentationCandidateIssue> issues;
	if (candidate_identifier.empty()) {
		issues.emplace_back(
			IssueCode::InvalidCandidateIdentifier,
			"Automated woody segmentation candidate identity is empty.");
	}
	if (!valid_parameter_set(parameter_set)) {
		issues.emplace_back(
			IssueCode::InvalidParameterSet,
			"Automated woody segmentation parameter set is invalid.");
	}
	if (job.schemaVersion() != job_schema) {
		issues.emplace_back(
			IssueCode::UnsupportedJobSchema,
			"Automated woody segmentation job schema is unsupported.");
	}
	if (job.target() != VegetationPointCloudReconstructionTarget::ShootArchitecture) {
		issues.emplace_back(
			IssueCode::WrongReconstructionTarget,
			"Automated woody segmentation requires ShootArchitecture intent.");
	}
	if (job.algorithmIdentifier() != ensemble_algorithm) {
		issues.emplace_back(
			IssueCode::UnsupportedAlgorithm,
			"Automated woody segmentation algorithm is unsupported.");
	}
	if (job.datasetIdentifier() != dataset.datasetIdentifier() ||
	    admission_report.datasetIdentifier() != dataset.datasetIdentifier()) {
		issues.emplace_back(
			IssueCode::DatasetIdentityMismatch,
			"Dataset, admission, and segmentation job identities differ.");
	}
	if (job.sourcePayloadSha256() !=
		    dataset.sourceMetadata().sourcePayloadSha256() ||
	    admission_report.sourcePayloadSha256() !=
		    dataset.sourceMetadata().sourcePayloadSha256()) {
		issues.emplace_back(
			IssueCode::SourceHashMismatch,
			"Dataset, admission, and segmentation job source hashes differ.");
	}
	if (!admission_report.admitted()) {
		issues.emplace_back(
			IssueCode::AdmissionRejected,
			"Point-cloud admission rejected automated woody segmentation.");
	}
	if (!issues.empty()) return report(std::move(issues));

	std::vector<const VegetationPointCloudPointRecord *> woody_points;
	for (const auto &point : dataset.points()) {
		if (point.organClass() == VegetationPointCloudOrganClass::Woody) {
			woody_points.push_back(&point);
		}
	}
	if (woody_points.size() < parameter_set.minimumCoverPointCount() * 2u) {
		issues.emplace_back(
			IssueCode::InsufficientWoodyEvidence,
			"Woody evidence cannot construct a connected cover graph.");
		return report(std::move(issues));
	}

	double minimum_x = std::numeric_limits<double>::infinity();
	double minimum_y = std::numeric_limits<double>::infinity();
	double minimum_z = std::numeric_limits<double>::infinity();
	for (const auto *point : woody_points) {
		minimum_x = std::min(minimum_x, point->positionMetres().x());
		minimum_y = std::min(minimum_y, point->positionMetres().y());
		minimum_z = std::min(minimum_z, point->positionMetres().z());
	}
	std::map<CoverGridIndex, CoverPointCollection> cover_collections;
	for (const auto *point : woody_points) {
		const CoverGridIndex index{
			grid_coordinate(
				point->positionMetres().x(), minimum_x,
				parameter_set.coverCellSizeMetres()),
			grid_coordinate(
				point->positionMetres().y(), minimum_y,
				parameter_set.coverCellSizeMetres()),
			grid_coordinate(
				point->positionMetres().z(), minimum_z,
				parameter_set.coverCellSizeMetres())};
		cover_collections[index].points.push_back(point);
	}
	std::vector<SegmentationCoverConstruction> covers;
	for (const auto &entry : cover_collections) {
		if (entry.second.points.size() <
		    parameter_set.minimumCoverPointCount()) {
			continue;
		}
		double x_sum = 0.0;
		double y_sum = 0.0;
		double z_sum = 0.0;
		std::vector<std::size_t> source_indices;
		for (const auto *point : entry.second.points) {
			x_sum += point->positionMetres().x();
			y_sum += point->positionMetres().y();
			z_sum += point->positionMetres().z();
			source_indices.push_back(point->sourceIndex());
		}
		std::sort(source_indices.begin(), source_indices.end());
		const double divisor = static_cast<double>(entry.second.points.size());
		covers.push_back(SegmentationCoverConstruction{
			entry.first,
			VegetationMeasuredPoint3d(
				x_sum / divisor, y_sum / divisor, z_sum / divisor),
			std::move(source_indices)});
	}
	if (covers.size() < 2u) {
		issues.emplace_back(
			IssueCode::InsufficientWoodyEvidence,
			"Woody cover partition contains fewer than two admitted sets.");
		return report(std::move(issues));
	}
	if (covers.size() > parameter_set.maximumCoverSetCount()) {
		issues.emplace_back(
			IssueCode::CoverSetLimitExceeded,
			"Woody cover-set count exceeds the parameter boundary.");
		return report(std::move(issues));
	}

	std::vector<SegmentationCoverEdge> candidate_edges;
	const auto index_distance = [](std::int64_t first, std::int64_t second) {
		return first >= second ? first - second : second - first;
	};
	const double maximum_neighbour_distance =
		parameter_set.coverCellSizeMetres() *
		parameter_set.neighbourRadiusMultiplier();
	for (std::size_t first = 0u; first < covers.size(); ++first) {
		for (std::size_t second = first + 1u; second < covers.size(); ++second) {
			const auto &first_grid = covers[first].grid_index;
			const auto &second_grid = covers[second].grid_index;
			if (index_distance(first_grid.x, second_grid.x) >
				    static_cast<std::int64_t>(
					    parameter_set.maximumCoverIndexDelta()) ||
			    index_distance(first_grid.y, second_grid.y) >
				    static_cast<std::int64_t>(
					    parameter_set.maximumCoverIndexDelta()) ||
			    index_distance(first_grid.z, second_grid.z) >
				    static_cast<std::int64_t>(
					    parameter_set.maximumCoverIndexDelta())) {
				continue;
			}
			const double centre_distance = distance(
				covers[first].centroid_metres,
				covers[second].centroid_metres);
			if (centre_distance <= maximum_neighbour_distance) {
				candidate_edges.push_back(
					SegmentationCoverEdge{first, second, centre_distance});
			}
		}
	}
	std::sort(
		candidate_edges.begin(), candidate_edges.end(),
		[](const SegmentationCoverEdge &first,
		   const SegmentationCoverEdge &second) {
			return std::tie(
				       first.centre_distance_metres, first.first_cover_index,
				       first.second_cover_index) <
			       std::tie(
				       second.centre_distance_metres, second.first_cover_index,
				       second.second_cover_index);
		});
	CoverSpanningTreeDisjointSet disjoint_set(covers.size());
	std::vector<SegmentationCoverEdge> spanning_tree_edges;
	std::vector<std::vector<std::size_t>> adjacency(covers.size());
	for (const auto &edge : candidate_edges) {
		if (!disjoint_set.connect(
			    edge.first_cover_index, edge.second_cover_index)) {
			continue;
		}
		spanning_tree_edges.push_back(edge);
		adjacency[edge.first_cover_index].push_back(edge.second_cover_index);
		adjacency[edge.second_cover_index].push_back(edge.first_cover_index);
	}
	if (spanning_tree_edges.size() + 1u != covers.size()) {
		issues.emplace_back(
			IssueCode::DisconnectedCoverGraph,
			"Woody cover graph is not one connected component.");
		return report(std::move(issues));
	}
	for (auto &neighbours : adjacency) {
		std::sort(neighbours.begin(), neighbours.end());
	}

	const auto root = std::min_element(
		covers.begin(), covers.end(),
		[](const SegmentationCoverConstruction &first,
		   const SegmentationCoverConstruction &second) {
			return std::make_tuple(
				       first.centroid_metres.z(), first.centroid_metres.x(),
				       first.centroid_metres.y(), first.grid_index.x,
				       first.grid_index.y, first.grid_index.z) <
			       std::make_tuple(
				       second.centroid_metres.z(), second.centroid_metres.x(),
				       second.centroid_metres.y(), second.grid_index.x,
				       second.grid_index.y, second.grid_index.z);
		});
	const std::size_t root_index =
		static_cast<std::size_t>(std::distance(covers.begin(), root));
	const std::size_t unavailable_index = std::numeric_limits<std::size_t>::max();
	std::vector<std::size_t> parents(covers.size(), unavailable_index);
	std::vector<std::vector<std::size_t>> children(covers.size());
	std::vector<std::size_t> traversal_order;
	std::queue<std::size_t> pending;
	parents[root_index] = root_index;
	pending.push(root_index);
	while (!pending.empty()) {
		const std::size_t current = pending.front();
		pending.pop();
		traversal_order.push_back(current);
		for (const std::size_t neighbour : adjacency[current]) {
			if (parents[neighbour] != unavailable_index) continue;
			parents[neighbour] = current;
			children[current].push_back(neighbour);
			pending.push(neighbour);
		}
	}
	for (auto &child_indices : children) {
		std::sort(child_indices.begin(), child_indices.end());
	}
	std::vector<double> subtree_maximum_z(covers.size(), 0.0);
	std::vector<double> subtree_path_length(covers.size(), 0.0);
	for (auto iterator = traversal_order.rbegin();
	     iterator != traversal_order.rend(); ++iterator) {
		const std::size_t current = *iterator;
		subtree_maximum_z[current] = covers[current].centroid_metres.z();
		for (const std::size_t child : children[current]) {
			subtree_maximum_z[current] = std::max(
				subtree_maximum_z[current], subtree_maximum_z[child]);
			subtree_path_length[current] = std::max(
				subtree_path_length[current],
				distance(
					covers[current].centroid_metres,
					covers[child].centroid_metres) +
					subtree_path_length[child]);
		}
	}

	std::vector<SegmentationAxisConstruction> axis_constructions;
	std::function<void(
		std::size_t, std::optional<std::size_t>, std::optional<std::size_t>,
		std::size_t)>
		construct_axis;
	construct_axis = [&](std::size_t start_cover,
	                    std::optional<std::size_t> junction_cover,
	                    std::optional<std::size_t> parent_axis,
	                    std::size_t branch_order) {
		if (axis_constructions.size() >= parameter_set.maximumAxisCount()) {
			issues.emplace_back(
				IssueCode::AxisLimitExceeded,
				"Automated woody axis count exceeds the parameter boundary.");
			return;
		}
		SegmentationAxisConstruction axis;
		axis.axis_index = axis_constructions.size();
		axis.parent_axis_index = parent_axis;
		axis.branch_order = branch_order;
		if (junction_cover.has_value()) {
			axis.centreline_cover_indices.push_back(*junction_cover);
		}
		std::size_t current = start_cover;
		std::optional<std::size_t> previous = junction_cover;
		std::vector<std::pair<std::size_t, std::size_t>> spawned_axes;
		while (true) {
			axis.owned_cover_indices.push_back(current);
			axis.centreline_cover_indices.push_back(current);
			const auto &current_children = children[current];
			if (current_children.empty()) break;
			std::optional<std::size_t> continuation;
			if (!previous.has_value()) {
				continuation = *std::max_element(
					current_children.begin(), current_children.end(),
					[&](std::size_t first, std::size_t second) {
						return std::tie(
							       subtree_maximum_z[first],
							       subtree_path_length[first], first) <
						       std::tie(
							       subtree_maximum_z[second],
							       subtree_path_length[second], second);
					});
			} else {
				const VegetationMeasuredPoint3d incoming = direction(
					covers[*previous].centroid_metres,
					covers[current].centroid_metres);
				double best_cosine = -std::numeric_limits<double>::infinity();
				std::size_t best_child = unavailable_index;
				for (const std::size_t child : current_children) {
					const double cosine = dot(
						incoming,
						direction(
							covers[current].centroid_metres,
							covers[child].centroid_metres));
					if (cosine > best_cosine + 1.0e-12 ||
					    (std::abs(cosine - best_cosine) <= 1.0e-12 &&
					     child < best_child)) {
						best_cosine = cosine;
						best_child = child;
					}
				}
				if (best_cosine >= parameter_set.minimumContinuationCosine()) {
					continuation = best_child;
				}
			}
			for (const std::size_t child : current_children) {
				if (!continuation.has_value() || child != *continuation) {
					spawned_axes.emplace_back(child, current);
				}
			}
			if (!continuation.has_value()) break;
			previous = current;
			current = *continuation;
		}
		if (axis.centreline_cover_indices.size() < 2u) {
			issues.emplace_back(
				IssueCode::InvalidAxisCandidate,
				"Automated woody axis candidate has no measurable span.");
			return;
		}
		const std::size_t current_axis_index = axis.axis_index;
		axis_constructions.push_back(std::move(axis));
		for (const auto &spawned : spawned_axes) {
			if (!issues.empty()) return;
			construct_axis(
				spawned.first, spawned.second, current_axis_index,
				branch_order + 1u);
		}
	};
	construct_axis(root_index, std::nullopt, std::nullopt, 0u);
	if (!issues.empty()) return report(std::move(issues));

	std::vector<std::string> cover_identifiers;
	std::vector<VegetationWoodyCoverSet> cover_models;
	cover_identifiers.reserve(covers.size());
	cover_models.reserve(covers.size());
	for (std::size_t index = 0u; index < covers.size(); ++index) {
		const std::string identifier =
			candidate_identifier + ":cover:" + std::to_string(index);
		cover_identifiers.push_back(identifier);
		cover_models.emplace_back(
			identifier, covers[index].grid_index.x, covers[index].grid_index.y,
			covers[index].grid_index.z, covers[index].centroid_metres,
			covers[index].source_point_indices);
	}
	std::vector<VegetationWoodyCoverConnection> cover_connections;
	cover_connections.reserve(covers.size() - 1u);
	for (std::size_t child = 0u; child < covers.size(); ++child) {
		if (child == root_index) continue;
		cover_connections.emplace_back(
			cover_identifiers[parents[child]], cover_identifiers[child],
			distance(
				covers[parents[child]].centroid_metres,
				covers[child].centroid_metres));
	}

	std::vector<std::vector<std::string>> owned_cover_identifiers_by_axis;
	std::vector<double> axial_station_spacing_by_axis;
	std::vector<VegetationWoodyAxisSegmentDefinition> axis_definitions;
	std::vector<std::vector<VegetationMeasuredPoint3d>> axis_polylines;
	owned_cover_identifiers_by_axis.reserve(axis_constructions.size());
	axial_station_spacing_by_axis.reserve(axis_constructions.size());
	axis_definitions.reserve(axis_constructions.size());
	axis_polylines.reserve(axis_constructions.size());
	for (const auto &axis : axis_constructions) {
		const std::string axis_identifier =
			"axis:auto:" + std::to_string(axis.axis_index);
		const std::string parent_axis_identifier = axis.parent_axis_index.has_value()
			                                           ? "axis:auto:" +
				                                             std::to_string(
					                                             *axis.parent_axis_index)
			                                           : std::string();
		std::vector<std::string> owned_cover_identifiers;
		for (const std::size_t cover_index : axis.owned_cover_indices) {
			owned_cover_identifiers.push_back(cover_identifiers[cover_index]);
		}
		std::vector<VegetationMeasuredPoint3d> centreline;
		std::vector<double> edge_lengths;
		double axis_length = 0.0;
		for (const std::size_t cover_index : axis.centreline_cover_indices) {
			centreline.push_back(covers[cover_index].centroid_metres);
			if (centreline.size() > 1u) {
				const double edge_length = distance(
					centreline[centreline.size() - 2u], centreline.back());
				edge_lengths.push_back(edge_length);
				axis_length += edge_length;
			}
		}
		const double station_spacing = std::max(
			parameter_set.minimumAxialStationSpacingMetres(),
			std::min(
				median(edge_lengths),
				axis_length /
					static_cast<double>(
						parameter_set.minimumCylindersPerAxis())));
		owned_cover_identifiers_by_axis.push_back(
			std::move(owned_cover_identifiers));
		axial_station_spacing_by_axis.push_back(station_spacing);
		axis_definitions.emplace_back(
			axis_identifier, parent_axis_identifier, axis.branch_order,
			station_spacing);
		axis_polylines.push_back(std::move(centreline));
	}
	auto provisional_assignments = [&]() {
		std::vector<std::size_t> assigned_axes;
		assigned_axes.reserve(woody_points.size());
		for (const auto *source_point : woody_points) {
			std::size_t closest_axis = 0u;
			double closest_distance = std::numeric_limits<double>::infinity();
			for (std::size_t axis_index = 0u;
			     axis_index < axis_polylines.size(); ++axis_index) {
				const double axis_distance = point_polyline_distance(
					source_point->positionMetres(), axis_polylines[axis_index]);
				if (axis_distance < closest_distance - 1.0e-12 ||
				    (std::abs(axis_distance - closest_distance) <= 1.0e-12 &&
				     axis_index < closest_axis)) {
					closest_distance = axis_distance;
					closest_axis = axis_index;
				}
			}
			assigned_axes.push_back(closest_axis);
		}
		return assigned_axes;
	};
	for (std::size_t refinement = 0u; refinement < 2u; ++refinement) {
		const auto assigned_axes = provisional_assignments();
		std::vector<std::vector<VegetationMeasuredPoint3d>> points_by_axis(
			axis_polylines.size());
		for (std::size_t point_index = 0u; point_index < woody_points.size();
		     ++point_index) {
			points_by_axis[assigned_axes[point_index]].push_back(
				woody_points[point_index]->positionMetres());
		}
		std::vector<std::optional<SegmentationAxisLine>> fitted_lines(
			axis_polylines.size());
		for (std::size_t axis_index = 0u; axis_index < axis_polylines.size();
		     ++axis_index) {
			fitted_lines[axis_index] = solve_axis_line(points_by_axis[axis_index]);
			if (!fitted_lines[axis_index].has_value()) continue;
			const auto &line = *fitted_lines[axis_index];
			double start_projection = line.minimum_projection;
			double end_projection = line.maximum_projection;
			if (axis_constructions[axis_index].parent_axis_index.has_value()) {
				const std::size_t parent_axis_index =
					*axis_constructions[axis_index].parent_axis_index;
				if (fitted_lines[parent_axis_index].has_value()) {
					start_projection = closest_child_line_projection(
						line, *fitted_lines[parent_axis_index]);
					const double minimum_span = std::abs(
						line.minimum_projection - start_projection);
					const double maximum_span = std::abs(
						line.maximum_projection - start_projection);
					end_projection = maximum_span >= minimum_span
						                 ? line.maximum_projection
						                 : line.minimum_projection;
				}
			} else {
				const VegetationMeasuredPoint3d first_endpoint = point(
					line.centroid + line.direction * start_projection);
				const VegetationMeasuredPoint3d second_endpoint = point(
					line.centroid + line.direction * end_projection);
				if (distance(
					    second_endpoint, axis_polylines[axis_index].front()) <
				    distance(
					    first_endpoint, axis_polylines[axis_index].front())) {
					std::swap(start_projection, end_projection);
				}
			}
			axis_polylines[axis_index] = {
				point(line.centroid + line.direction * start_projection),
				point(line.centroid + line.direction * end_projection)};
		}
	}
	std::vector<VegetationWoodySegmentationAxisCandidate> axis_models;
	axis_models.reserve(axis_constructions.size());
	for (const auto &axis : axis_constructions) {
		const std::string axis_identifier =
			"axis:auto:" + std::to_string(axis.axis_index);
		const std::string parent_axis_identifier = axis.parent_axis_index.has_value()
			                                           ? "axis:auto:" +
				                                             std::to_string(
					                                             *axis.parent_axis_index)
			                                           : std::string();
		axis_models.emplace_back(
			axis_identifier, parent_axis_identifier, axis.branch_order,
			owned_cover_identifiers_by_axis[axis.axis_index],
			axis_polylines[axis.axis_index],
			axial_station_spacing_by_axis[axis.axis_index]);
	}

	std::vector<VegetationWoodyPointSegmentAssignment> assignments;
	assignments.reserve(woody_points.size());
	double minimum_confidence = 1.0;
	for (const auto *point : woody_points) {
		std::size_t closest_axis = 0u;
		double closest_distance = std::numeric_limits<double>::infinity();
		double second_distance = std::numeric_limits<double>::infinity();
		for (std::size_t axis_index = 0u; axis_index < axis_polylines.size();
		     ++axis_index) {
			const double axis_distance = point_polyline_distance(
				point->positionMetres(), axis_polylines[axis_index]);
			if (axis_distance < closest_distance - 1.0e-12 ||
			    (std::abs(axis_distance - closest_distance) <= 1.0e-12 &&
			     axis_index < closest_axis)) {
				second_distance = closest_distance;
				closest_distance = axis_distance;
				closest_axis = axis_index;
			} else if (axis_distance < second_distance) {
				second_distance = axis_distance;
			}
		}
		double confidence = 1.0;
		if (axis_polylines.size() > 1u) {
			confidence = std::clamp(
				1.0 - closest_distance /
					      (second_distance +
					       parameter_set.coverCellSizeMetres()),
				0.0, 1.0);
		}
		minimum_confidence = std::min(minimum_confidence, confidence);
		assignments.emplace_back(
			point->sourceIndex(), "axis:auto:" + std::to_string(closest_axis),
			confidence);
	}
	if (minimum_confidence < parameter_set.minimumAssignmentConfidence()) {
		issues.emplace_back(
			IssueCode::AmbiguousPointAssignment,
			"Automated woody assignment confidence is below the parameter boundary.");
	}
	VegetationWoodyPointSegmentation segmentation(
		segmentation_schema, candidate_identifier + ":segmentation",
		dataset.datasetIdentifier(),
		dataset.sourceMetadata().sourcePayloadSha256(),
		segmentation_algorithm, "axis:auto:0", true,
		std::move(axis_definitions), std::move(assignments));
	const auto validation_report =
		VegetationWoodyPointSegmentationValidationService().validate(
			segmentation, dataset);
	if (!validation_report.acceptedForGraphConstruction()) {
		issues.emplace_back(
			IssueCode::SegmentationRejected,
			"Automated woody segmentation failed contract validation.");
	}
	VegetationWoodySegmentationCandidate candidate(
		candidate_identifier, parameter_set, cover_identifiers[root_index],
		std::move(cover_models), std::move(cover_connections),
		std::move(axis_models), std::move(segmentation));
	return report(
		std::move(issues), validation_report, std::move(candidate));
}
