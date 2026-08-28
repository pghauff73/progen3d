#include "vehicle/mcsmv2/service/McsMv22GeneratedMeshProvider.h"

#include "AppPaths.h"
#include "geometry/service/GeneratedMeshProviderRegistry.h"
#include "vehicle/mcsmv2/service/McsMv21SemanticCatalogFactory.h"
#include "vehicle/mcsmv2/service/McsMv21SemanticGeometryGenerationService.h"
#include "vehicle/mcsmv2/service/McsMv22ClosureKinematicEvaluationService.h"
#include "vehicle/mcsmv2/service/McsMv22KinematicCatalogFactory.h"
#include "vehicle/mcsmv2/service/McsMv22KinematicGeometryGenerationService.h"
#include "vehicle/mcsmv2/service/McsMv22NativeKinematicEvidenceLoadingService.h"
#include "vehicle/mcsmv2/service/ModernCarSemanticCatalogFactory.h"
#include "vehicle/service/VehicleSuspensionKinematicEvaluationService.h"
#include "vehicle/service/VehicleTyreMeshGenerationService.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {

class McsMv22GeneratedMeshProviderRegistration
{
public:
	McsMv22GeneratedMeshProviderRegistration()
	{
		GeneratedMeshProviderRegistry::accessProcessRegistry().registerProvider(
			std::make_shared<McsMv22GeneratedMeshProvider>());
	}
};

const McsMv22GeneratedMeshProviderRegistration
	mcsmv22_generated_mesh_provider_registration;

enum class McsMv22MeshComponent
{
	FixedBody,
	ClosuresClosed,
	ClosuresOpen,
	GlassClosed,
	GlassOpen,
	Suspension,
	NominalTyres,
	TyreSweeps
};

struct McsMv22MeshRequest
{
	std::string variant_identifier;
	McsMv22MeshComponent component = McsMv22MeshComponent::FixedBody;
};

struct McsMv22GeneratedMeshCacheEntry
{
	std::map<McsMv22MeshComponent, GeneratedPrimitiveMesh> meshes;
	std::string diagnostic;
};

std::optional<McsMv22MeshRequest> parseMeshKey(const std::string &mesh_key)
{
	const std::pair<const char *, const char *> variants[] = {
		{"Reference", "reference"}, {"Track", "track"},
		{"Aero", "aero"}, {"Crossover", "crossover"}};
	const std::pair<const char *, McsMv22MeshComponent> components[] = {
		{"FixedBodyMesh", McsMv22MeshComponent::FixedBody},
		{"ClosuresClosedMesh", McsMv22MeshComponent::ClosuresClosed},
		{"ClosuresOpenMesh", McsMv22MeshComponent::ClosuresOpen},
		{"GlassClosedMesh", McsMv22MeshComponent::GlassClosed},
		{"GlassOpenMesh", McsMv22MeshComponent::GlassOpen},
		{"SuspensionMesh", McsMv22MeshComponent::Suspension},
		{"NominalTyreMesh", McsMv22MeshComponent::NominalTyres},
		{"TyreSweepMesh", McsMv22MeshComponent::TyreSweeps}};
	for (const auto &variant : variants) {
		for (const auto &component : components) {
			if (mesh_key == std::string("MCSMv22") + variant.first + component.first) {
				return McsMv22MeshRequest{variant.second, component.second};
			}
		}
	}
	return std::nullopt;
}

const ModernCarSemanticVariant *findBaseVariant(
	const ModernCarSemanticFamily &family,
	const std::string &identifier)
{
	for (const ModernCarSemanticVariant &variant : family.variants()) {
		if (variant.identifier() == identifier) return &variant;
	}
	return nullptr;
}

const McsMv21SemanticVariantDefinition *findSemanticVariant(
	const McsMv21SemanticFamilyDefinition &family,
	const std::string &identifier)
{
	for (const McsMv21SemanticVariantDefinition &variant : family.variants()) {
		if (variant.identifier() == identifier) return &variant;
	}
	return nullptr;
}

const McsMv22KinematicVariantDefinition *findKinematicVariant(
	const McsMv22KinematicFamilyDefinition &family,
	const std::string &identifier)
{
	for (const McsMv22KinematicVariantDefinition &variant : family.variants()) {
		if (variant.identifier() == identifier) return &variant;
	}
	return nullptr;
}

