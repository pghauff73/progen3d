#include "vehicle/mcsmv2/service/VehicleSemanticSurfaceGenerationService.h"

#include "geometry/model/Profile2D.h"
#include "geometry/model/ProfileLoop2D.h"
#include "geometry/model/ProfileWindingCorrection.h"
#include "geometry/model/SurfaceLoftShapeSpecification.h"
#include "geometry/service/LoftMeshGenerator.h"
#include "geometry/service/MeshTopologyAnalyzer.h"
#include "vehicle/mcsmv2/service/VehicleSectionInterpolationRelationshipService.h"
#include "vehicle/mcsmv2/service/VehicleSemanticSectionFieldEvaluationService.h"
#include "vehicle/mcsmv2/service/VehicleSemanticSectionInterpolationService.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

std::vector<glm::dvec3>
VehicleSemanticSurfaceGenerationService::createSourceSectionRing(
	const ModernCarSemanticVariant &variant,
	double source_x,
	std::size_t half_section_sample_count) const
{
	if (half_section_sample_count < 4u) {
		throw std::invalid_argument(
			"Semantic surface section requires at least four half-section samples.");
	}
	VehicleSemanticSectionFieldEvaluationService evaluation(variant.sectionField());
	const std::array<VehicleSectionLandmark, kVehicleSectionLandmarkCount> landmarks =
		evaluation.evaluateLandmarks(source_x);
	std::vector<double> widths;
	std::vector<double> heights;
	widths.reserve(landmarks.size());
	heights.reserve(landmarks.size());
	for (auto landmark = landmarks.rbegin(); landmark != landmarks.rend(); ++landmark) {
		widths.push_back(landmark->halfWidth());
		heights.push_back(landmark->height());
	}
	widths.back() = 0.0;
	std::vector<double> chord_parameters(widths.size(), 0.0);
	for (std::size_t index = 1u; index < widths.size(); ++index) {
		const double width_delta = widths[index] - widths[index - 1u];
		const double height_delta = heights[index] - heights[index - 1u];
		chord_parameters[index] = chord_parameters[index - 1u] +
			std::sqrt(width_delta * width_delta + height_delta * height_delta);
	}
	if (!(chord_parameters.back() > 1.0e-9)) {
		throw std::runtime_error("Semantic surface section collapsed.");
	}
	for (double &parameter : chord_parameters) parameter /= chord_parameters.back();
	VehicleSemanticSectionInterpolationService interpolation_service;
	const ShapePreservingCubicInterpolation width_interpolation =
		interpolation_service.createInterpolation(chord_parameters, widths);
	const ShapePreservingCubicInterpolation height_interpolation =
		interpolation_service.createInterpolation(chord_parameters, heights);
	std::vector<glm::dvec3> right_points;
	right_points.reserve(half_section_sample_count);
	for (std::size_t sample = 0u; sample < half_section_sample_count; ++sample) {
		const double parameter = static_cast<double>(sample) /
			static_cast<double>(half_section_sample_count - 1u);
		right_points.emplace_back(
			source_x,
			width_interpolation.evaluateAt(parameter),
			height_interpolation.evaluateAt(parameter));
	}
	std::vector<glm::dvec3> ring = right_points;
	for (std::size_t index = right_points.size() - 1u; index > 1u; --index) {
		const glm::dvec3 &right_point = right_points[index - 1u];
		ring.emplace_back(right_point.x, -right_point.y, right_point.z);
	}
	return ring;
}

