#include "vehicle/mcsmv2/service/McsMv2GeneratedMeshProvider.h"

#include "geometry/model/MeshSurfaceTag.h"
#include "geometry/service/GeneratedMeshProviderRegistry.h"
#include "vehicle/mcsmv2/model/ModernCarSemanticFamily.h"
#include "vehicle/mcsmv2/service/ModernCarSemanticCatalogFactory.h"
#include "vehicle/mcsmv2/service/ModernCarSemanticIsoSurfaceGenerationService.h"

#include <memory>
#include <string>
#include <vector>

namespace {

class McsMv2GeneratedMeshProviderRegistration
{
public:
	McsMv2GeneratedMeshProviderRegistration()
	{
		GeneratedMeshProviderRegistry::accessProcessRegistry().registerProvider(
			std::make_shared<McsMv2GeneratedMeshProvider>());
	}
};

const McsMv2GeneratedMeshProviderRegistration
	mcsmv2_generated_mesh_provider_registration;

const char *variantIdentifierForMeshKey(const std::string &mesh_key)
{
	if (mesh_key == "MCSMv2ReferenceBodyMesh") return "reference";
	if (mesh_key == "MCSMv2TrackBodyMesh") return "track";
	if (mesh_key == "MCSMv2AeroBodyMesh") return "aero";
	if (mesh_key == "MCSMv2CrossoverBodyMesh") return "crossover";
	return nullptr;
}

ParametricModelGenerationPolicy generationPolicyForDetailLevel(
	GeometryDetailLevel detail_level)
{
	if (detail_level == GeometryDetailLevel::Bounds) {
		return ParametricModelGenerationPolicy(
			"MCSMv2.GeneratedMesh.Bounds",
			ParametricGenerationResolution::Low,
			32u,
			20u,
			20u,
			0.0,
			true);
	}
	if (detail_level == GeometryDetailLevel::CoarseShape) {
		return ParametricModelGenerationPolicy(
			"MCSMv2.GeneratedMesh.CoarseShape",
			ParametricGenerationResolution::Low,
			44u,
			28u,
			28u,
			0.0,
			true);
	}
	return ParametricModelGenerationPolicy(
		"MCSMv2.GeneratedMesh.EditorSafe",
		ParametricGenerationResolution::Medium,
		52u,
		32u,
		32u,
		0.0,
		true);
}

const ModernCarSemanticVariant *findVariant(
	const ModernCarSemanticFamily &family,
	const std::string &identifier)
{
	for (const ModernCarSemanticVariant &variant : family.variants()) {
		if (variant.identifier() == identifier) return &variant;
	}
	return nullptr;
}

} // namespace

bool McsMv2GeneratedMeshProvider::recognizes(const std::string &mesh_key) const
{
	return variantIdentifierForMeshKey(mesh_key) != nullptr;
}

GeneratedMeshResolutionResult McsMv2GeneratedMeshProvider::resolve(
	const std::string &mesh_key,
	GeometryDetailLevel detail_level) const
{
	const char *variant_identifier = variantIdentifierForMeshKey(mesh_key);
	if (variant_identifier == nullptr) {
		return GeneratedMeshResolutionResult::createFailure(
			"MCSMv2 generated mesh provider does not recognize key '" +
			mesh_key + "'.");
	}

	const ModernCarSemanticFamily family =
		ModernCarSemanticCatalogFactory().createFamily();
	const ModernCarSemanticVariant *variant =
		findVariant(family, variant_identifier);
	if (variant == nullptr) {
		return GeneratedMeshResolutionResult::createFailure(
			"MCSMv2 semantic catalog is missing variant '" +
			std::string(variant_identifier) + "'.");
	}

	const GeneratedModernCarSemanticBodyResult result =
		ModernCarSemanticIsoSurfaceGenerationService().generate(
			*variant,
			generationPolicyForDetailLevel(detail_level));
	if (!result.succeeded() || !result.generatedBody().has_value()) {
		std::string diagnostic =
			"MCSMv2 generated mesh provider failed to generate key '" +
			mesh_key + "'.";
		if (!result.validationReport().issues().empty()) {
			diagnostic += " " + result.validationReport().issues().front().message();
		}
		return GeneratedMeshResolutionResult::createFailure(std::move(diagnostic));
	}

	const GeneratedBodyMesh &body_mesh =
		result.generatedBody()->bodyMesh();
	if (!body_mesh.mesh()) {
		return GeneratedMeshResolutionResult::createFailure(
			"MCSMv2 generated mesh provider returned an empty body mesh.");
	}
	if (body_mesh.mesh()->vertices.size() >= (1u << 16u)) {
		return GeneratedMeshResolutionResult::createFailure(
			"MCSMv2 generated mesh exceeds the editor 16-bit vertex limit.");
	}

	std::vector<MeshSurfaceTag> surface_tags(
		body_mesh.mesh()->faces.size(), MeshSurfaceTag(MeshSurfaceRole::Outer));
	return GeneratedMeshResolutionResult::createSuccess(
		GeneratedPrimitiveMesh(
			std::make_shared<Mesh>(*body_mesh.mesh()),
			std::move(surface_tags)));
}