std::filesystem::path locateRepositoryResource(
	const std::filesystem::path &relative_path)
{
	const std::filesystem::path application_resource =
		progen3d_resource_path(relative_path);
	if (std::filesystem::exists(application_resource)) {
		return application_resource;
	}
	std::error_code error;
	const std::filesystem::path working_directory_resource =
		std::filesystem::current_path(error) / relative_path;
	if (!error && std::filesystem::exists(working_directory_resource)) {
		return working_directory_resource.lexically_normal();
	}
	return application_resource;
}

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

void addTransformedMesh(
	Mesh *target,
	const GeneratedPrimitiveMesh &source,
	const glm::dmat4 &transform)
{
	Mesh transformed(*source.mesh());
	transformed.apply(glm::mat4(transform));
	target->add(transformed);
}

void addOctahedron(Mesh *mesh, const glm::dvec3 &centre, double radius)
{
	const int first_vertex = static_cast<int>(mesh->vertices.size());
	mesh->vertices.emplace_back(glm::vec3(centre + glm::dvec3(radius, 0.0, 0.0)));
	mesh->vertices.emplace_back(glm::vec3(centre + glm::dvec3(-radius, 0.0, 0.0)));
	mesh->vertices.emplace_back(glm::vec3(centre + glm::dvec3(0.0, radius, 0.0)));
	mesh->vertices.emplace_back(glm::vec3(centre + glm::dvec3(0.0, -radius, 0.0)));
	mesh->vertices.emplace_back(glm::vec3(centre + glm::dvec3(0.0, 0.0, radius)));
	mesh->vertices.emplace_back(glm::vec3(centre + glm::dvec3(0.0, 0.0, -radius)));
	const int faces[][3] = {
		{0, 2, 4}, {2, 1, 4}, {1, 3, 4}, {3, 0, 4},
		{2, 0, 5}, {1, 2, 5}, {3, 1, 5}, {0, 3, 5}};
	for (const auto &face : faces) {
		mesh->faces.emplace_back(
			first_vertex + face[0], first_vertex + face[1], first_vertex + face[2]);
	}
}

const VehicleGlassAperture *findGlassAperture(
	const McsMv21SemanticGeometry &geometry,
	const std::string &identifier)
{
	for (const VehicleGlassAperture &aperture : geometry.glassApertures()) {
		if (aperture.identifier() == identifier) return &aperture;
	}
	return nullptr;
}