VehicleSemanticSurface VehicleSemanticSurfaceGenerationService::generate(
	const ModernCarSemanticVariant &variant,
	std::size_t longitudinal_section_count,
	std::size_t half_section_sample_count) const
{
	if (longitudinal_section_count < 2u) {
		throw std::invalid_argument(
			"Semantic surface generation requires at least two longitudinal sections.");
	}
	const VehicleSemanticSectionField &section_field = variant.sectionField();
	VehicleSemanticSectionFieldEvaluationService evaluation(section_field);
	VehicleSectionInterpolationRelationshipService relationship_service;
	std::vector<LoftSectionSpecification> loft_sections;
	std::vector<VehicleSemanticSurfaceSection> semantic_sections;
	loft_sections.reserve(longitudinal_section_count);
	semantic_sections.reserve(longitudinal_section_count);
	for (std::size_t section_index = 0u;
	     section_index < longitudinal_section_count; ++section_index) {
		const double parameter = static_cast<double>(section_index) /
			static_cast<double>(longitudinal_section_count - 1u);
		const double source_x = evaluation.rearSourceX() +
			(evaluation.frontSourceX() - evaluation.rearSourceX()) * parameter;
		std::vector<glm::dvec3> source_ring = createSourceSectionRing(
			variant, source_x, half_section_sample_count);
		std::vector<glm::vec2> profile_points;
		profile_points.reserve(source_ring.size());
		for (const glm::dvec3 &source_point : source_ring) {
			profile_points.emplace_back(source_point.y, source_point.z);
		}
		loft_sections.emplace_back(
			static_cast<float>(source_x - variant.package().sourcePackageCenterStation()),
			Profile2D(
				ProfileLoop2D(
					std::move(profile_points), ProfileWindingCorrection::None),
				{}));
		semantic_sections.emplace_back(
			relationship_service.resolve(section_field, source_x),
			std::move(source_ring));
	}
	const std::string key =
		"MCSMv2:SemanticSurface:" + variant.identifier() + ":" +
		std::to_string(longitudinal_section_count) + ":" +
		std::to_string(half_section_sample_count);
	const SurfaceLoftShapeSpecification specification(
		std::move(loft_sections),
		ShapeSpecificationKey(key), key,
		GeometryDetailLevel::Assembly);
	const GeometryBuildResult result = LoftMeshGenerator().build(specification);
	if (!result.succeeded() || !result.generatedMesh().mesh()) {
		throw std::runtime_error(
			result.firstDiagnostic().empty()
				? "Semantic surface loft generation failed."
				: result.firstDiagnostic());
	}
	const std::size_t ring_size = semantic_sections.front().sourceRingPoints().size();
	const std::size_t faces_per_patch = ring_size * 2u;
	const std::size_t expected_face_count =
		(longitudinal_section_count - 1u) * faces_per_patch;
	if (result.generatedMesh().mesh()->faces.size() != expected_face_count) {
		throw std::runtime_error(
			"Semantic surface loft discarded one or more provenance-owned faces.");
	}
	std::vector<MeshSurfaceTag> provenance_tags;
	provenance_tags.reserve(expected_face_count);
	std::vector<VehicleSemanticSurfacePatch> patches;
	patches.reserve(longitudinal_section_count - 1u);
	for (std::size_t patch_index = 0u;
	     patch_index + 1u < longitudinal_section_count; ++patch_index) {
		const std::size_t first_face = patch_index * faces_per_patch;
		patches.emplace_back(
			patch_index, patch_index + 1u, first_face, faces_per_patch);
		for (std::size_t face = 0u; face < faces_per_patch; ++face) {
			provenance_tags.emplace_back(
				MeshSurfaceRole::ProfileOuterSide, 0u, patch_index);
		}
	}
	GeneratedPrimitiveMesh generated_mesh(
		result.generatedMesh().mesh(), std::move(provenance_tags));
	const MeshTopologyReport topology =
		MeshTopologyAnalyzer().analyze(*generated_mesh.mesh(), generated_mesh.faceSurfaceTags());
	if (topology.nonmanifold_edge_count != 0u ||
	    topology.degenerate_triangle_count != 0u) {
		throw std::runtime_error(
			"Semantic surface loft produced invalid topology.");
	}
	return VehicleSemanticSurface(
		variant.identifier() + ".SemanticSurface",
		std::move(generated_mesh),
		std::move(semantic_sections),
		std::move(patches),
		topology,
		topology.topology_hash);
}
