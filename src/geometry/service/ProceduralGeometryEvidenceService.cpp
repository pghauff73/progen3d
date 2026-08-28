#include "geometry/service/ProceduralGeometryEvidenceService.h"

#include "geometry/model/CylinderShapeSpecification.h"
#include "geometry/model/ExtrudeProfileShapeSpecification.h"
#include "geometry/model/RevolveShapeSpecification.h"
#include "geometry/model/SphereShapeSpecification.h"
#include "geometry/model/SweepProfileShapeSpecification.h"
#include "geometry/model/SweepDiskShapeSpecification.h"
#include "geometry/service/MeshTopologyAnalyzer.h"

#include <glm/gtc/constants.hpp>

#include <cmath>

namespace {

bool is_convex_profile(const Profile2D &profile)
{
	if (!profile.innerLoops().empty()) return false;
	const std::vector<glm::vec2> &points = profile.outerLoop().points();
	for (std::size_t index = 0; index < points.size(); ++index) {
		const glm::vec2 first =
			points[(index + 1u) % points.size()] - points[index];
		const glm::vec2 second =
			points[(index + 2u) % points.size()] -
			points[(index + 1u) % points.size()];
		if (first.x * second.y - first.y * second.x <= 0.0f) return false;
	}
	return true;
}

}

PrimitiveVolumeEvidence ProceduralGeometryEvidenceService::createVolumeEvidence(
	const ShapeSpecification &specification,
	const MeshTopologyReport &topology) const
{
	if (!topology.isWatertight()) {
		return PrimitiveVolumeEvidence::createUndefined();
	}

	switch (specification.family()) {
	case ShapeFamily::Cylinder: {
		const auto &cylinder =
			static_cast<const CylinderShapeSpecification &>(specification);
		if (cylinder.topology() == ShapeTopology::Surface) {
			return PrimitiveVolumeEvidence::createUndefined();
		}
		if (!cylinder.clips().empty()) {
			return PrimitiveVolumeEvidence::createMeshDerived(
				std::fabs(topology.signed_volume));
		}
		const float outer = 0.5f * cylinder.radialDomain().maximumRadiusFraction();
		const float inner = 0.5f * cylinder.radialDomain().minimumRadiusFraction();
		const float height = cylinder.axialDomain().maximumPosition() -
		                     cylinder.axialDomain().minimumPosition();
		const float sweep = glm::radians(cylinder.angularDomain().sweepDegrees());
		return PrimitiveVolumeEvidence::createAnalytic(
			0.5f * sweep * (outer * outer - inner * inner) * height);
	}

	case ShapeFamily::Sphere: {
		const auto &sphere =
			static_cast<const SphereShapeSpecification &>(specification);
		if (sphere.topology() == ShapeTopology::Surface) {
			return PrimitiveVolumeEvidence::createUndefined();
		}
		if (!sphere.clips().empty()) {
			return PrimitiveVolumeEvidence::createMeshDerived(
				std::fabs(topology.signed_volume));
		}
		const float outer = 0.5f * sphere.radialDomain().maximumRadiusFraction();
		const float inner = 0.5f * sphere.radialDomain().minimumRadiusFraction();
		const float polar_minimum = glm::radians(
			sphere.polarDomain().minimumDegrees());
		const float polar_maximum = glm::radians(
			sphere.polarDomain().maximumDegrees());
		const float azimuth_sweep = glm::radians(
			sphere.angularDomain().sweepDegrees());
		const float radial_volume =
			(outer * outer * outer - inner * inner * inner) / 3.0f;
		return PrimitiveVolumeEvidence::createAnalytic(
			radial_volume * azimuth_sweep *
			(std::cos(polar_minimum) - std::cos(polar_maximum)));
	}

	case ShapeFamily::AxialProfile:
	case ShapeFamily::ExtrudeProfile:
	case ShapeFamily::SweepProfile:
	case ShapeFamily::VariableSectionSweep:
	case ShapeFamily::HostedOpening:
	case ShapeFamily::EmbossedBead:
	case ShapeFamily::EdgeFlange:
	case ShapeFamily::PanelCut:
	case ShapeFamily::SweepDisk:
	case ShapeFamily::TaperedSweep:
	case ShapeFamily::BranchJunction:
	case ShapeFamily::LeafBlade:
	case ShapeFamily::PetalBlade:
	case ShapeFamily::Plant:
	case ShapeFamily::Vine:
	case ShapeFamily::ScatterRegion:
	case ShapeFamily::Revolve:
	case ShapeFamily::Loft:
	case ShapeFamily::SurfaceLoft:
	case ShapeFamily::CurveNetworkSurface:
	case ShapeFamily::ShellLoft:
	case ShapeFamily::ShellOffset:
	case ShapeFamily::MirrorShape:
	case ShapeFamily::CurvedPanel:
	case ShapeFamily::FoldedProfile:
	case ShapeFamily::FormedPanel:
	case ShapeFamily::CompoundShape:
	case ShapeFamily::GeneratedMeshReference:
		return PrimitiveVolumeEvidence::createMeshDerived(
			std::fabs(topology.signed_volume));

	default:
		break;
	}

	return PrimitiveVolumeEvidence::createUndefined();
}