std::shared_ptr<const McsMv22GeneratedMeshCacheEntry> generateMeshSet(
	const std::string &variant_identifier)
{
	auto entry = std::make_shared<McsMv22GeneratedMeshCacheEntry>();
	const ModernCarSemanticFamily base_family =
		ModernCarSemanticCatalogFactory().createFamily();
	const McsMv21SemanticFamilyDefinition semantic_family =
		McsMv21SemanticCatalogFactory().createFamilyDefinition();
	const McsMv22KinematicFamilyDefinition kinematic_family =
		McsMv22KinematicCatalogFactory().createFamilyDefinition();
	const ModernCarSemanticVariant *base_variant =
		findBaseVariant(base_family, variant_identifier);
	const McsMv21SemanticVariantDefinition *semantic_variant =
		findSemanticVariant(semantic_family, variant_identifier);
	const McsMv22KinematicVariantDefinition *kinematic_variant =
		findKinematicVariant(kinematic_family, variant_identifier);
	if (base_variant == nullptr || semantic_variant == nullptr ||
	    kinematic_variant == nullptr) {
		entry->diagnostic = "MCSMv2.2 catalogs do not contain variant '" +
		                    variant_identifier + "'.";
		return entry;
	}
	const ParametricModelGenerationPolicy body_policy(
		"MCSMv2.2.GeneratedMesh.FinalBody", ParametricGenerationResolution::Low,
		52u, 32u, 32u, 0.0, true);
	const ParametricModelGenerationPolicy registration_policy(
		"MCSMv2.2.GeneratedMesh.Registration", ParametricGenerationResolution::Low,
		113u, 65u, 81u, 0.0, true);
	const GeneratedMcsMv21SemanticGeometryResult semantic_result =
		McsMv21SemanticGeometryGenerationService().generate(
			*base_variant, *semantic_variant, body_policy, registration_policy, 201u);
	if (!semantic_result.succeeded() || !semantic_result.generatedGeometry()) {
		entry->diagnostic = "Inherited MCSMv2.1 semantic geometry generation failed.";
		return entry;
	}
	auto semantic_geometry = std::make_shared<McsMv21SemanticGeometry>(
		*semantic_result.generatedGeometry());
	const std::filesystem::path release_root = locateRepositoryResource(
		"examples/MVPv2_5_Red_AWD_Hatchback/MCSMv2_2_0_RELEASE");
	const std::filesystem::path partition_root = locateRepositoryResource(
		"examples/MVPv2_5_Red_AWD_Hatchback/MCSMv2_2_0_NATIVE_EVIDENCE/partition_meshes");
	McsMv22NativeKinematicEvidenceGeometry evidence_geometry =
		McsMv22NativeKinematicEvidenceLoadingService().load(
			release_root, partition_root, variant_identifier,
			kinematic_family.sourceRelease().toProgen3dMatrix());
	const GeneratedMcsMv22KinematicGeometryResult kinematic_result =
		McsMv22KinematicGeometryGenerationService().generate(
			semantic_geometry, kinematic_family.sourceRelease(), *kinematic_variant,
			evidence_geometry);
	if (!kinematic_result.succeeded() || !kinematic_result.generatedGeometry()) {
		entry->diagnostic = kinematic_result.diagnostic().empty()
			                    ? "MCSMv2.2 kinematic geometry generation failed."
			                    : kinematic_result.diagnostic();
		return entry;
	}

	const McsMv21SemanticGeometry &semantic =
		*kinematic_result.generatedGeometry()->semanticGeometry();
	const VehicleSurfaceOwnership &ownership =
		semantic.domainPartition().ownership();
	std::set<std::string> closure_identifiers;
	for (const McsMv22ClosureHingeDefinition &hinge : kinematic_variant->hinges()) {
		closure_identifiers.insert(hinge.closureIdentifier());
	}
	entry->meshes.emplace(
		McsMv22MeshComponent::FixedBody,
		extractOwnedMesh(
			*semantic.finalBodyMesh().mesh(), ownership,
			[&closure_identifiers](const std::string &owner) {
				return owner.rfind("aperture:", 0u) != 0u &&
				       closure_identifiers.count(owner) == 0u;
			}));

	auto closures_closed = std::make_shared<Mesh>();
	auto closures_open = std::make_shared<Mesh>();
	const VehicleClosureSweepSet closure_sweeps =
		McsMv22ClosureKinematicEvaluationService().createClosureSweepSet(
			*kinematic_variant);
	for (const VehicleClosureSweep &sweep : closure_sweeps.sweeps()) {
		const GeneratedPrimitiveMesh closure_mesh = extractOwnedMesh(
			*semantic.finalBodyMesh().mesh(), ownership,
			[&sweep](const std::string &owner) {
				return owner == sweep.surfaceBinding().closureIdentifier();
			});
		closures_closed->add(*closure_mesh.mesh());
		const glm::dmat4 target_transform =
			kinematic_family.sourceRelease().toProgen3dMatrix() *
			sweep.poseSamples().back().sourceTransform() *
			glm::inverse(kinematic_family.sourceRelease().toProgen3dMatrix());
		addTransformedMesh(closures_open.get(), closure_mesh, target_transform);
	}
	entry->meshes.emplace(
		McsMv22MeshComponent::ClosuresClosed,
		createGeneratedMesh(std::move(closures_closed)));
	entry->meshes.emplace(
		McsMv22MeshComponent::ClosuresOpen,
		createGeneratedMesh(std::move(closures_open)));

	auto glass_closed = std::make_shared<Mesh>();
	auto glass_open = std::make_shared<Mesh>();
	const VehicleGlassMotionSet glass_motions =
		McsMv22ClosureKinematicEvaluationService().createGlassMotionSet(
			*kinematic_variant, 1.0);
	for (const VehicleGlassMotion &motion : glass_motions.motions()) {
		const VehicleGlassAperture *aperture = findGlassAperture(
			semantic, motion.surfaceBinding().apertureDomainIdentifier());
		if (aperture == nullptr) {
			entry->diagnostic = "MCSMv2.2 glass aperture was not generated: " +
			                    motion.surfaceBinding().apertureDomainIdentifier();
			return entry;
		}
		glass_closed->add(*aperture->glassMesh().mesh());
		const glm::dmat4 target_transform =
			kinematic_family.sourceRelease().toProgen3dMatrix() *
			motion.poseSamples().back().worldSourceTransform() *
			glm::inverse(kinematic_family.sourceRelease().toProgen3dMatrix());
		addTransformedMesh(glass_open.get(), aperture->glassMesh(), target_transform);
	}
	entry->meshes.emplace(
		McsMv22MeshComponent::GlassClosed,
		createGeneratedMesh(std::move(glass_closed)));
	entry->meshes.emplace(
		McsMv22MeshComponent::GlassOpen,
		createGeneratedMesh(std::move(glass_open)));

	auto suspension = std::make_shared<Mesh>();
	for (const McsMv22SuspensionHardpointDefinition &hardpoint :
	     kinematic_variant->hardpoints()) {
		const glm::dvec3 target_position = glm::dvec3(
			kinematic_family.sourceRelease().toProgen3dMatrix() *
			glm::dvec4(hardpoint.sourcePosition(), 1.0));
		addOctahedron(suspension.get(), target_position, 0.025);
	}
	entry->meshes.emplace(
		McsMv22MeshComponent::Suspension,
		createGeneratedMesh(std::move(suspension)));

	const GeneratedPrimitiveMesh source_tyre =
		VehicleTyreMeshGenerationService().generateSourceFrameTyre(
			VehicleTyreMeshSpecification(
				kinematic_variant->wheelRadiusMetres(),
				kinematic_variant->wheelWidthMetres()));
	auto nominal_tyres = std::make_shared<Mesh>();
	auto tyre_sweeps = std::make_shared<Mesh>();
	for (const McsMv22WheelPoseEvidence &pose :
	     kinematic_variant->acceptedWheelPoses()) {
		const glm::dmat4 direct_target_transform =
			kinematic_family.sourceRelease().toProgen3dMatrix() *
			pose.sourceTransform();
		addTransformedMesh(tyre_sweeps.get(), source_tyre, direct_target_transform);
	}
	const VehicleSuspensionKinematicEvaluationService suspension_service;
	for (const VehicleSuspensionCornerKinematicModel &corner :
	     kinematic_result.generatedGeometry()->suspensionCorners()) {
		const VehicleWheelPoseEvaluation nominal_pose =
			suspension_service.evaluateWheelPose(
				corner.corner(), corner.nominalSourceHubCentre(),
				corner.sourceTrackWidthMetres(), corner.wheelRadiusMetres(), 0.0, 0.0,
				corner.solutionPolicy());
		addTransformedMesh(
			nominal_tyres.get(), source_tyre,
			kinematic_family.sourceRelease().toProgen3dMatrix() *
				nominal_pose.sourceTransform());
	}
	entry->meshes.emplace(
		McsMv22MeshComponent::NominalTyres,
		createGeneratedMesh(std::move(nominal_tyres)));
	entry->meshes.emplace(
		McsMv22MeshComponent::TyreSweeps,
		createGeneratedMesh(std::move(tyre_sweeps)));
	return entry;
}

