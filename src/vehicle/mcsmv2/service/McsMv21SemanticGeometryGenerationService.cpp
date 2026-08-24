#include "vehicle/mcsmv2/service/McsMv21SemanticGeometryGenerationService.h"

#include "vehicle/mcsmv2/service/ModernCarSemanticIsoSurfaceGenerationService.h"
#include "vehicle/mcsmv2/service/VehicleGlassApertureConstructionService.h"
#include "vehicle/mcsmv2/service/VehicleSemanticSurfaceRegistrationService.h"
#include "vehicle/mcsmv2/service/VehicleSurfaceDomainPartitionService.h"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

GeneratedPrimitiveMesh createGeneratedMesh(std::shared_ptr<Mesh> mesh)
{
	const std::size_t face_count = mesh ? mesh->faces.size() : 0u;
	return GeneratedPrimitiveMesh(
		std::move(mesh),
		std::vector<MeshSurfaceTag>(
			face_count,
			MeshSurfaceTag(MeshSurfaceRole::Outer)));
}

GeneratedPrimitiveMesh extractExteriorBodyMesh(
	const Mesh &body_mesh,
	const VehicleSurfaceOwnership &ownership)
{
	if (ownership.faceOwnerIdentifiers().size() != body_mesh.faces.size()) {
		throw std::invalid_argument(
			"Exterior body extraction requires one owner for every body face.");
	}
	auto exterior_mesh = std::make_shared<Mesh>();
	std::vector<int> remapped_vertices(body_mesh.vertices.size(), -1);
	for (std::size_t face_index = 0u;
	     face_index < body_mesh.faces.size();
	     ++face_index) {
		const std::string &owner = ownership.faceOwnerIdentifiers()[face_index];
		if (owner.rfind("aperture:", 0u) == 0u) continue;
		const glm::ivec3 source_face = body_mesh.faces[face_index];
		glm::ivec3 target_face(0);
		const int source_indices[3] = {
			source_face.x, source_face.y, source_face.z};
		for (int corner = 0; corner < 3; ++corner) {
			const int source_index = source_indices[corner];
			int &target_index =
				remapped_vertices[static_cast<std::size_t>(source_index)];
			if (target_index < 0) {
				target_index = static_cast<int>(exterior_mesh->vertices.size());
				exterior_mesh->vertices.push_back(
					body_mesh.vertices[static_cast<std::size_t>(source_index)]);
			}
			target_face[corner] = target_index;
		}
		exterior_mesh->faces.push_back(target_face);
	}
	exterior_mesh->calc_normals();
	exterior_mesh->buildCollisionAccel();
	return createGeneratedMesh(std::move(exterior_mesh));
}

} // namespace

GeneratedMcsMv21SemanticGeometryResult
McsMv21SemanticGeometryGenerationService::generate(
	const ModernCarSemanticVariant &variant,
	const McsMv21SemanticVariantDefinition &semantic_definition,
	const ParametricModelGenerationPolicy &final_body_policy,
	const ParametricModelGenerationPolicy &registration_policy,
	std::size_t domain_grid_samples) const
{
	ModernCarSemanticValidationReport validation;
	const GeneratedModernCarSemanticBodyResult body_result =
		ModernCarSemanticIsoSurfaceGenerationService().generate(
			variant, final_body_policy);
	validation.append(body_result.validationReport());
	if (!body_result.generatedBody() ||
	    !body_result.generatedBody()->bodyMesh().mesh()) {
		return GeneratedMcsMv21SemanticGeometryResult(
			std::nullopt, std::move(validation));
	}
	const RegisteredVehicleSemanticSurfaceResult registration_result =
		VehicleSemanticSurfaceRegistrationService().registerSurface(
			variant, semantic_definition, registration_policy);
	validation.append(registration_result.validationReport());
	if (!registration_result.registeredSurface()) {
		return GeneratedMcsMv21SemanticGeometryResult(
			std::nullopt, std::move(validation));
	}
	try {
		auto final_body_mesh = std::make_shared<Mesh>(
			*body_result.generatedBody()->bodyMesh().mesh());
		final_body_mesh->buildCollisionAccel();
		const VehicleSurfaceDomainPartition partition =
			VehicleSurfaceDomainPartitionService().partition(
				*final_body_mesh,
				*registration_result.registeredSurface(),
				semantic_definition.domainCatalog(),
				domain_grid_samples);
		if (!partition.passed()) {
			validation.addIssue({
				ModernCarSemanticValidationCode::SurfaceOwnershipFailure,
				"MCSMv2.1 final body did not pass domain coverage and exclusive ownership.",
				{variant.identifier()}});
		}
		const std::vector<VehicleGlassAperture> glass_apertures =
			VehicleGlassApertureConstructionService().construct(
				*registration_result.registeredSurface(),
				semantic_definition.domainCatalog(),
				*final_body_mesh);
		const bool glass_passed =
			glass_apertures.size() == semantic_definition.domainCatalog().apertureDomains().size() &&
			std::all_of(
				glass_apertures.begin(),
				glass_apertures.end(),
				[](const VehicleGlassAperture &aperture) {
					return aperture.passed();
				});
		if (!glass_passed) {
			validation.addIssue({
				ModernCarSemanticValidationCode::GlassApertureFailure,
				"MCSMv2.1 one or more static glass apertures failed support or separation validation.",
				{variant.identifier()}});
		}
		GeneratedPrimitiveMesh final_body = createGeneratedMesh(final_body_mesh);
		GeneratedPrimitiveMesh exterior_body = extractExteriorBodyMesh(
			*final_body_mesh, partition.ownership());
		const bool assurance_v2_passed =
			validation.isValid() && semantic_definition.sourceReleaseGatePass() &&
			registration_result.registeredSurface()->correspondenceReport().passed() &&
			partition.passed() && glass_passed;
		return GeneratedMcsMv21SemanticGeometryResult(
			McsMv21SemanticGeometry(
				variant.identifier(),
				std::move(final_body),
				std::move(exterior_body),
				*registration_result.registeredSurface(),
				partition,
				glass_apertures,
				assurance_v2_passed),
			std::move(validation));
	}
	catch (const std::exception &error) {
		validation.addIssue({
			ModernCarSemanticValidationCode::InvalidSurfaceDomain,
			error.what(),
			{variant.identifier()}});
		return GeneratedMcsMv21SemanticGeometryResult(
			std::nullopt, std::move(validation));
	}
}