PrimitiveCollisionPolicy ProceduralGeometryEvidenceService::chooseCollisionPolicy(
	const ShapeSpecification &specification,
	bool watertight) const
{
	switch (specification.family()) {
	case ShapeFamily::Cylinder: {
		const auto &cylinder =
			static_cast<const CylinderShapeSpecification &>(specification);
		if (cylinder.topology() == ShapeTopology::Surface || !watertight ||
		    cylinder.topology() == ShapeTopology::Shell) {
			return PrimitiveCollisionPolicy::StaticTriangleMesh;
		}
		return cylinder.angularDomain().isFullRevolution() ||
		       cylinder.angularDomain().sweepDegrees() <= 180.0f
			? PrimitiveCollisionPolicy::ConvexMesh
			: PrimitiveCollisionPolicy::StaticTriangleMesh;
	}

	case ShapeFamily::Sphere: {
		const auto &sphere =
			static_cast<const SphereShapeSpecification &>(specification);
		if (sphere.topology() == ShapeTopology::Surface || !watertight ||
		    sphere.topology() == ShapeTopology::Shell) {
			return PrimitiveCollisionPolicy::StaticTriangleMesh;
		}
		const bool full_polar_domain =
			sphere.polarDomain().minimumDegrees() == 0.0f &&
			sphere.polarDomain().maximumDegrees() == 180.0f;
		const bool convex_azimuth_domain =
			sphere.angularDomain().isFullRevolution() ||
			sphere.angularDomain().sweepDegrees() <= 180.0f;
		return full_polar_domain && convex_azimuth_domain
			? PrimitiveCollisionPolicy::ConvexMesh
			: PrimitiveCollisionPolicy::StaticTriangleMesh;
	}

	case ShapeFamily::AxialProfile:
		return PrimitiveCollisionPolicy::StaticTriangleMesh;

	case ShapeFamily::ExtrudeProfile: {
		const auto &extrude =
			static_cast<const ExtrudeProfileShapeSpecification &>(specification);
		if (!watertight) return PrimitiveCollisionPolicy::StaticTriangleMesh;
		return is_convex_profile(extrude.profile())
			? PrimitiveCollisionPolicy::ConvexMesh
			: PrimitiveCollisionPolicy::StaticTriangleMesh;
	}

	case ShapeFamily::SweepProfile:
	case ShapeFamily::PanelCut: {
		const auto &sweep =
			static_cast<const SweepProfileShapeSpecification &>(specification);
		if (!watertight || sweep.pathPoints().size() != 2u) {
			return PrimitiveCollisionPolicy::StaticTriangleMesh;
		}
		return is_convex_profile(sweep.profile())
			? PrimitiveCollisionPolicy::ConvexMesh
			: PrimitiveCollisionPolicy::StaticTriangleMesh;
	}

	case ShapeFamily::VariableSectionSweep:
	case ShapeFamily::HostedOpening:
	case ShapeFamily::EmbossedBead:
	case ShapeFamily::EdgeFlange:
		return PrimitiveCollisionPolicy::StaticTriangleMesh;

	case ShapeFamily::SweepDisk: {
		const auto &sweep =
			static_cast<const SweepDiskShapeSpecification &>(specification);
		if (!watertight || sweep.pathPoints().size() != 2u) {
			return PrimitiveCollisionPolicy::StaticTriangleMesh;
		}
		return PrimitiveCollisionPolicy::ConvexMesh;
	}

	case ShapeFamily::TaperedSweep:
	case ShapeFamily::BranchJunction:
		return watertight
			? PrimitiveCollisionPolicy::StaticTriangleMesh
			: PrimitiveCollisionPolicy::StaticTriangleMesh;

	case ShapeFamily::LeafBlade:
	case ShapeFamily::PetalBlade:
	case ShapeFamily::Plant:
	case ShapeFamily::Vine:
	case ShapeFamily::ScatterRegion:
		return PrimitiveCollisionPolicy::StaticTriangleMesh;

	case ShapeFamily::Revolve:
	case ShapeFamily::Loft:
	case ShapeFamily::SurfaceLoft:
	case ShapeFamily::CurveNetworkSurface:
	case ShapeFamily::ShellLoft:
	case ShapeFamily::ShellOffset:
	case ShapeFamily::MirrorShape:
	case ShapeFamily::CurvedPanel:
	case ShapeFamily::FoldedProfile:
	case ShapeFamily::FormedPanel:
	case ShapeFamily::CompoundShape:
	case ShapeFamily::GeneratedMeshReference:
		return PrimitiveCollisionPolicy::StaticTriangleMesh;

	default:
		break;
	}

	return PrimitiveCollisionPolicy::Disabled;
}
