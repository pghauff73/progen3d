#include "vegetation/service/PlantMeshGenerator.h"

#include "vegetation/model/PlantShapeSpecification.h"
#include "vegetation/model/PlantGrowthRequest.h"
#include "vegetation/service/PlantGrowthService.h"
#include "vegetation/service/PlantTopologyGenerationService.h"
#include "vegetation/service/VegetationGeometryAssemblyService.h"

#include <memory>

GeneratedPrimitiveMesh PlantMeshGenerator::generate(
	const ShapeSpecification &specification,
	std::string *diagnostic) const
{
	const auto *plant = dynamic_cast<const PlantShapeSpecification *>(&specification);
	if (plant == nullptr) {
		if (diagnostic != nullptr) {
			*diagnostic = "Plant mesh generation requires a PlantShapeSpecification.";
		}
		return GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {});
	}

	const bool explicit_growth = plant->growth().has_value();
	const PlantTopologyGenerationResult topology =
		PlantTopologyGenerationService().generate(
			*plant,
			explicit_growth ? plant->species().matureAge() : plant->age(),
			explicit_growth ? PlantDevelopmentState::Mature
			                : plant->developmentState());
	if (!topology.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = topology.diagnostic();
		return GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {});
	}

	BranchGraph resolved_graph = *topology.graph();
	if (explicit_growth) {
		const PlantGrowthResult growth = PlantGrowthService().resolve(
			PlantGrowthRequest(
				resolved_graph, plant->age(), plant->developmentState(),
				*plant->growth()));
		if (!growth.succeeded() || !growth.snapshot().has_value()) {
			if (diagnostic != nullptr) *diagnostic = growth.diagnostic();
			return GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {});
		}
		resolved_graph = growth.snapshot()->graph();
	}

	const VegetationGeometryBuildResult geometry =
		VegetationGeometryAssemblyService().build(
			resolved_graph, plant->species(), plant->detailLevel(),
			plant->age(), plant->developmentState(),
			plant->deterministicSeed());
	if (!geometry.succeeded() || !geometry.geometry().has_value()) {
		if (diagnostic != nullptr) *diagnostic = geometry.diagnostic();
		return GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {});
	}
	return geometry.geometry()->combinedPreviewMesh();
}
