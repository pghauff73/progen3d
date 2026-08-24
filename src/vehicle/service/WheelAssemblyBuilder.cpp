#include "vehicle/service/WheelAssemblyBuilder.h"

#include "geometry/model/ExtrudeProfileShapeSpecification.h"
#include "geometry/model/InstanceArrayShapeSpecification.h"
#include "geometry/model/InstanceArraySpecification.h"
#include "geometry/model/RevolveShapeSpecification.h"
#include "geometry/model/RoundedBoxSpecification.h"
#include "geometry/service/ExtrudeProfileMeshGenerator.h"
#include "geometry/service/ExtrudeProfileSpecificationValidator.h"
#include "geometry/service/InstanceArrayGeometryBuilder.h"
#include "geometry/service/Profile2DFactory.h"
#include "geometry/service/RevolveMeshGenerator.h"
#include "geometry/service/RevolveSpecificationValidator.h"
#include "geometry/service/RoundedBoxShapeSpecificationFactory.h"
#include "spatial/model/InterfaceCompatibilityProfile.h"
#include "spatial/model/SpatialClearanceRequirement.h"
#include "spatial/model/SpatialInterfaceFrame.h"
#include "spatial/model/SpatialInterfaceRegion.h"
#include "vehicle/service/VehicleAssemblyCompositionService.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

struct BuiltVehiclePartGeometry
{
	std::shared_ptr<const ShapeSpecification> shape;
	GeneratedPrimitiveMesh mesh{std::make_shared<Mesh>(), {}};

	bool isValid() const
	{
		return shape && mesh.mesh() && !mesh.mesh()->faces.empty();
	}
};

bool finite_positive(float value)
{
	return std::isfinite(value) && value > 0.0f;
}

BuiltVehiclePartGeometry build_revolved_profile(
	std::vector<glm::vec2> profile_points,
	int angular_segments,
	GeometryDetailLevel detail_level,
	const GeometryComplexityLimits &complexity_limits,
	std::string *diagnostic)
{
	RevolveShapeSpecificationCandidate candidate;
	candidate.radial_profile.outer_loop = std::move(profile_points);
	candidate.angular_segments = angular_segments;
	candidate.detail_level = detail_level;
	auto shape = RevolveSpecificationValidator(complexity_limits).validate(
		std::move(candidate), diagnostic);
	if (!shape) return {};
	GeometryBuildResult built = RevolveMeshGenerator(complexity_limits).build(*shape);
	if (!built.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = built.firstDiagnostic();
		return {};
	}
	return {shape, built.generatedMesh()};
}

BuiltVehiclePartGeometry build_torus(
	float outside_radius,
	float section_width,
	GeometryDetailLevel detail_level,
	const GeometryComplexityLimits &complexity_limits,
	std::string *diagnostic)
{
	const float minor_radius = section_width * 0.5f;
	const float major_radius = outside_radius - minor_radius;
	if (major_radius <= minor_radius || minor_radius <= 0.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "Tire section width must leave a positive revolved major radius.";
		}
		return {};
	}
	std::vector<glm::vec2> profile;
	profile.reserve(16u);
	for (int index = 0; index < 16; ++index) {
		const float angle = glm::two_pi<float>() * static_cast<float>(index) / 16.0f;
		profile.emplace_back(
			major_radius + minor_radius * std::cos(angle),
			minor_radius * std::sin(angle));
	}
	return build_revolved_profile(
		std::move(profile), 48, detail_level, complexity_limits, diagnostic);
}

BuiltVehiclePartGeometry build_annular_disc(
	float inner_radius,
	float outer_radius,
	float width,
	int angular_segments,
	GeometryDetailLevel detail_level,
	const GeometryComplexityLimits &complexity_limits,
	std::string *diagnostic)
{
	return build_revolved_profile(
		{{inner_radius, -width * 0.5f},
		 {outer_radius, -width * 0.5f},
		 {outer_radius, width * 0.5f},
		 {inner_radius, width * 0.5f}},
		angular_segments, detail_level, complexity_limits, diagnostic);
}

BuiltVehiclePartGeometry build_rectangular_prism(
	float width,
	float height,
	float depth,
	GeometryDetailLevel detail_level,
	const GeometryComplexityLimits &complexity_limits,
	std::string *diagnostic)
{
	auto profile = Profile2DFactory(complexity_limits).createRectangle(
		width, height, diagnostic);
	if (!profile) return {};
	ExtrudeProfileShapeSpecificationCandidate candidate;
	candidate.profile.outer_loop = profile->outerLoop().points();
	candidate.depth = depth;
	candidate.detail_level = detail_level;
	auto shape = ExtrudeProfileSpecificationValidator(complexity_limits).validate(
		std::move(candidate), diagnostic);
	if (!shape) return {};
	GeometryBuildResult built = ExtrudeProfileMeshGenerator(complexity_limits).build(
		*shape);
	if (!built.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = built.firstDiagnostic();
		return {};
	}
	return {shape, built.generatedMesh()};
}

} // namespace

