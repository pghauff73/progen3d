#include "vegetation/service/ScatterRegionMeshGenerator.h"

#include "geometry/model/InstanceArraySpecification.h"
#include "geometry/service/InstanceArrayGeometryBuilder.h"
#include "vegetation/model/PlantDevelopmentState.h"
#include "vegetation/model/PlantShapeSpecification.h"
#include "vegetation/model/ScatterRegionRequest.h"
#include "vegetation/model/ScatterRegionShapeSpecification.h"
#include "vegetation/service/PlantMeshGenerator.h"
#include "vegetation/service/PlantShapeSpecificationValidator.h"
#include "vegetation/service/ScatterRegionService.h"

#include <memory>
#include <string>
#include <vector>

namespace {

GeneratedPrimitiveMesh failed_mesh(const std::string &message, std::string *diagnostic)
{
	if (diagnostic != nullptr) *diagnostic = message;
	return GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {});
}

} // namespace

GeneratedPrimitiveMesh ScatterRegionMeshGenerator::generate(
	const ShapeSpecification &specification,
	std::string *diagnostic) const
{
	const auto *scatter =
		dynamic_cast<const ScatterRegionShapeSpecification *>(&specification);
	if (scatter == nullptr) {
		return failed_mesh(
			"ScatterRegion mesh generation requires a ScatterRegionShapeSpecification.",
			diagnostic);
	}
	const ScatterRegionResult placements = ScatterRegionService().resolve(
		ScatterRegionRequest(
			scatter->scatter(), scatter->deterministicSeed(),
			scatter->obstacles()));
	if (!placements.succeeded() || !placements.snapshot().has_value()) {
		return failed_mesh(placements.diagnostic(), diagnostic);
	}

	PlantShapeSpecificationCandidate plant_candidate;
	plant_candidate.species = scatter->species();
	plant_candidate.age = scatter->species().matureAge();
	plant_candidate.development_state = PlantDevelopmentState::Mature;
	plant_candidate.deterministic_seed = scatter->deterministicSeed();
	plant_candidate.detail_level = scatter->detailLevel();
	std::string plant_diagnostic;
	const std::shared_ptr<const PlantShapeSpecification> plant =
		PlantShapeSpecificationValidator().validate(
			std::move(plant_candidate), &plant_diagnostic);
	if (!plant) return failed_mesh(plant_diagnostic, diagnostic);
	const GeneratedPrimitiveMesh source =
		PlantMeshGenerator().generate(*plant, &plant_diagnostic);
	if (!source.mesh() || source.mesh()->faces.empty()) {
		return failed_mesh(plant_diagnostic, diagnostic);
	}

	std::vector<glm::mat4> transforms;
	transforms.reserve(placements.snapshot()->placements().size());
	for (const ScatterPlacement &placement :
	     placements.snapshot()->placements()) {
		transforms.push_back(placement.localTransform());
	}
	const GeometryBuildResult instanced = InstanceArrayGeometryBuilder().build(
		source, InstanceArraySpecification(std::move(transforms)));
	if (!instanced.succeeded()) {
		return failed_mesh(instanced.firstDiagnostic(), diagnostic);
	}
	if (diagnostic != nullptr) diagnostic->clear();
	return instanced.generatedMesh();
}
