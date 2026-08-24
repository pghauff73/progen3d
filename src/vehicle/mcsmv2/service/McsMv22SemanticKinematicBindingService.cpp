#include "vehicle/mcsmv2/service/McsMv22SemanticKinematicBindingService.h"

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

GeneratedPrimitiveMesh createGeneratedMesh(std::shared_ptr<Mesh> mesh)
{
	mesh->calc_normals();
	return GeneratedPrimitiveMesh(
		mesh,
		std::vector<MeshSurfaceTag>(
			mesh->faces.size(), MeshSurfaceTag(MeshSurfaceRole::Outer)));
}

GeneratedPrimitiveMesh extractOwnedMesh(
	const Mesh &source,
	const VehicleSurfaceOwnership &ownership,
	const std::function<bool(const std::string &)> &select_owner)
{
	auto selected = std::make_shared<Mesh>();
	std::vector<int> remapped(source.vertices.size(), -1);
	for (std::size_t face_index = 0u; face_index < source.faces.size(); ++face_index) {
		if (!select_owner(ownership.faceOwnerIdentifiers()[face_index])) continue;
		const glm::ivec3 source_face = source.faces[face_index];
		glm::ivec3 target_face(0);
		for (int corner = 0; corner < 3; ++corner) {
			const int source_index = source_face[corner];
			int &target_index = remapped[static_cast<std::size_t>(source_index)];
			if (target_index < 0) {
				target_index = static_cast<int>(selected->vertices.size());
				selected->vertices.push_back(source.vertices[static_cast<std::size_t>(source_index)]);
			}
			target_face[corner] = target_index;
		}
		selected->faces.push_back(target_face);
	}
	return createGeneratedMesh(std::move(selected));
}

} // namespace

McsMv22SemanticKinematicBindings McsMv22SemanticKinematicBindingService::bind(
	const McsMv21SemanticGeometry &semantic_geometry,
	const McsMv22KinematicVariantDefinition &kinematic_definition) const
{
	if (semantic_geometry.variantIdentifier() != kinematic_definition.identifier()) {
		throw std::invalid_argument(
			"MCSMv2.2 semantic geometry variant does not match kinematic definition.");
	}
	const VehicleSurfaceOwnership &ownership =
		semantic_geometry.domainPartition().ownership();
	if (!ownership.passed() || ownership.faceOwnerIdentifiers().size() !=
	    semantic_geometry.finalBodyMesh().mesh()->faces.size()) {
		throw std::invalid_argument(
			"MCSMv2.2 semantic ownership is incomplete or does not match final-body faces.");
	}
	if (kinematic_definition.panelOwners().size() != 17u ||
	    kinematic_definition.apertureOwners().size() != 6u) {
		throw std::invalid_argument(
			"MCSMv2.2 semantic binding requires 17 panel and six aperture records.");
	}
	for (const McsMv22SurfaceOwnerDefinition &owner :
	     kinematic_definition.panelOwners()) {
		const bool exact_owner_exists =
			ownership.ownerFaceCounts().count(owner.identifier()) != 0u;
		const bool inherited_static_alias_exists =
			owner.identifier() == "underbody" && !owner.isClosure() &&
			ownership.ownerFaceCounts().count("fixed_body") != 0u;
		if (!exact_owner_exists && !inherited_static_alias_exists) {
			throw std::invalid_argument(
				"MCSMv2.2 panel owner is absent from inherited semantic ownership: " +
				owner.identifier());
		}
	}
	for (const McsMv22SurfaceOwnerDefinition &owner :
	     kinematic_definition.apertureOwners()) {
		if (ownership.ownerFaceCounts().count("aperture:" + owner.identifier()) == 0u) {
			throw std::invalid_argument(
				"MCSMv2.2 aperture owner is absent from inherited semantic ownership: " +
				owner.identifier());
		}
	}
	std::set<std::string> closure_identifiers;
	std::map<std::string, GeneratedPrimitiveMesh> closure_meshes;
	for (const McsMv22ClosureHingeDefinition &hinge : kinematic_definition.hinges()) {
		if (!closure_identifiers.insert(hinge.closureIdentifier()).second) {
			throw std::invalid_argument("MCSMv2.2 closure binding identifier is duplicated.");
		}
		GeneratedPrimitiveMesh closure_mesh = extractOwnedMesh(
			*semantic_geometry.finalBodyMesh().mesh(), ownership,
			[&hinge](const std::string &owner) {
				return owner == hinge.closureIdentifier();
			});
		if (closure_mesh.mesh()->faces.empty()) {
			throw std::invalid_argument(
				"MCSMv2.2 closure semantic mesh is empty: " + hinge.closureIdentifier());
		}
		closure_meshes.emplace(hinge.closureIdentifier(), std::move(closure_mesh));
	}
	std::map<std::string, GeneratedPrimitiveMesh> glass_meshes;
	for (const McsMv22HelicalGlassDefinition &glass : kinematic_definition.glassSystems()) {
		const VehicleGlassAperture *matched_aperture = nullptr;
		for (const VehicleGlassAperture &aperture : semantic_geometry.glassApertures()) {
			if (aperture.identifier() == glass.identifier()) {
				matched_aperture = &aperture;
				break;
			}
		}
		if (matched_aperture == nullptr || matched_aperture->glassMesh().mesh()->faces.empty()) {
			throw std::invalid_argument(
				"MCSMv2.2 glass semantic mesh is empty or missing: " + glass.identifier());
		}
		glass_meshes.emplace(glass.identifier(), matched_aperture->glassMesh());
	}
	if (closure_meshes.size() != 6u || glass_meshes.size() != 4u) {
		throw std::invalid_argument(
			"MCSMv2.2 semantic binding requires six closure and four moving-glass meshes.");
	}
	GeneratedPrimitiveMesh fixed_body_mesh = extractOwnedMesh(
		*semantic_geometry.finalBodyMesh().mesh(), ownership,
		[&closure_identifiers](const std::string &owner) {
			return owner.rfind("aperture:", 0u) != 0u &&
			       closure_identifiers.count(owner) == 0u;
		});
	if (fixed_body_mesh.mesh()->faces.empty()) {
		throw std::invalid_argument("MCSMv2.2 fixed-body semantic mesh is empty.");
	}
	return McsMv22SemanticKinematicBindings(
		semantic_geometry.finalBodyMesh(), std::move(fixed_body_mesh),
		std::move(closure_meshes),
		std::move(glass_meshes));
}
