#pragma once

#include "geometry/model/ResolvedPrimitiveGeometry.h"
#include "geometry/model/ShapeSpecification.h"
#include "geometry/model/ShapeSpecificationKey.h"

#include <memory>
#include <string>
#include <unordered_map>

class AxialProfileMeshGenerator;
class BotanicalBladeMeshGenerator;
class BranchJunctionMeshGenerator;
class CylinderMeshGenerator;
class CurvedPanelMeshGenerator;
class CurveNetworkSurfaceMeshGenerator;
class ExtrudeProfileMeshGenerator;
class FoldedProfileMeshGenerator;
class GeneratedMeshReferenceResolutionService;
class LoftMeshGenerator;
class PlantMeshGenerator;
class ProceduralGeometryEvidenceService;
class RevolveMeshGenerator;
class ScatterRegionMeshGenerator;
class SphereMeshGenerator;
class SweepDiskMeshGenerator;
class SweepProfileMeshGenerator;
class TaperedSweepMeshGenerator;
class VariableSectionSweepMeshGenerator;
class VineMeshGenerator;

class ProceduralMeshRepository
{
public:
	ProceduralMeshRepository();
	~ProceduralMeshRepository();

	std::shared_ptr<const ResolvedPrimitiveGeometry> resolve(
		const ShapeSpecification &specification,
		std::string *diagnostic);

	std::size_t cachedShapeCount() const;

private:
	std::unique_ptr<AxialProfileMeshGenerator> axial_profile_mesh_generator_;
	std::unique_ptr<BotanicalBladeMeshGenerator> botanical_blade_mesh_generator_;
	std::unique_ptr<BranchJunctionMeshGenerator> branch_junction_mesh_generator_;
	std::unique_ptr<CylinderMeshGenerator> cylinder_mesh_generator_;
	std::unique_ptr<CurvedPanelMeshGenerator> curved_panel_mesh_generator_;
	std::unique_ptr<CurveNetworkSurfaceMeshGenerator>
		curve_network_surface_mesh_generator_;
	std::unique_ptr<ExtrudeProfileMeshGenerator> extrude_profile_mesh_generator_;
	std::unique_ptr<FoldedProfileMeshGenerator> folded_profile_mesh_generator_;
	std::unique_ptr<GeneratedMeshReferenceResolutionService>
		generated_mesh_reference_resolution_service_;
	std::unique_ptr<LoftMeshGenerator> loft_mesh_generator_;
	std::unique_ptr<PlantMeshGenerator> plant_mesh_generator_;
	std::unique_ptr<RevolveMeshGenerator> revolve_mesh_generator_;
	std::unique_ptr<ScatterRegionMeshGenerator> scatter_region_mesh_generator_;
	std::unique_ptr<SphereMeshGenerator> sphere_mesh_generator_;
	std::unique_ptr<SweepDiskMeshGenerator> sweep_disk_mesh_generator_;
	std::unique_ptr<SweepProfileMeshGenerator> sweep_profile_mesh_generator_;
	std::unique_ptr<TaperedSweepMeshGenerator> tapered_sweep_mesh_generator_;
	std::unique_ptr<VariableSectionSweepMeshGenerator>
		variable_section_sweep_mesh_generator_;
	std::unique_ptr<VineMeshGenerator> vine_mesh_generator_;
	std::unique_ptr<ProceduralGeometryEvidenceService> geometry_evidence_service_;
	std::unordered_map<ShapeSpecificationKey,
	                   std::shared_ptr<const ResolvedPrimitiveGeometry>,
	                   ShapeSpecificationKeyHash> resolved_shapes_;
};
