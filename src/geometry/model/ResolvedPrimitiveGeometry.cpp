#include "geometry/model/ResolvedPrimitiveGeometry.h"

#include <limits>
#include <stdexcept>
#include <utility>

std::shared_ptr<const ResolvedPrimitiveGeometry>
ResolvedPrimitiveGeometry::createCubeVertexTemplate()
{
	return std::shared_ptr<const ResolvedPrimitiveGeometry>(
		new ResolvedPrimitiveGeometry(
			PrimitiveGeometryRepresentation::CubeVertexTemplate,
			{},
			{},
			true,
			PrimitiveVolumeEvidence::createAnalytic(1.0f),
			PrimitiveCollisionPolicy::ConvexMesh,
			0));
}

std::shared_ptr<const ResolvedPrimitiveGeometry>
ResolvedPrimitiveGeometry::createTriangleMesh(
	std::shared_ptr<const Mesh> mesh,
	std::vector<MeshSurfaceTag> face_surface_tags,
	bool watertight,
	PrimitiveVolumeEvidence volume_evidence,
	PrimitiveCollisionPolicy collision_policy,
	std::uint64_t topology_hash)
{
	return std::shared_ptr<const ResolvedPrimitiveGeometry>(
		new ResolvedPrimitiveGeometry(
			PrimitiveGeometryRepresentation::TriangleMesh,
			std::move(mesh),
			std::move(face_surface_tags),
			watertight,
			volume_evidence,
			collision_policy,
			topology_hash));
}

ResolvedPrimitiveGeometry::ResolvedPrimitiveGeometry(
	PrimitiveGeometryRepresentation representation,
	std::shared_ptr<const Mesh> mesh,
	std::vector<MeshSurfaceTag> face_surface_tags,
	bool watertight,
	PrimitiveVolumeEvidence volume_evidence,
	PrimitiveCollisionPolicy collision_policy,
	std::uint64_t topology_hash)
	: representation_(representation),
	  mesh_(std::move(mesh)),
	  face_surface_tags_(std::move(face_surface_tags)),
	  watertight_(watertight),
	  volume_evidence_(volume_evidence),
	  collision_policy_(collision_policy),
	  topology_hash_(topology_hash)
{
	if (!mesh_ || mesh_->vertices.empty()) {
		return;
	}

	local_bounds_minimum_ = glm::vec3(std::numeric_limits<float>::max());
	local_bounds_maximum_ = glm::vec3(std::numeric_limits<float>::lowest());
	for (const glm::vec3 &vertex : mesh_->vertices) {
		local_bounds_minimum_ = glm::min(local_bounds_minimum_, vertex);
		local_bounds_maximum_ = glm::max(local_bounds_maximum_, vertex);
	}
	has_local_bounds_ = true;
}

PrimitiveGeometryRepresentation ResolvedPrimitiveGeometry::representation() const
{
	return representation_;
}

bool ResolvedPrimitiveGeometry::usesCubeVertexTemplate() const
{
	return representation_ == PrimitiveGeometryRepresentation::CubeVertexTemplate;
}

bool ResolvedPrimitiveGeometry::hasTriangleMesh() const
{
	return representation_ == PrimitiveGeometryRepresentation::TriangleMesh &&
	       mesh_ && !mesh_->faces.empty();
}

const Mesh &ResolvedPrimitiveGeometry::triangleMesh() const
{
	if (!mesh_) {
		throw std::logic_error("Resolved primitive geometry has no triangle mesh.");
	}
	return *mesh_;
}

std::size_t ResolvedPrimitiveGeometry::triangleVertexCount() const
{
	return hasTriangleMesh() ? triangleMesh().faces.size() * 3u : 0u;
}

bool ResolvedPrimitiveGeometry::hasLocalBounds() const
{
	return has_local_bounds_;
}

const glm::vec3 &ResolvedPrimitiveGeometry::localBoundsMinimum() const
{
	return local_bounds_minimum_;
}

const glm::vec3 &ResolvedPrimitiveGeometry::localBoundsMaximum() const
{
	return local_bounds_maximum_;
}

const std::vector<MeshSurfaceTag> &ResolvedPrimitiveGeometry::faceSurfaceTags() const
{
	return face_surface_tags_;
}

bool ResolvedPrimitiveGeometry::isWatertight() const
{
	return watertight_;
}

const PrimitiveVolumeEvidence &ResolvedPrimitiveGeometry::volumeEvidence() const
{
	return volume_evidence_;
}

PrimitiveCollisionPolicy ResolvedPrimitiveGeometry::collisionPolicy() const
{
	return collision_policy_;
}

std::uint64_t ResolvedPrimitiveGeometry::topologyHash() const
{
	return topology_hash_;
}
