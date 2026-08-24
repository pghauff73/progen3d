#include "vehicle/mcsmv2/service/VehicleSemanticSurfaceRegistrationService.h"

#include "geometry/model/ImplicitSurfaceGenerationRequest.h"
#include "geometry/service/ClosedSurfaceFaceOrientationService.h"
#include "geometry/service/ImplicitSurfaceMeshingService.h"
#include "geometry/service/MeshTopologyAnalyzer.h"
#include "vehicle/mcsmv2/model/InverseAffinePrewarpedField.h"
#include "vehicle/mcsmv2/model/SemanticSectionBodyField.h"
#include "vehicle/mcsmv2/service/ImplicitFieldCalibrationResolutionService.h"
#include "vehicle/mcsmv2/service/SemanticImplicitAgreementEvaluationService.h"
#include "vehicle/mcsmv2/service/VehicleSemanticSurfaceGenerationService.h"
#include "vehicle/mcsmv2/service/VehicleSurfaceProjectionService.h"

#include <cmath>
#include <array>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

std::vector<MeshSurfaceTag> outerSurfaceTags(std::size_t face_count)
{
	return std::vector<MeshSurfaceTag>(face_count, MeshSurfaceTag(MeshSurfaceRole::Outer));
}

GeneratedPrimitiveMesh createParameterizedSemanticMesh(
	const ModernCarSemanticVariant &variant,
	std::size_t longitudinal_section_count,
	std::size_t half_section_sample_count,
	std::vector<glm::dvec2> *surface_coordinates)
{
	if (surface_coordinates == nullptr || longitudinal_section_count < 2u ||
	    half_section_sample_count < 4u) {
		throw std::invalid_argument(
			"Parameterized semantic mesh requires valid samples and UV output.");
	}
	VehicleSemanticSurfaceGenerationService semantic_surface_service;
	VehicleSemanticSectionFieldEvaluationService section_evaluation(
		variant.sectionField());
	const std::size_t ring_size = half_section_sample_count * 2u - 2u;
	auto mesh = std::make_shared<Mesh>();
	mesh->vertices.reserve(longitudinal_section_count * ring_size + 2u);
	surface_coordinates->clear();
	surface_coordinates->reserve(longitudinal_section_count * ring_size + 2u);
	for (std::size_t section_index = 0u;
	     section_index < longitudinal_section_count;
	     ++section_index) {
		const double longitudinal_parameter = static_cast<double>(section_index) /
			static_cast<double>(longitudinal_section_count - 1u);
		const double source_x = section_evaluation.rearSourceX() +
			(section_evaluation.frontSourceX() - section_evaluation.rearSourceX()) *
			longitudinal_parameter;
		const std::vector<glm::dvec3> source_ring =
			semantic_surface_service.createSourceSectionRing(
				variant, source_x, half_section_sample_count);
		if (source_ring.size() != ring_size) {
			throw std::runtime_error("Semantic section ring size changed unexpectedly.");
		}
		for (std::size_t ring_index = 0u; ring_index < ring_size; ++ring_index) {
			mesh->vertices.push_back(glm::vec3(
				variant.referenceFrame().convertSourcePointToProgen3d(
					source_ring[ring_index])));
			surface_coordinates->emplace_back(
				longitudinal_parameter,
				static_cast<double>(ring_index) / static_cast<double>(ring_size));
		}
	}
	for (std::size_t section_index = 0u;
	     section_index + 1u < longitudinal_section_count;
	     ++section_index) {
		const std::size_t first_ring = section_index * ring_size;
		const std::size_t second_ring = (section_index + 1u) * ring_size;
		for (std::size_t ring_index = 0u; ring_index < ring_size; ++ring_index) {
			const std::size_t next_ring_index = (ring_index + 1u) % ring_size;
			mesh->faces.emplace_back(
				static_cast<int>(first_ring + ring_index),
				static_cast<int>(second_ring + ring_index),
				static_cast<int>(second_ring + next_ring_index));
			mesh->faces.emplace_back(
				static_cast<int>(first_ring + ring_index),
				static_cast<int>(second_ring + next_ring_index),
				static_cast<int>(first_ring + next_ring_index));
		}
	}
	const std::array<std::size_t, 2u> terminal_section_indices{{
		0u, longitudinal_section_count - 1u}};
	for (const std::size_t section_index : terminal_section_indices) {
		glm::dvec3 centre(0.0);
		const std::size_t ring_start = section_index * ring_size;
		for (std::size_t ring_index = 0u; ring_index < ring_size; ++ring_index) {
			centre += glm::dvec3(mesh->vertices[ring_start + ring_index]);
		}
		centre /= static_cast<double>(ring_size);
		const int centre_index = static_cast<int>(mesh->vertices.size());
		mesh->vertices.push_back(glm::vec3(centre));
		surface_coordinates->emplace_back(
			section_index == 0u ? 0.0 : 1.0, 0.5);
		for (std::size_t ring_index = 0u; ring_index < ring_size; ++ring_index) {
			const int current = static_cast<int>(ring_start + ring_index);
			const int next = static_cast<int>(
				ring_start + (ring_index + 1u) % ring_size);
			if (section_index == 0u) {
				mesh->faces.emplace_back(centre_index, next, current);
			}
			else {
				mesh->faces.emplace_back(centre_index, current, next);
			}
		}
	}
	const ClosedSurfaceFaceOrientationReport orientation =
		ClosedSurfaceFaceOrientationService().orientOutward(*mesh);
	if (!orientation.succeeded()) {
		throw std::runtime_error(orientation.diagnostic());
	}
	mesh->buildCollisionAccel();
	return GeneratedPrimitiveMesh(mesh, outerSurfaceTags(mesh->faces.size()));
}

