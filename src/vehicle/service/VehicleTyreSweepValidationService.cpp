#include "vehicle/service/VehicleTyreSweepValidationService.h"

#include "geometry/service/MeshTopologyAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

VehicleTyrePoseSweep VehicleTyreSweepValidationService::validateAndBuildSweep(
	VehicleCornerLocation corner,
	const GeneratedPrimitiveMesh &source_tyre_mesh,
	std::vector<VehicleTyrePoseSample> pose_samples,
	const Mesh &final_body_mesh,
	const glm::dmat4 &source_to_progen3d_matrix,
	double declared_clearance_metres,
	double accepted_minimum_clearance_metres,
	double tessellation_tolerance_metres) const
{
	if (pose_samples.empty()) {
		throw std::invalid_argument("Vehicle tyre sweep requires at least one pose sample.");
	}
	std::vector<VehicleTransformedTyrePoseEvidence> evidence;
	std::vector<glm::dvec3> transformed_vertices;
	double minimum_clearance = std::numeric_limits<double>::infinity();
	bool all_poses_passed = true;
	for (const VehicleTyrePoseSample &sample : pose_samples) {
		const GeneratedPrimitiveMesh transformed = transformation_service_.transformSourceTyre(
			source_tyre_mesh, sample, source_to_progen3d_matrix);
		for (const glm::vec3 &vertex : transformed.mesh()->vertices) {
			transformed_vertices.emplace_back(vertex);
		}
		const MeshTopologyReport topology = MeshTopologyAnalyzer().analyze(
			*transformed.mesh(), transformed.faceSurfaceTags());
		const MeshSurfaceDistanceReport distance =
			distance_service_.evaluate(*transformed.mesh(), final_body_mesh);
		const MeshIntersectionScreeningReport intersections =
			intersection_service_.screen(*transformed.mesh(), final_body_mesh);
		std::size_t parity_sample_count = 0u;
		std::size_t inside_sample_count = 0u;
		const std::size_t stride = std::max<std::size_t>(
			1u, transformed.mesh()->vertices.size() / 24u);
		for (std::size_t vertex_index = 0u;
		     vertex_index < transformed.mesh()->vertices.size();
		     vertex_index += stride) {
			++parity_sample_count;
			const MeshRayParityClassification classification = parity_service_.classify(
				final_body_mesh,
				glm::dvec3(transformed.mesh()->vertices[vertex_index]));
			if (classification.containment() == MeshPointContainment::Inside) {
				++inside_sample_count;
			}
		}
		minimum_clearance = std::min(minimum_clearance, distance.minimumDistance());
		const bool pose_passed = topology.isWatertight() && distance.complete() &&
			distance.finite() && intersections.passed() && inside_sample_count == 0u &&
			distance.minimumDistance() + tessellation_tolerance_metres >=
			declared_clearance_metres;
		all_poses_passed = all_poses_passed && pose_passed;
		evidence.emplace_back(
			sample.wheelPose(), transformed.mesh()->vertices.size(),
			transformed.mesh()->faces.size(), topology.topology_hash,
			distance, intersections, parity_sample_count, inside_sample_count,
			pose_passed);
	}
	const ConvexHullMeshGenerationResult hull =
		hull_service_.generateConservativeEnvelope(transformed_vertices);
	const bool accepted_source_clearance =
		accepted_minimum_clearance_metres + tessellation_tolerance_metres >=
		declared_clearance_metres;
	const bool passed = all_poses_passed && accepted_source_clearance &&
		hull.report().passed();
	return VehicleTyrePoseSweep(
		corner, source_tyre_mesh, std::move(pose_samples), hull.hullMesh(),
		VehicleTyreSweepValidationReport(
			std::move(evidence), hull.report(), minimum_clearance, passed),
		accepted_minimum_clearance_metres, tessellation_tolerance_metres,
		accepted_source_clearance);
}
