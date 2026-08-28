#include "vegetation/service/PlantTopologyGenerationService.h"

#include "vegetation/model/RuleBranchingGenerationRequest.h"
#include "vegetation/model/SpaceColonizationRequest.h"
#include "vegetation/service/BranchGraphDeterministicHashService.h"
#include "vegetation/service/BranchGraphValidationService.h"
#include "vegetation/service/LSystemBranchGraphGenerationService.h"
#include "vegetation/service/PlantOrganAttachmentService.h"
#include "vegetation/service/RuleBranchingGenerationService.h"
#include "vegetation/service/SpaceColonizationSeedGraphFactory.h"
#include "vegetation/service/SpaceColonizationService.h"

#include <optional>
#include <string>

PlantTopologyGenerationResult PlantTopologyGenerationService::generate(
	const PlantShapeSpecification &plant,
	float topology_age,
	PlantDevelopmentState topology_state) const
{
	if (plant.topologyGeneration().method() ==
	    PlantTopologyGenerationMethod::RuleBranching) {
		const RuleBranchingGenerationResult topology =
			RuleBranchingGenerationService(complexity_limits_).generate(
				RuleBranchingGenerationRequest(
					plant.species(), topology_age, topology_state,
					plant.deterministicSeed(), plant.tropismInfluences()));
		if (!topology.succeeded()) {
			return PlantTopologyGenerationResult::failed(topology.diagnostic());
		}
		return PlantTopologyGenerationResult::succeeded(
			*topology.graph(), topology.topologyHash());
	}
	if (plant.topologyGeneration().method() ==
	    PlantTopologyGenerationMethod::LSystem) {
		if (!plant.topologyGeneration().lSystem().has_value()) {
			return PlantTopologyGenerationResult::failed(
				"Plant LSystem generation is missing its topology specification.");
		}
		const LSystemBranchGraphGenerationResult topology =
			LSystemBranchGraphGenerationService(complexity_limits_).generate(
				*plant.topologyGeneration().lSystem(), topology_age,
				topology_state, plant.tropismInfluences());
		if (!topology.succeeded() || !topology.snapshot().has_value()) {
			return PlantTopologyGenerationResult::failed(topology.diagnostic());
		}
		std::string diagnostic;
		const std::optional<BranchGraph> organ_graph =
			PlantOrganAttachmentService(complexity_limits_).attachLeaves(
				topology.snapshot()->graph(), plant.species(), topology_age,
				&diagnostic);
		if (!organ_graph.has_value()) {
			return PlantTopologyGenerationResult::failed(diagnostic);
		}
		const BranchGraphValidationReport validation =
			BranchGraphValidationService(complexity_limits_).validate(*organ_graph);
		if (!validation.succeeded()) {
			return PlantTopologyGenerationResult::failed(
				"Plant LSystem organ attachment produced an invalid graph: " +
				validation.issues().front().diagnostic());
		}
		return PlantTopologyGenerationResult::succeeded(
			*organ_graph, BranchGraphDeterministicHashService().hash(*organ_graph));
	}

	if (!plant.topologyGeneration().spaceColonization().has_value()) {
		return PlantTopologyGenerationResult::failed(
			"Plant SpaceColonization generation is missing its topology specification.");
	}
	const PlantSpaceColonizationSpecification &space_colonization =
		*plant.topologyGeneration().spaceColonization();
	std::string diagnostic;
	const std::optional<BranchGraph> seed_graph =
		SpaceColonizationSeedGraphFactory().create(
			plant.species(), space_colonization, &diagnostic);
	if (!seed_graph.has_value()) {
		return PlantTopologyGenerationResult::failed(diagnostic);
	}
	const SpaceColonizationResult colonization =
		SpaceColonizationService(complexity_limits_).resolve(
			SpaceColonizationRequest(
				*seed_graph, space_colonization.crownVolume(),
				space_colonization.colonization(), plant.deterministicSeed(),
				space_colonization.obstacles()));
	if (!colonization.succeeded() || !colonization.snapshot().has_value()) {
		return PlantTopologyGenerationResult::failed(colonization.diagnostic());
	}
	const std::optional<BranchGraph> organ_graph =
		PlantOrganAttachmentService(complexity_limits_).attachLeaves(
			colonization.snapshot()->graph(), plant.species(), topology_age,
			&diagnostic);
	if (!organ_graph.has_value()) {
		return PlantTopologyGenerationResult::failed(diagnostic);
	}
	const BranchGraphValidationReport validation =
		BranchGraphValidationService(complexity_limits_).validate(*organ_graph);
	if (!validation.succeeded()) {
		return PlantTopologyGenerationResult::failed(
			"Plant organ attachment produced an invalid graph: " +
			validation.issues().front().diagnostic());
	}
	return PlantTopologyGenerationResult::succeeded(
		*organ_graph, BranchGraphDeterministicHashService().hash(*organ_graph));
}