GeneratedPrimitiveMesh createImplicitScaffold(
	const ModernCarSemanticVariant &variant,
	const ParametricModelGenerationPolicy &policy)
{
	const std::shared_ptr<const ImplicitScalarField> source_field =
		std::make_shared<SemanticSectionBodyField>(variant);
	const ImplicitSurfaceGenerationRequest request(
		policy.identifier() + ".MCSMv21.Scaffold",
		policy.longitudinalSamples(),
		policy.lateralSamples(),
		policy.verticalSamples(),
		policy.isoValue(),
		1,
		0.38,
		-0.41,
		GeometryDetailLevel::CoarseShape);
	const ImplicitFieldCalibrationResolutionResult calibration_result =
		ImplicitFieldCalibrationResolutionService().resolve(
			source_field, variant.fieldCalibration(), request);
	if (!calibration_result.succeeded() || !calibration_result.calibration()) {
		throw std::runtime_error(
			calibration_result.diagnostic().empty()
				? "MCSMv2.1 scaffold calibration failed."
				: calibration_result.diagnostic());
	}
	const std::shared_ptr<const ImplicitScalarField> calibrated_field =
		std::make_shared<InverseAffinePrewarpedField>(
			source_field, *calibration_result.calibration());
	const ImplicitSurfaceGenerationResult scaffold_result =
		ImplicitSurfaceMeshingService().generate(*calibrated_field, request);
	if (!scaffold_result.succeeded() || !scaffold_result.generatedMesh() ||
	    !scaffold_result.generatedMesh()->mesh()) {
		throw std::runtime_error(
			scaffold_result.firstDiagnostic().empty()
				? "MCSMv2.1 scaffold meshing failed."
				: scaffold_result.firstDiagnostic());
	}
	auto scaffold_mesh =
		std::make_shared<Mesh>(*scaffold_result.generatedMesh()->mesh());
	for (glm::vec3 &vertex : scaffold_mesh->vertices) {
		vertex = glm::vec3(variant.referenceFrame().convertSourcePointToProgen3d(
			glm::dvec3(vertex)));
	}
	const ClosedSurfaceFaceOrientationReport orientation =
		ClosedSurfaceFaceOrientationService().orientOutward(*scaffold_mesh);
	if (!orientation.succeeded()) {
		throw std::runtime_error(orientation.diagnostic());
	}
	scaffold_mesh->buildCollisionAccel();
	return GeneratedPrimitiveMesh(
		scaffold_mesh, outerSurfaceTags(scaffold_mesh->faces.size()));
}

