#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "geometry/model/MeshTopologyReport.h"
#include "vehicle/mcsmv2/model/VehicleSemanticSurfacePatch.h"
#include "vehicle/mcsmv2/model/VehicleSemanticSurfaceSection.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class VehicleSemanticSurface
{
public:
	VehicleSemanticSurface(
		std::string identifier,
		GeneratedPrimitiveMesh generated_mesh,
		std::vector<VehicleSemanticSurfaceSection> sections,
		std::vector<VehicleSemanticSurfacePatch> patches,
		MeshTopologyReport topology_report,
		std::uint64_t deterministic_hash)
		: identifier_(std::move(identifier)),
		  generated_mesh_(std::move(generated_mesh)),
		  sections_(std::move(sections)),
		  patches_(std::move(patches)),
		  topology_report_(topology_report),
		  deterministic_hash_(deterministic_hash)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const GeneratedPrimitiveMesh &generatedMesh() const { return generated_mesh_; }
	const std::vector<VehicleSemanticSurfaceSection> &sections() const
	{
		return sections_;
	}
	const std::vector<VehicleSemanticSurfacePatch> &patches() const
	{
		return patches_;
	}
	const MeshTopologyReport &topologyReport() const { return topology_report_; }
	std::uint64_t deterministicHash() const { return deterministic_hash_; }

private:
	std::string identifier_;
	GeneratedPrimitiveMesh generated_mesh_{std::make_shared<Mesh>(), {}};
	std::vector<VehicleSemanticSurfaceSection> sections_;
	std::vector<VehicleSemanticSurfacePatch> patches_;
	MeshTopologyReport topology_report_;
	std::uint64_t deterministic_hash_ = 0u;
};
