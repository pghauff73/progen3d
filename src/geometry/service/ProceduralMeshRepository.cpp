#include "geometry/service/ProceduralMeshRepository.h"

#include "geometry/model/CylinderShapeSpecification.h"
#include "geometry/model/CompoundShapeSpecification.h"
#include "geometry/model/FormedPanelShapeSpecification.h"
#include "geometry/model/GeneratedMeshReferenceShapeSpecification.h"
#include "geometry/model/InstanceArrayShapeSpecification.h"
#include "geometry/model/MirrorShapeSpecification.h"
#include "geometry/model/ShellOffsetShapeSpecification.h"
#include "geometry/model/SphereShapeSpecification.h"
#include "geometry/service/AxialProfileMeshGenerator.h"
#include "geometry/service/BotanicalBladeMeshGenerator.h"
#include "geometry/service/BranchJunctionMeshGenerator.h"
#include "geometry/service/CylinderMeshGenerator.h"
#include "geometry/service/CurvedPanelMeshGenerator.h"
#include "geometry/service/CurveNetworkSurfaceMeshGenerator.h"
#include "geometry/service/ExtrudeProfileMeshGenerator.h"
#include "geometry/service/FoldedProfileMeshGenerator.h"
#include "geometry/service/GeneratedMeshComposer.h"
#include "geometry/service/GeneratedMeshReferenceResolutionService.h"
#include "geometry/service/InstanceArrayGeometryBuilder.h"
#include "geometry/service/LoftMeshGenerator.h"
#include "geometry/service/MeshTopologyAnalyzer.h"
#include "geometry/service/MirroredShapeGeometryBuilder.h"
#include "geometry/service/ProceduralGeometryEvidenceService.h"
#include "geometry/service/RevolveMeshGenerator.h"
#include "geometry/service/ShellOffsetGeometryBuilder.h"
#include "geometry/service/SphereMeshGenerator.h"
#include "geometry/service/SweepDiskMeshGenerator.h"
#include "geometry/service/SweepProfileMeshGenerator.h"
#include "geometry/service/TaperedSweepMeshGenerator.h"
#include "geometry/service/VariableSectionSweepMeshGenerator.h"
#include "vegetation/service/PlantMeshGenerator.h"
#include "vegetation/service/ScatterRegionMeshGenerator.h"
#include "vegetation/service/VineMeshGenerator.h"

#include <utility>

namespace {

ShapeSpecificationKey cache_key_for(const ShapeSpecification &specification)
{
	return ShapeSpecificationKey(
		specification.key().canonicalValue() + "|detail=" +
		std::to_string(geometryDetailLevelRank(specification.detailLevel())));
}

}

ProceduralMeshRepository::ProceduralMeshRepository()
	: axial_profile_mesh_generator_(std::make_unique<AxialProfileMeshGenerator>()),
	  botanical_blade_mesh_generator_(std::make_unique<BotanicalBladeMeshGenerator>()),
	  branch_junction_mesh_generator_(std::make_unique<BranchJunctionMeshGenerator>()),
	  cylinder_mesh_generator_(std::make_unique<CylinderMeshGenerator>()),
	  curved_panel_mesh_generator_(std::make_unique<CurvedPanelMeshGenerator>()),
	  curve_network_surface_mesh_generator_(
		  std::make_unique<CurveNetworkSurfaceMeshGenerator>()),
	  extrude_profile_mesh_generator_(
		  std::make_unique<ExtrudeProfileMeshGenerator>()),
	  folded_profile_mesh_generator_(std::make_unique<FoldedProfileMeshGenerator>()),
	  generated_mesh_reference_resolution_service_(
		  std::make_unique<GeneratedMeshReferenceResolutionService>()),
	  loft_mesh_generator_(std::make_unique<LoftMeshGenerator>()),
	  plant_mesh_generator_(std::make_unique<PlantMeshGenerator>()),
	  revolve_mesh_generator_(std::make_unique<RevolveMeshGenerator>()),
	  scatter_region_mesh_generator_(
		  std::make_unique<ScatterRegionMeshGenerator>()),
	  sphere_mesh_generator_(std::make_unique<SphereMeshGenerator>()),
	  sweep_disk_mesh_generator_(std::make_unique<SweepDiskMeshGenerator>()),
	  sweep_profile_mesh_generator_(std::make_unique<SweepProfileMeshGenerator>()),
	  tapered_sweep_mesh_generator_(std::make_unique<TaperedSweepMeshGenerator>()),
	  variable_section_sweep_mesh_generator_(
		  std::make_unique<VariableSectionSweepMeshGenerator>()),
	  vine_mesh_generator_(std::make_unique<VineMeshGenerator>()),
	  geometry_evidence_service_(
		  std::make_unique<ProceduralGeometryEvidenceService>())
{
}