VehicleAssemblyBuildResult WheelAssemblyBuilder::build(
	const WheelAssemblySpecification &specification) const
{
	if (specification.objectIdentifier().empty() ||
	    specification.tireMaterial().empty() ||
	    specification.rimMaterial().empty() ||
	    specification.brakeMaterial().empty() ||
	    !finite_positive(specification.tireRadius()) ||
	    !finite_positive(specification.tireSectionWidth()) ||
	    !finite_positive(specification.rimRadius()) ||
	    !finite_positive(specification.rimWidth()) ||
	    !finite_positive(specification.hubRadius()) ||
	    !finite_positive(specification.brakeRadius()) ||
	    specification.rimRadius() >= specification.tireRadius() ||
	    specification.hubRadius() >= specification.rimRadius() ||
	    specification.brakeRadius() >= specification.rimRadius() ||
	    specification.spokeCount() < 3 || specification.spokeCount() > 32) {
		return VehicleAssemblyBuildResult::failed(
			"WheelAssembly requires ordered finite radii, positive widths, materials, and 3-32 spokes.");
	}

	std::string diagnostic;
	const GeometryDetailLevel detail_level = specification.detailLevel();
	BuiltVehiclePartGeometry tire = build_torus(
		specification.tireRadius(), specification.tireSectionWidth(), detail_level,
		complexity_limits_, &diagnostic);
	if (!tire.isValid()) return VehicleAssemblyBuildResult::failed(diagnostic);
	std::vector<VehicleAssemblyPart> parts;
	parts.emplace_back(
		"tire", "Tire", specification.tireMaterial(), tire.shape, tire.mesh,
		glm::mat4(1.0f),
		GeometryDetailRange(
			GeometryDetailLevel::Bounds,
			GeometryDetailLevel::FastenersAndSeals));

	if (geometryDetailLevelRank(detail_level) >=
	    geometryDetailLevelRank(GeometryDetailLevel::CoarseShape)) {
		BuiltVehiclePartGeometry rim = build_annular_disc(
			specification.hubRadius(), specification.rimRadius(),
			specification.rimWidth(), 40, detail_level, complexity_limits_,
			&diagnostic);
		if (!rim.isValid()) return VehicleAssemblyBuildResult::failed(diagnostic);
		parts.emplace_back(
			"rim_barrel", "RimBarrel", specification.rimMaterial(), rim.shape,
			rim.mesh, glm::mat4(1.0f),
			GeometryDetailRange(
				GeometryDetailLevel::CoarseShape,
				GeometryDetailLevel::FastenersAndSeals));
	}

	if (geometryDetailLevelRank(detail_level) >=
	    geometryDetailLevelRank(GeometryDetailLevel::Assembly)) {
		BuiltVehiclePartGeometry hub = build_annular_disc(
			0.0f, specification.hubRadius(), specification.rimWidth() * 0.75f,
			32, detail_level, complexity_limits_, &diagnostic);
		BuiltVehiclePartGeometry brake = build_annular_disc(
			specification.hubRadius() * 0.58f, specification.brakeRadius(),
			0.018f, 40, detail_level, complexity_limits_, &diagnostic);
		if (!hub.isValid() || !brake.isValid()) {
			return VehicleAssemblyBuildResult::failed(diagnostic);
		}
		parts.emplace_back(
			"hub", "Hub", specification.rimMaterial(), hub.shape, hub.mesh,
			glm::mat4(1.0f),
			GeometryDetailRange(
				GeometryDetailLevel::Assembly,
				GeometryDetailLevel::FastenersAndSeals));
		parts.emplace_back(
			"brake_disc", "BrakeDisc", specification.brakeMaterial(), brake.shape,
			brake.mesh,
			glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.025f, 0.0f)),
			GeometryDetailRange(
				GeometryDetailLevel::Assembly,
				GeometryDetailLevel::FastenersAndSeals));

		const float spoke_length =
			specification.rimRadius() - specification.hubRadius();
		BuiltVehiclePartGeometry spoke = build_rectangular_prism(
			0.045f, specification.rimWidth() * 0.38f, spoke_length,
			detail_level, complexity_limits_, &diagnostic);
		if (!spoke.isValid()) return VehicleAssemblyBuildResult::failed(diagnostic);
		InstanceArraySpecification spoke_array = InstanceArraySpecification::createRadial(
			static_cast<std::size_t>(specification.spokeCount()),
			specification.hubRadius(), glm::vec3(0.0f, 1.0f, 0.0f));
		GeometryBuildResult spoke_mesh = InstanceArrayGeometryBuilder(complexity_limits_).build(
			spoke.mesh, spoke_array);
		if (!spoke_mesh.succeeded()) {
			return VehicleAssemblyBuildResult::failed(spoke_mesh.firstDiagnostic());
		}
		std::ostringstream spoke_key;
		spoke_key << "RadialSpokeArray:v1:source="
		          << spoke.shape->key().canonicalValue() << ":count="
		          << specification.spokeCount() << ":radius="
		          << specification.hubRadius();
		auto spoke_shape = std::make_shared<const InstanceArrayShapeSpecification>(
			spoke.shape, std::move(spoke_array),
			ShapeSpecificationKey(spoke_key.str()), spoke_key.str(), detail_level);
		parts.emplace_back(
			"spoke_array", "RadialSpokeArray", specification.rimMaterial(),
			spoke_shape, spoke_mesh.generatedMesh(), glm::mat4(1.0f),
			GeometryDetailRange(
				GeometryDetailLevel::Assembly,
				GeometryDetailLevel::FastenersAndSeals));

		RoundedBoxSpecification caliper_specification(
			0.10f, 0.12f, 0.06f, 0.018f, 3);
		auto caliper_shape = RoundedBoxShapeSpecificationFactory(complexity_limits_).create(
			caliper_specification, detail_level, &diagnostic);
		if (!caliper_shape) return VehicleAssemblyBuildResult::failed(diagnostic);
		GeometryBuildResult caliper_mesh =
			ExtrudeProfileMeshGenerator(complexity_limits_).build(*caliper_shape);
		if (!caliper_mesh.succeeded()) {
			return VehicleAssemblyBuildResult::failed(caliper_mesh.firstDiagnostic());
		}
		parts.emplace_back(
			"caliper", "BrakeCaliper", "brake-caliper", caliper_shape,
			caliper_mesh.generatedMesh(),
			glm::translate(
				glm::mat4(1.0f),
				glm::vec3(
					specification.brakeRadius() * 0.78f,
					-specification.rimWidth() * 0.25f,
					-specification.brakeRadius() * 0.15f)),
			GeometryDetailRange(
				GeometryDetailLevel::Assembly,
				GeometryDetailLevel::FastenersAndSeals));
	}

	if (geometryDetailLevelRank(detail_level) >=
	    geometryDetailLevelRank(GeometryDetailLevel::FastenersAndSeals)) {
		BuiltVehiclePartGeometry fastener = build_annular_disc(
			0.0f, 0.012f, 0.020f, 16, detail_level, complexity_limits_,
			&diagnostic);
		if (!fastener.isValid()) return VehicleAssemblyBuildResult::failed(diagnostic);
		InstanceArraySpecification fastener_array = InstanceArraySpecification::createRadial(
			5u, specification.hubRadius() * 0.58f, glm::vec3(0.0f, 1.0f, 0.0f));
		GeometryBuildResult fastener_mesh =
			InstanceArrayGeometryBuilder(complexity_limits_).build(
				fastener.mesh, fastener_array);
		if (!fastener_mesh.succeeded()) {
			return VehicleAssemblyBuildResult::failed(fastener_mesh.firstDiagnostic());
		}
		auto fastener_shape = std::make_shared<const InstanceArrayShapeSpecification>(
			fastener.shape, std::move(fastener_array),
			ShapeSpecificationKey(
				"WheelFastenerArray:v1:" + fastener.shape->key().canonicalValue()),
			"WheelFastenerArray(count(5))", detail_level);
		parts.emplace_back(
			"wheel_fasteners", "WheelFasteners", specification.rimMaterial(),
			fastener_shape, fastener_mesh.generatedMesh(), glm::mat4(1.0f),
			GeometryDetailRange::exact(GeometryDetailLevel::FastenersAndSeals));
	}

	GeometryBuildResult combined =
		VehicleAssemblyCompositionService(complexity_limits_).compose(parts);
	if (!combined.succeeded()) {
		return VehicleAssemblyBuildResult::failed(combined.firstDiagnostic());
	}

	const SpatialObjectId owner(specification.objectIdentifier());
	std::vector<SpatialInterface> interfaces;
	interfaces.emplace_back(
		SpatialInterfaceId("axis"), owner, SpatialInterfaceType::Shaft,
		SpatialInterfaceFrame(
			glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
			glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::axisSegment(specification.rimWidth()),
		InterfaceCompatibilityProfile(
			InterfaceShape::Axis, InterfaceGender::Female,
			std::nullopt, std::nullopt, std::nullopt, "vehicle-wheel-axis"),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.001f));
	interfaces.emplace_back(
		SpatialInterfaceId("mount_face"), owner, SpatialInterfaceType::Seat,
		SpatialInterfaceFrame(
			glm::vec3(0.0f, -specification.rimWidth() * 0.5f, 0.0f),
			glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::planeRectangle(
			specification.hubRadius() * 2.0f,
			specification.hubRadius() * 2.0f),
		InterfaceCompatibilityProfile(
			InterfaceShape::Circular, InterfaceGender::Female,
			specification.hubRadius() * 2.0f, std::nullopt, std::nullopt,
			"vehicle-hub-mount"),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.001f));

	return VehicleAssemblyBuildResult::succeeded(
		VehicleAssemblyGeometry(
			std::move(parts), combined.generatedMesh(), std::move(interfaces),
			detail_level));
}