GeneratedPrimitiveMesh registerSemanticMesh(
	const GeneratedPrimitiveMesh &raw_semantic_mesh,
	const GeneratedPrimitiveMesh &implicit_scaffold_mesh,
	const ModernCarSemanticVariant &variant,
	std::size_t longitudinal_section_count,
	std::size_t half_section_sample_count)
{
	const Mesh &raw_mesh = *raw_semantic_mesh.mesh();
	const Mesh &scaffold_mesh = *implicit_scaffold_mesh.mesh();
	auto registered_mesh = std::make_shared<Mesh>(raw_mesh);
	VehicleSurfaceProjectionService projection_service;
	for (std::size_t vertex_index = 0u;
	     vertex_index < raw_mesh.vertices.size();
	     ++vertex_index) {
		const glm::dvec3 raw_point(raw_mesh.vertices[vertex_index]);
		const std::optional<VehicleSurfaceProjection> projection =
			projection_service.projectPoint(scaffold_mesh, raw_point);
		if (!projection) {
			throw std::runtime_error("MCSMv2.1 semantic registration projection failed.");
		}
		registered_mesh->vertices[vertex_index] = glm::vec3(projection->projectedPoint());
	}
	const std::size_t ring_size = half_section_sample_count * 2u - 2u;
	const glm::dvec3 forward = glm::normalize(
		variant.referenceFrame().progen3dForwardDirection());
	auto preserveLongitudinalDatum = [&](std::size_t terminal_start,
	                                    std::size_t adjacent_start) {
		for (std::size_t ring_index = 0u; ring_index < ring_size; ++ring_index) {
			const std::size_t terminal_index = terminal_start + ring_index;
			const std::size_t adjacent_index = adjacent_start + ring_index;
			const glm::dvec3 projected_terminal(
				registered_mesh->vertices[terminal_index]);
			const glm::dvec3 projected_adjacent(
				registered_mesh->vertices[adjacent_index]);
			const glm::dvec3 raw_terminal(raw_mesh.vertices[terminal_index]);
			const glm::dvec3 raw_transverse =
				raw_terminal - forward * glm::dot(raw_terminal, forward);
			const glm::dvec3 blended_transverse =
				0.74 * (projected_terminal -
				        forward * glm::dot(projected_terminal, forward)) +
				0.25 * (projected_adjacent -
				        forward * glm::dot(projected_adjacent, forward)) +
				0.01 * raw_transverse;
			registered_mesh->vertices[terminal_index] = glm::vec3(
				blended_transverse + forward * glm::dot(raw_terminal, forward));
		}
	};
	preserveLongitudinalDatum(0u, ring_size);
	const std::size_t final_ring_start =
		(longitudinal_section_count - 1u) * ring_size;
	const std::size_t previous_ring_start =
		(longitudinal_section_count - 2u) * ring_size;
	preserveLongitudinalDatum(final_ring_start, previous_ring_start);
	for (const std::pair<std::size_t, std::size_t> centre_and_ring : {
			std::pair<std::size_t, std::size_t>(
				longitudinal_section_count * ring_size, 0u),
			std::pair<std::size_t, std::size_t>(
				longitudinal_section_count * ring_size + 1u, final_ring_start)}) {
		glm::dvec3 centre(0.0);
		for (std::size_t ring_index = 0u; ring_index < ring_size; ++ring_index) {
			centre += glm::dvec3(
				registered_mesh->vertices[centre_and_ring.second + ring_index]);
		}
		const double cap_direction = centre_and_ring.second == 0u ? -1.0 : 1.0;
		registered_mesh->vertices[centre_and_ring.first] = glm::vec3(
			centre / static_cast<double>(ring_size) +
			forward * cap_direction * 0.0001);
	}
	for (int repair_pass = 0; repair_pass < 4; ++repair_pass) {
		bool repaired_face = false;
		for (const glm::ivec3 &face : registered_mesh->faces) {
			const glm::dvec3 first(
				registered_mesh->vertices[static_cast<std::size_t>(face.x)]);
			const glm::dvec3 second(
				registered_mesh->vertices[static_cast<std::size_t>(face.y)]);
			const glm::dvec3 third(
				registered_mesh->vertices[static_cast<std::size_t>(face.z)]);
			if (glm::dot(glm::cross(second - first, third - first),
			             glm::cross(second - first, third - first)) > 1.0e-16) {
				continue;
			}
			const std::array<int, 3u> face_vertices{{face.x, face.y, face.z}};
			int repair_vertex = face.z;
			double largest_raw_offset = -1.0;
			for (const int vertex_index : face_vertices) {
				const glm::dvec3 raw_offset =
					glm::dvec3(raw_mesh.vertices[static_cast<std::size_t>(vertex_index)]) -
					glm::dvec3(registered_mesh->vertices[
						static_cast<std::size_t>(vertex_index)]);
				const double squared_offset = glm::dot(raw_offset, raw_offset);
				if (squared_offset > largest_raw_offset) {
					largest_raw_offset = squared_offset;
					repair_vertex = vertex_index;
				}
			}
			glm::dvec3 repair_direction =
				glm::dvec3(raw_mesh.vertices[static_cast<std::size_t>(repair_vertex)]) -
				glm::dvec3(registered_mesh->vertices[
					static_cast<std::size_t>(repair_vertex)]);
			if (glm::dot(repair_direction, repair_direction) <= 1.0e-18) {
				repair_direction = glm::cross(second - first, forward);
			}
			if (glm::dot(repair_direction, repair_direction) <= 1.0e-18) {
				repair_direction = glm::dvec3(0.0, 1.0, 0.0);
			}
			repair_direction = glm::normalize(repair_direction);
			registered_mesh->vertices[static_cast<std::size_t>(repair_vertex)] +=
				glm::vec3(repair_direction * 0.0002);
			repaired_face = true;
		}
		if (!repaired_face) break;
	}
	const ClosedSurfaceFaceOrientationReport orientation =
		ClosedSurfaceFaceOrientationService().orientOutward(*registered_mesh);
	if (!orientation.succeeded()) {
		throw std::runtime_error(orientation.diagnostic());
	}
	registered_mesh->buildCollisionAccel();
	return GeneratedPrimitiveMesh(
		registered_mesh, outerSurfaceTags(registered_mesh->faces.size()));
}

} // namespace

