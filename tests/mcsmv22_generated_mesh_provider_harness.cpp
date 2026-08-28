#include "geometry/service/GeneratedMeshReferenceResolutionService.h"
#include "geometry/service/MeshTopologyAnalyzer.h"

#include <array>
#include <iostream>
#include <string>

namespace {

bool require(bool condition, const std::string &message)
{
	if (!condition) std::cerr << "FAIL: " << message << '\n';
	return condition;
}

} // namespace

int main()
{
	const std::array<const char *, 8> keys = {{
		"MCSMv22ReferenceFixedBodyMesh",
		"MCSMv22ReferenceClosuresClosedMesh",
		"MCSMv22ReferenceClosuresOpenMesh",
		"MCSMv22ReferenceGlassClosedMesh",
		"MCSMv22ReferenceGlassOpenMesh",
		"MCSMv22ReferenceSuspensionMesh",
		"MCSMv22ReferenceNominalTyreMesh",
		"MCSMv22ReferenceTyreSweepMesh",
	}};
	const GeneratedMeshReferenceResolutionService service;
	bool passed = true;
	for (const char *key : keys) {
		const GeneratedMeshResolutionResult first =
			service.resolve(key, GeometryDetailLevel::Assembly);
		passed &= require(first.succeeded(), std::string(key) + " must resolve");
		if (!first.succeeded() || !first.generatedMesh() ||
		    !first.generatedMesh()->mesh()) {
			std::cerr << first.diagnostic() << '\n';
			continue;
		}
		const Mesh &first_mesh = *first.generatedMesh()->mesh();
		passed &= require(!first_mesh.vertices.empty(), std::string(key) + " must contain vertices");
		passed &= require(!first_mesh.faces.empty(), std::string(key) + " must contain faces");
		passed &= require(first_mesh.vertices.size() < (1u << 16u), std::string(key) + " must remain within 16-bit draw-list limits");
		const GeneratedMeshResolutionResult second =
			service.resolve(key, GeometryDetailLevel::Assembly);
		passed &= require(second.succeeded() && second.generatedMesh(), std::string(key) + " repeated resolution must succeed");
		if (!second.succeeded() || !second.generatedMesh()) continue;
		const MeshTopologyReport first_topology = MeshTopologyAnalyzer().analyze(first_mesh, first.generatedMesh()->faceSurfaceTags());
		const MeshTopologyReport second_topology = MeshTopologyAnalyzer().analyze(*second.generatedMesh()->mesh(), second.generatedMesh()->faceSurfaceTags());
		passed &= require(first_topology.topology_hash == second_topology.topology_hash, std::string(key) + " repeated resolution must be deterministic");
		std::cout << key << " vertices=" << first_mesh.vertices.size()
		          << " faces=" << first_mesh.faces.size()
		          << " hash=" << first_topology.topology_hash << '\n';
	}
	if (!passed) return 1;
	std::cout << "MCSMv2.2 generated mesh provider checks passed.\n";
	return 0;
}
