#!/usr/bin/env bash

source "$repository_root/tests/mcsmv2_native_link_sources.sh"

mcsmv21_native_link_sources=(
	"${mcsmv2_native_link_sources[@]}"
	"$repository_root/src/geometry/service/SimplePolygonValidator.cpp"
	"$repository_root/src/geometry/service/ProfileCapTriangulator.cpp"
	"$repository_root/src/geometry/service/LoftMeshGenerator.cpp"
	"$repository_root/src/vehicle/mcsmv2/service/VehicleSemanticSurfaceGenerationService.cpp"
	"$repository_root/src/vehicle/mcsmv2/service/McsMv21SemanticCatalogFactory.cpp"
	"$repository_root/src/vehicle/mcsmv2/service/VehicleSurfaceProjectionService.cpp"
	"$repository_root/src/vehicle/mcsmv2/service/SemanticImplicitAgreementEvaluationService.cpp"
	"$repository_root/src/vehicle/mcsmv2/service/VehicleSemanticSurfaceRegistrationService.cpp"
	"$repository_root/src/vehicle/mcsmv2/service/VehicleSurfaceDomainEvaluationService.cpp"
	"$repository_root/src/vehicle/mcsmv2/service/VehicleSurfaceDomainPartitionService.cpp"
	"$repository_root/src/vehicle/mcsmv2/service/VehicleGlassApertureConstructionService.cpp"
	"$repository_root/src/vehicle/mcsmv2/service/McsMv21SemanticGeometryGenerationService.cpp"
	"$repository_root/src/vehicle/mcsmv2/service/McsMv21GeneratedMeshProvider.cpp"
	"$repository_root/src/vehicle/mcsmv2/service/McsMv21SemanticGrammarLoweringService.cpp"
)