RegisteredVehicleSemanticSurfaceResult
VehicleSemanticSurfaceRegistrationService::registerSurface(
	const ModernCarSemanticVariant &variant,
	const McsMv21SemanticVariantDefinition &semantic_definition,
	const ParametricModelGenerationPolicy &scaffold_policy,
	std::size_t longitudinal_section_count,
	std::size_t half_section_sample_count,
	std::size_t maximum_correspondence_samples) const
{
	ModernCarSemanticValidationReport validation;
	try {
		std::vector<glm::dvec2> surface_coordinates;
		GeneratedPrimitiveMesh raw_semantic_mesh = createParameterizedSemanticMesh(
			variant,
			longitudinal_section_count,
			half_section_sample_count,
			&surface_coordinates);
		GeneratedPrimitiveMesh implicit_scaffold_mesh =
			createImplicitScaffold(variant, scaffold_policy);
		GeneratedPrimitiveMesh registered_semantic_mesh = registerSemanticMesh(
			raw_semantic_mesh,
			implicit_scaffold_mesh,
			variant,
			longitudinal_section_count,
			half_section_sample_count);
		const MeshTopologyReport topology = MeshTopologyAnalyzer().analyze(
			*registered_semantic_mesh.mesh(),
			registered_semantic_mesh.faceSurfaceTags());
		if (!topology.isWatertight()) {
			validation.addIssue({
				ModernCarSemanticValidationCode::NonWatertightMesh,
				"MCSMv2.1 registered semantic surface is not watertight.",
				{variant.identifier()}});
		}
		const SemanticImplicitCorrespondenceReport correspondence =
			SemanticImplicitAgreementEvaluationService().evaluate(
				*registered_semantic_mesh.mesh(),
				surface_coordinates,
				*implicit_scaffold_mesh.mesh(),
				semantic_definition.acceptedCorrespondence(),
				maximum_correspondence_samples);
		RegisteredVehicleSemanticSurface registered_surface(
			variant.identifier() + ".MCSMv21.RegisteredSemanticSurface",
			std::move(raw_semantic_mesh),
			std::move(implicit_scaffold_mesh),
			std::move(registered_semantic_mesh),
			std::move(surface_coordinates),
			topology,
			correspondence);
		if (!correspondence.passed()) {
			validation.addIssue({
				ModernCarSemanticValidationCode::SemanticImplicitCorrespondenceFailure,
				"MCSMv2.1 registered semantic surface exceeded the accepted correspondence tolerances.",
				{variant.identifier()}});
		}
		return RegisteredVehicleSemanticSurfaceResult(
			std::move(registered_surface),
			std::move(validation));
	}
	catch (const std::exception &error) {
		validation.addIssue({
			ModernCarSemanticValidationCode::SemanticImplicitCorrespondenceFailure,
			error.what(),
			{variant.identifier()}});
		return RegisteredVehicleSemanticSurfaceResult(
			std::nullopt, std::move(validation));
	}
}
