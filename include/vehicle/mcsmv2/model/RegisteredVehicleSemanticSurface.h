#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "geometry/model/MeshTopologyReport.h"
#include "vehicle/mcsmv2/model/SemanticImplicitCorrespondenceReport.h"

#include <glm/glm.hpp>

#include <string>
#include <utility>
#include <vector>

class RegisteredVehicleSemanticSurface
{
public:
	RegisteredVehicleSemanticSurface(
		std::string identifier,
		GeneratedPrimitiveMesh raw_semantic_mesh,
		GeneratedPrimitiveMesh implicit_scaffold_mesh,
		GeneratedPrimitiveMesh registered_semantic_mesh,
		std::vector<glm::dvec2> vertex_surface_coordinates,
		MeshTopologyReport topology_report,
		SemanticImplicitCorrespondenceReport correspondence_report)
		: identifier_(std::move(identifier)),
		  raw_semantic_mesh_(std::move(raw_semantic_mesh)),
		  implicit_scaffold_mesh_(std::move(implicit_scaffold_mesh)),
		  registered_semantic_mesh_(std::move(registered_semantic_mesh)),
		  vertex_surface_coordinates_(std::move(vertex_surface_coordinates)),
		  topology_report_(topology_report),
		  correspondence_report_(std::move(correspondence_report))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const GeneratedPrimitiveMesh &rawSemanticMesh() const { return raw_semantic_mesh_; }
	const GeneratedPrimitiveMesh &implicitScaffoldMesh() const
	{
		return implicit_scaffold_mesh_;
	}
	const GeneratedPrimitiveMesh &registeredSemanticMesh() const
	{
		return registered_semantic_mesh_;
	}
	const std::vector<glm::dvec2> &vertexSurfaceCoordinates() const
	{
		return vertex_surface_coordinates_;
	}
	const MeshTopologyReport &topologyReport() const { return topology_report_; }
	const SemanticImplicitCorrespondenceReport &correspondenceReport() const
	{
		return correspondence_report_;
	}

private:
	std::string identifier_;
	GeneratedPrimitiveMesh raw_semantic_mesh_{std::make_shared<Mesh>(), {}};
	GeneratedPrimitiveMesh implicit_scaffold_mesh_{std::make_shared<Mesh>(), {}};
	GeneratedPrimitiveMesh registered_semantic_mesh_{std::make_shared<Mesh>(), {}};
	std::vector<glm::dvec2> vertex_surface_coordinates_;
	MeshTopologyReport topology_report_;
	SemanticImplicitCorrespondenceReport correspondence_report_{
		SurfaceCorrespondenceDirectionReport(0u, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, ""),
		SurfaceCorrespondenceDirectionReport(0u, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, ""),
		0.0, 0.0, 0.0, {}, false};
};
