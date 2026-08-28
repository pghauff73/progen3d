#include "vehicle/mcsmv2/service/McsMv21GeneratedMeshProvider.h"

#include "geometry/service/GeneratedMeshProviderRegistry.h"
#include "vehicle/mcsmv2/service/McsMv21SemanticCatalogFactory.h"
#include "vehicle/mcsmv2/service/McsMv21SemanticGeometryGenerationService.h"
#include "vehicle/mcsmv2/service/ModernCarSemanticCatalogFactory.h"

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

class McsMv21GeneratedMeshProviderRegistration
{
public:
	McsMv21GeneratedMeshProviderRegistration()
	{
		GeneratedMeshProviderRegistry::accessProcessRegistry().registerProvider(
			std::make_shared<McsMv21GeneratedMeshProvider>());
	}
};

const McsMv21GeneratedMeshProviderRegistration
	mcsmv21_generated_mesh_provider_registration;

enum class McsMv21MeshComponent
{
	FinalBody,
	ExteriorBody,
	FixedBody,
	PanelRegions,
	Glass
};

struct McsMv21MeshRequest
{
	std::string variant_identifier;
	McsMv21MeshComponent component = McsMv21MeshComponent::FinalBody;
};

struct McsMv21GeneratedMeshCacheEntry
{
	std::optional<GeneratedPrimitiveMesh> final_body;
	std::optional<GeneratedPrimitiveMesh> exterior_body;
	std::optional<GeneratedPrimitiveMesh> fixed_body;
	std::optional<GeneratedPrimitiveMesh> panel_regions;
	std::optional<GeneratedPrimitiveMesh> glass;
	std::string diagnostic;
};