ProceduralMeshRepository::~ProceduralMeshRepository() = default;

std::shared_ptr<const ResolvedPrimitiveGeometry>
ProceduralMeshRepository::resolve(
	const ShapeSpecification &specification,
	std::string *diagnostic)
{
	const ShapeSpecificationKey cache_key = cache_key_for(specification);
	const auto existing = resolved_shapes_.find(cache_key);
	if (existing != resolved_shapes_.end()) return existing->second;

	GeneratedPrimitiveMesh generated(std::make_shared<Mesh>(), {});
	switch (specification.family()) {
	case ShapeFamily::Cylinder:
		generated = cylinder_mesh_generator_->generate(specification, diagnostic);
		break;
	case ShapeFamily::Sphere:
		generated = sphere_mesh_generator_->generate(specification, diagnostic);
		break;
	case ShapeFamily::AxialProfile:
		generated = axial_profile_mesh_generator_->generate(specification, diagnostic);
		break;
	case ShapeFamily::ExtrudeProfile:
		generated = extrude_profile_mesh_generator_->generate(specification, diagnostic);
		break;
	case ShapeFamily::SweepProfile:
	case ShapeFamily::PanelCut:
		generated = sweep_profile_mesh_generator_->generate(specification, diagnostic);
		break;
	case ShapeFamily::VariableSectionSweep:
		generated = variable_section_sweep_mesh_generator_->generate(
			specification, diagnostic);
		break;
	case ShapeFamily::HostedOpening: {
		GeneratedPrimitiveMesh opening_mesh =
			extrude_profile_mesh_generator_->generate(specification, diagnostic);
		std::vector<MeshSurfaceTag> opening_tags;
		opening_tags.reserve(opening_mesh.faceSurfaceTags().size());
		for (const MeshSurfaceTag &tag : opening_mesh.faceSurfaceTags()) {
			opening_tags.emplace_back(
				tag.role() == MeshSurfaceRole::ProfileInnerSide
					? MeshSurfaceRole::HostedOpeningCutWall
					: tag.role(),
				tag.boundaryIndex(),
				tag.objectPartIndex());
		}
		generated = GeneratedPrimitiveMesh(opening_mesh.mesh(), std::move(opening_tags));
		break;
	}
	case ShapeFamily::EmbossedBead:
	case ShapeFamily::EdgeFlange: {
		GeneratedPrimitiveMesh feature_mesh =
			sweep_profile_mesh_generator_->generate(specification, diagnostic);
		const MeshSurfaceRole feature_role =
			specification.family() == ShapeFamily::EmbossedBead
				? MeshSurfaceRole::EmbossedBeadOuter
				: MeshSurfaceRole::EdgeFlangeOuter;
		std::vector<MeshSurfaceTag> feature_tags;
		feature_tags.reserve(feature_mesh.faceSurfaceTags().size());
		for (const MeshSurfaceTag &tag : feature_mesh.faceSurfaceTags()) {
			feature_tags.emplace_back(
				feature_role, tag.boundaryIndex(), tag.objectPartIndex());
		}
		generated = GeneratedPrimitiveMesh(feature_mesh.mesh(), std::move(feature_tags));
		break;
	}
	case ShapeFamily::SweepDisk:
		generated = sweep_disk_mesh_generator_->generate(specification, diagnostic);
		break;
	case ShapeFamily::TaperedSweep:
		generated = tapered_sweep_mesh_generator_->generate(specification, diagnostic);
		break;
	case ShapeFamily::BranchJunction:
		generated = branch_junction_mesh_generator_->generate(specification, diagnostic);
		break;
	case ShapeFamily::LeafBlade:
	case ShapeFamily::PetalBlade:
		generated = botanical_blade_mesh_generator_->generate(specification, diagnostic);
		break;
	case ShapeFamily::Plant:
		generated = plant_mesh_generator_->generate(specification, diagnostic);
		break;
	case ShapeFamily::Vine:
		generated = vine_mesh_generator_->generate(specification, diagnostic);
		break;
	case ShapeFamily::ScatterRegion:
		generated = scatter_region_mesh_generator_->generate(
			specification, diagnostic);
		break;
	case ShapeFamily::Revolve:
		generated = revolve_mesh_generator_->generate(specification, diagnostic);
		break;
	case ShapeFamily::Loft:
	case ShapeFamily::SurfaceLoft:
	case ShapeFamily::ShellLoft:
		generated = loft_mesh_generator_->generate(specification, diagnostic);
		break;
	case ShapeFamily::CurveNetworkSurface:
		generated = curve_network_surface_mesh_generator_->generate(
			specification, diagnostic);
		break;
	case ShapeFamily::CurvedPanel:
		generated = curved_panel_mesh_generator_->generate(specification, diagnostic);
		break;
	case ShapeFamily::FormedPanel: {
		const auto *formed_panel =
			dynamic_cast<const FormedPanelShapeSpecification *>(&specification);
		if (formed_panel == nullptr) {
			if (diagnostic != nullptr) {
				*diagnostic = "FormedPanel requires a FormedPanelShapeSpecification.";
			}
			return {};
		}
		GeneratedPrimitiveMesh panel_surface =
			loft_mesh_generator_->generate(specification, diagnostic);
		if (!panel_surface.mesh() || panel_surface.mesh()->faces.empty()) return {};
		const ShellOffsetShapeSpecification offset_specification(
			{},
			formed_panel->thickness(),
			formed_panel->offsetSide(),
			ShapeSpecificationKey(
				formed_panel->key().canonicalValue() + ":formed-panel-offset"),
			formed_panel->canonicalText(),
			formed_panel->detailLevel());
		const GeometryBuildResult panel_result =
			ShellOffsetGeometryBuilder().build(panel_surface, offset_specification);
		if (!panel_result.succeeded()) {
			if (diagnostic != nullptr) *diagnostic = panel_result.firstDiagnostic();
			return {};
		}
		generated = panel_result.generatedMesh();
		break;
	}
	case ShapeFamily::MirrorShape: {
		const auto *mirror_specification =
			dynamic_cast<const MirrorShapeSpecification *>(&specification);
		if (mirror_specification == nullptr || !mirror_specification->sourceShape()) {
			if (diagnostic != nullptr) {
				*diagnostic = "MirrorShape requires a source shape specification.";
			}
			return {};
		}
		const std::shared_ptr<const ResolvedPrimitiveGeometry> source_geometry = resolve(
			*mirror_specification->sourceShape(), diagnostic);
		if (!source_geometry || !source_geometry->hasTriangleMesh()) {
			if (diagnostic != nullptr && diagnostic->empty()) {
				*diagnostic = "MirrorShape source did not resolve to a triangle mesh.";
			}
			return {};
		}
		const GeneratedPrimitiveMesh source_mesh(
			std::make_shared<Mesh>(source_geometry->triangleMesh()),
			source_geometry->faceSurfaceTags());
		const GeometryBuildResult mirror_result =
			MirroredShapeGeometryBuilder().build(source_mesh, *mirror_specification);
		if (!mirror_result.succeeded()) {
			if (diagnostic != nullptr) *diagnostic = mirror_result.firstDiagnostic();
			return {};
		}
		generated = mirror_result.generatedMesh();
		break;
	}
	case ShapeFamily::ShellOffset: {
		const auto *shell_specification =
			dynamic_cast<const ShellOffsetShapeSpecification *>(&specification);
		if (shell_specification == nullptr || !shell_specification->sourceShape()) {
			if (diagnostic != nullptr) {
				*diagnostic = "ShellOffset requires a source shape specification.";
			}
			return {};
		}
		const std::shared_ptr<const ResolvedPrimitiveGeometry> source_geometry = resolve(
			*shell_specification->sourceShape(), diagnostic);
		if (!source_geometry || !source_geometry->hasTriangleMesh()) {
			if (diagnostic != nullptr && diagnostic->empty()) {
				*diagnostic = "ShellOffset source did not resolve to a triangle mesh.";
			}
			return {};
		}
		const GeneratedPrimitiveMesh source_mesh(
			std::make_shared<Mesh>(source_geometry->triangleMesh()),
			source_geometry->faceSurfaceTags());
		const GeometryBuildResult shell_result =
			ShellOffsetGeometryBuilder().build(source_mesh, *shell_specification);
		if (!shell_result.succeeded()) {
			if (diagnostic != nullptr) *diagnostic = shell_result.firstDiagnostic();
			return {};
		}
		generated = shell_result.generatedMesh();
		break;
	}
	case ShapeFamily::FoldedProfile:
		generated = folded_profile_mesh_generator_->generate(specification, diagnostic);
		break;
	case ShapeFamily::CompoundShape: {
		const auto *compound =
			dynamic_cast<const CompoundShapeSpecification *>(&specification);
		if (compound == nullptr || compound->parts().empty()) {
			if (diagnostic != nullptr) {
				*diagnostic = "CompoundShape requires a non-empty CompoundShapeSpecification.";
			}
			return {};
		}
		std::vector<GeneratedMeshPlacement> placements;
		placements.reserve(compound->parts().size());
		for (const CompoundShapePartSpecification &part : compound->parts()) {
			std::string part_diagnostic;
			const std::shared_ptr<const ResolvedPrimitiveGeometry> part_geometry =
				resolve(*part.shape(), &part_diagnostic);
			if (!part_geometry || !part_geometry->hasTriangleMesh()) {
				if (diagnostic != nullptr) {
					*diagnostic = "CompoundShape child part '" + part.purpose() + "' failed";
					if (!part_diagnostic.empty()) {
						*diagnostic += ": " + part_diagnostic;
					}
					else {
						*diagnostic += " to resolve to a triangle mesh.";
					}
				}
				return {};
			}
			placements.emplace_back(
				part.purpose(),
				GeneratedPrimitiveMesh(
					std::make_shared<Mesh>(part_geometry->triangleMesh()),
					part_geometry->faceSurfaceTags()),
				part.localTransform());
		}
		const GeometryBuildResult compound_result =
			GeneratedMeshComposer().compose(placements);
		if (!compound_result.succeeded()) {
			if (diagnostic != nullptr) *diagnostic = compound_result.firstDiagnostic();
			return {};
		}
		generated = compound_result.generatedMesh();
		break;
	}
	case ShapeFamily::InstanceArray: {
		const auto *array_specification =
			dynamic_cast<const InstanceArrayShapeSpecification *>(&specification);
		if (array_specification == nullptr || !array_specification->sourceShape()) {
			if (diagnostic != nullptr) {
				*diagnostic = "InstanceArray requires an InstanceArrayShapeSpecification with a source shape.";
			}
			return {};
		}
		const std::shared_ptr<const ResolvedPrimitiveGeometry> source_geometry = resolve(
			*array_specification->sourceShape(), diagnostic);
		if (!source_geometry || !source_geometry->hasTriangleMesh()) {
			if (diagnostic != nullptr && diagnostic->empty()) {
				*diagnostic = "InstanceArray source did not resolve to a triangle mesh.";
			}
			return {};
		}
		GeneratedPrimitiveMesh source_mesh(
			std::make_shared<Mesh>(source_geometry->triangleMesh()),
			source_geometry->faceSurfaceTags());
		const GeometryBuildResult array_result = InstanceArrayGeometryBuilder().build(
			source_mesh, array_specification->instanceArray());
		if (!array_result.succeeded()) {
			if (diagnostic != nullptr) *diagnostic = array_result.firstDiagnostic();
			return {};
		}
		generated = array_result.generatedMesh();
		break;
	}
	case ShapeFamily::GeneratedMeshReference: {
		const auto *generated_mesh_reference =
			dynamic_cast<const GeneratedMeshReferenceShapeSpecification *>(
				&specification);
		if (generated_mesh_reference == nullptr) {
			if (diagnostic != nullptr) {
				*diagnostic =
					"GeneratedMeshReference requires a GeneratedMeshReferenceShapeSpecification.";
			}
			return {};
		}
		const GeneratedMeshResolutionResult resolution =
			generated_mesh_reference_resolution_service_->resolve(
				generated_mesh_reference->meshKey(),
				generated_mesh_reference->detailLevel());
		if (!resolution.succeeded() || !resolution.generatedMesh().has_value()) {
			if (diagnostic != nullptr) *diagnostic = resolution.diagnostic();
			return {};
		}
		generated = *resolution.generatedMesh();
		break;
	}
	default:
		if (diagnostic != nullptr) {
			*diagnostic = "The requested procedural shape family is not implemented.";
		}
		return {};
	}
	if (!generated.mesh() || generated.mesh()->faces.empty()) {
		if (diagnostic != nullptr && diagnostic->empty()) {
			*diagnostic = "Procedural shape generation produced no triangles.";
		}
		return {};
	}
	if (generated.faceSurfaceTags().size() != generated.mesh()->faces.size()) {
		if (diagnostic != nullptr) {
			*diagnostic = "Generated face surface tags do not match the triangle count.";
		}
		return {};
	}

	MeshTopologyAnalyzer analyzer;
	const MeshTopologyReport topology = analyzer.analyze(
		*generated.mesh(), generated.faceSurfaceTags());
	if (topology.degenerate_triangle_count != 0) {
		if (diagnostic != nullptr) {
			*diagnostic = "Generated shape contains degenerate triangles.";
		}
		return {};
	}
	if (specification.requestsClosedGeometry() && !topology.isWatertight()) {
		if (diagnostic != nullptr) {
			*diagnostic = "Requested closed shape is not watertight after topology analysis: " +
			              std::to_string(topology.boundary_edge_count) +
			              " boundary edges and " +
			              std::to_string(topology.nonmanifold_edge_count) +
			              " nonmanifold edges.";
		}
		return {};
	}

	std::shared_ptr<const Mesh> immutable_mesh = generated.mesh();
	auto resolved = ResolvedPrimitiveGeometry::createTriangleMesh(
		std::move(immutable_mesh),
		generated.faceSurfaceTags(),
		topology.isWatertight(),
		geometry_evidence_service_->createVolumeEvidence(specification, topology),
		geometry_evidence_service_->chooseCollisionPolicy(
			specification, topology.isWatertight()),
		topology.topology_hash);
	resolved_shapes_.emplace(cache_key, resolved);
	return resolved;
}

std::size_t ProceduralMeshRepository::cachedShapeCount() const
{
	return resolved_shapes_.size();
}
