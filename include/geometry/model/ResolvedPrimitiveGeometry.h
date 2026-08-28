#pragma once

#include "Mesh.h"
#include "geometry/model/MeshSurfaceTag.h"
#include "geometry/model/PrimitiveCollisionPolicy.h"
#include "geometry/model/PrimitiveVolumeEvidence.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

enum class PrimitiveGeometryRepresentation
{
	CubeVertexTemplate,
	TriangleMesh
};

class ResolvedPrimitiveGeometry
{
public:
	static std::shared_ptr<const ResolvedPrimitiveGeometry> createCubeVertexTemplate();
	static std::shared_ptr<const ResolvedPrimitiveGeometry> createTriangleMesh(
		std::shared_ptr<const Mesh> mesh,
		std::vector<MeshSurfaceTag> face_surface_tags = {},
		bool watertight = false,
		PrimitiveVolumeEvidence volume_evidence =
			PrimitiveVolumeEvidence::createUndefined(),
		PrimitiveCollisionPolicy collision_policy =
			PrimitiveCollisionPolicy::StaticTriangleMesh,
		std::uint64_t topology_hash = 0);

	PrimitiveGeometryRepresentation representation() const;
	bool usesCubeVertexTemplate() const;
	bool hasTriangleMesh() const;
	const Mesh &triangleMesh() const;
	std::size_t triangleVertexCount() const;

	bool hasLocalBounds() const;
	const glm::vec3 &localBoundsMinimum() const;
	const glm::vec3 &localBoundsMaximum() const;
	const std::vector<MeshSurfaceTag> &faceSurfaceTags() const;
	bool isWatertight() const;
	const PrimitiveVolumeEvidence &volumeEvidence() const;
	PrimitiveCollisionPolicy collisionPolicy() const;
	std::uint64_t topologyHash() const;

private:
	ResolvedPrimitiveGeometry(PrimitiveGeometryRepresentation representation,
	                          std::shared_ptr<const Mesh> mesh,
	                          std::vector<MeshSurfaceTag> face_surface_tags,
	                          bool watertight,
	                          PrimitiveVolumeEvidence volume_evidence,
	                          PrimitiveCollisionPolicy collision_policy,
	                          std::uint64_t topology_hash);

	PrimitiveGeometryRepresentation representation_ =
		PrimitiveGeometryRepresentation::CubeVertexTemplate;
	std::shared_ptr<const Mesh> mesh_;
	glm::vec3 local_bounds_minimum_{0.0f};
	glm::vec3 local_bounds_maximum_{0.0f};
	bool has_local_bounds_ = false;
	std::vector<MeshSurfaceTag> face_surface_tags_;
	bool watertight_ = false;
	PrimitiveVolumeEvidence volume_evidence_ =
		PrimitiveVolumeEvidence::createUndefined();
	PrimitiveCollisionPolicy collision_policy_ =
		PrimitiveCollisionPolicy::Disabled;
	std::uint64_t topology_hash_ = 0;
};