std::shared_ptr<const McsMv22GeneratedMeshCacheEntry> cachedMeshSet(
	const std::string &variant_identifier)
{
	static std::mutex cache_mutex;
	static std::map<std::string, std::shared_ptr<const McsMv22GeneratedMeshCacheEntry>> cache;
	std::lock_guard<std::mutex> lock(cache_mutex);
	const auto existing = cache.find(variant_identifier);
	if (existing != cache.end()) return existing->second;
	const auto generated = generateMeshSet(variant_identifier);
	cache.emplace(variant_identifier, generated);
	return generated;
}

} // namespace

bool McsMv22GeneratedMeshProvider::recognizes(const std::string &mesh_key) const
{
	return parseMeshKey(mesh_key).has_value();
}

GeneratedMeshResolutionResult McsMv22GeneratedMeshProvider::resolve(
	const std::string &mesh_key,
	GeometryDetailLevel detail_level) const
{
	(void)detail_level;
	const std::optional<McsMv22MeshRequest> request = parseMeshKey(mesh_key);
	if (!request) {
		return GeneratedMeshResolutionResult::createFailure(
			"MCSMv2.2 generated mesh provider does not recognize key '" +
			mesh_key + "'.");
	}
	const auto entry = cachedMeshSet(request->variant_identifier);
	const auto selected = entry->meshes.find(request->component);
	if (selected == entry->meshes.end() || !selected->second.mesh()) {
		return GeneratedMeshResolutionResult::createFailure(
			entry->diagnostic.empty()
				? "MCSMv2.2 generated mesh set did not provide key '" + mesh_key + "'."
				: entry->diagnostic);
	}
	if (selected->second.mesh()->vertices.size() >= (1u << 16u)) {
		return GeneratedMeshResolutionResult::createFailure(
			"MCSMv2.2 generated mesh exceeds the editor 16-bit vertex limit.");
	}
	return GeneratedMeshResolutionResult::createSuccess(selected->second);
}