std::optional<McsMv21MeshRequest> parseMeshKey(const std::string &mesh_key)
{
	const std::pair<const char *, const char *> variants[] = {
		{"Reference", "reference"},
		{"Track", "track"},
		{"Aero", "aero"},
		{"Crossover", "crossover"}};
	const std::pair<const char *, McsMv21MeshComponent> components[] = {
		{"BodyMesh", McsMv21MeshComponent::FinalBody},
		{"ExteriorBodyMesh", McsMv21MeshComponent::ExteriorBody},
		{"FixedBodyMesh", McsMv21MeshComponent::FixedBody},
		{"PanelRegionMesh", McsMv21MeshComponent::PanelRegions},
		{"GlassMesh", McsMv21MeshComponent::Glass}};
	for (const auto &variant : variants) {
		for (const auto &component : components) {
			if (mesh_key ==
			    std::string("MCSMv21") + variant.first + component.first) {
				return McsMv21MeshRequest{variant.second, component.second};
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

GeneratedPrimitiveMesh extractOwnedMesh(
	const Mesh &source_mesh,
	const VehicleSurfaceOwnership &ownership,
	McsMv21MeshComponent component)
{
	auto selected_mesh = std::make_shared<Mesh>();
	std::vector<int> remapped_vertices(source_mesh.vertices.size(), -1);
	for (std::size_t face_index = 0u;
	     face_index < source_mesh.faces.size();
	     ++face_index) {
		const std::string &owner = ownership.faceOwnerIdentifiers()[face_index];
		const bool is_fixed = owner.rfind("fixed_body", 0u) == 0u;
		const bool is_aperture = owner.rfind("aperture:", 0u) == 0u;
		const bool selected =
			(component == McsMv21MeshComponent::FixedBody && is_fixed) ||
			(component == McsMv21MeshComponent::PanelRegions &&
			 !is_fixed && !is_aperture);
		if (!selected) continue;
		const glm::ivec3 source_face = source_mesh.faces[face_index];
		glm::ivec3 selected_face(0);
		const int source_indices[3] = {
			source_face.x, source_face.y, source_face.z};
		for (int corner = 0; corner < 3; ++corner) {
			const int source_index = source_indices[corner];
			int &selected_index =
				remapped_vertices[static_cast<std::size_t>(source_index)];
			if (selected_index < 0) {
				selected_index = static_cast<int>(selected_mesh->vertices.size());
				selected_mesh->vertices.push_back(
					source_mesh.vertices[static_cast<std::size_t>(source_index)]);
			}
			selected_face[corner] = selected_index;
		}
		selected_mesh->faces.push_back(selected_face);
	}
	selected_mesh->calc_normals();
	selected_mesh->buildCollisionAccel();
	return GeneratedPrimitiveMesh(
		selected_mesh,
		std::vector<MeshSurfaceTag>(
			selected_mesh->faces.size(), MeshSurfaceTag(MeshSurfaceRole::Outer)));
}

GeneratedPrimitiveMesh combineGlassMeshes(
	const std::vector<VehicleGlassAperture> &apertures)
{
	auto combined_mesh = std::make_shared<Mesh>();
	for (const VehicleGlassAperture &aperture : apertures) {
		combined_mesh->add(*aperture.glassMesh().mesh());
	}
	combined_mesh->calc_normals();
	combined_mesh->buildCollisionAccel();
	return GeneratedPrimitiveMesh(
		combined_mesh,
		std::vector<MeshSurfaceTag>(
			combined_mesh->faces.size(), MeshSurfaceTag(MeshSurfaceRole::Outer)));
}

std::shared_ptr<const McsMv21GeneratedMeshCacheEntry> generateMeshSet(
	const std::string &variant_identifier)
{
	auto entry = std::make_shared<McsMv21GeneratedMeshCacheEntry>();
	const ModernCarSemanticFamily base_family =
		ModernCarSemanticCatalogFactory().createFamily();
	const McsMv21SemanticFamilyDefinition semantic_family =
		McsMv21SemanticCatalogFactory().createFamilyDefinition();
	const ModernCarSemanticVariant *base_variant =
		findBaseVariant(base_family, variant_identifier);
	const McsMv21SemanticVariantDefinition *semantic_variant =
		findSemanticVariant(semantic_family, variant_identifier);
	if (base_variant == nullptr || semantic_variant == nullptr) {
		entry->diagnostic =
			"MCSMv2.1 catalogs do not contain variant '" +
			variant_identifier + "'.";
		return entry;
	}
	const ParametricModelGenerationPolicy body_policy(
		"MCSMv2.1.GeneratedMesh.FinalBody",
		ParametricGenerationResolution::Low,
		52u,
		32u,
		32u,
		0.0,
		true);
	const ParametricModelGenerationPolicy registration_policy(
		"MCSMv2.1.GeneratedMesh.Registration",
		ParametricGenerationResolution::Low,
		113u,
		65u,
		81u,
		0.0,
		true);
	const GeneratedMcsMv21SemanticGeometryResult result =
		McsMv21SemanticGeometryGenerationService().generate(
			*base_variant,
			*semantic_variant,
			body_policy,
			registration_policy,
			201u);
	if (!result.succeeded() || !result.generatedGeometry()) {
		entry->diagnostic =
			"MCSMv2.1 generated mesh set failed for variant '" +
			variant_identifier + "'.";
		if (!result.validationReport().issues().empty()) {
			entry->diagnostic += " " +
				result.validationReport().issues().front().message();
		}
		return entry;
	}
	const McsMv21SemanticGeometry &geometry = *result.generatedGeometry();
	entry->final_body = geometry.finalBodyMesh();
	entry->exterior_body = geometry.exteriorBodyMesh();
	entry->fixed_body = extractOwnedMesh(
		*geometry.finalBodyMesh().mesh(),
		geometry.domainPartition().ownership(),
		McsMv21MeshComponent::FixedBody);
	entry->panel_regions = extractOwnedMesh(
		*geometry.finalBodyMesh().mesh(),
		geometry.domainPartition().ownership(),
		McsMv21MeshComponent::PanelRegions);
	entry->glass = combineGlassMeshes(geometry.glassApertures());
	return entry;
}

std::shared_ptr<const McsMv21GeneratedMeshCacheEntry> cachedMeshSet(
	const std::string &variant_identifier)
{
	static std::mutex cache_mutex;
	static std::map<
		std::string,
		std::shared_ptr<const McsMv21GeneratedMeshCacheEntry>> cache;
	std::lock_guard<std::mutex> lock(cache_mutex);
	const auto existing = cache.find(variant_identifier);
	if (existing != cache.end()) return existing->second;
	const std::shared_ptr<const McsMv21GeneratedMeshCacheEntry> generated =
		generateMeshSet(variant_identifier);
	cache.emplace(variant_identifier, generated);
	return generated;
}

const std::optional<GeneratedPrimitiveMesh> &selectMesh(
	const McsMv21GeneratedMeshCacheEntry &entry,
	McsMv21MeshComponent component)
{
	switch (component) {
	case McsMv21MeshComponent::FinalBody: return entry.final_body;
	case McsMv21MeshComponent::ExteriorBody: return entry.exterior_body;
	case McsMv21MeshComponent::FixedBody: return entry.fixed_body;
	case McsMv21MeshComponent::PanelRegions: return entry.panel_regions;
	case McsMv21MeshComponent::Glass: return entry.glass;
	}
	return entry.final_body;
}

} // namespace

bool McsMv21GeneratedMeshProvider::recognizes(const std::string &mesh_key) const
{
	return parseMeshKey(mesh_key).has_value();
}

GeneratedMeshResolutionResult McsMv21GeneratedMeshProvider::resolve(
	const std::string &mesh_key,
	GeometryDetailLevel detail_level) const
{
	(void)detail_level;
	const std::optional<McsMv21MeshRequest> request = parseMeshKey(mesh_key);
	if (!request) {
		return GeneratedMeshResolutionResult::createFailure(
			"MCSMv2.1 generated mesh provider does not recognize key '" +
			mesh_key + "'.");
	}
	const std::shared_ptr<const McsMv21GeneratedMeshCacheEntry> entry =
		cachedMeshSet(request->variant_identifier);
	const std::optional<GeneratedPrimitiveMesh> &selected =
		selectMesh(*entry, request->component);
	if (!selected || !selected->mesh()) {
		return GeneratedMeshResolutionResult::createFailure(
			entry->diagnostic.empty()
				? "MCSMv2.1 generated mesh set did not provide key '" +
					mesh_key + "'."
				: entry->diagnostic);
	}
	if (selected->mesh()->vertices.size() >= (1u << 16u)) {
		return GeneratedMeshResolutionResult::createFailure(
			"MCSMv2.1 generated mesh exceeds the editor 16-bit vertex limit.");
	}
	return GeneratedMeshResolutionResult::createSuccess(*selected);
}
