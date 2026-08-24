#include "vehicle/service/VehicleBodyAssemblyBuilder.h"

#include "geometry/model/GeneratedMeshPlacement.h"
#include "geometry/model/ShellLoftShapeSpecification.h"
#include "geometry/model/SweepDiskShapeSpecification.h"
#include "geometry/service/LoftMeshGenerator.h"
#include "geometry/service/LoftSpecificationValidator.h"
#include "geometry/service/SweepDiskMeshGenerator.h"
#include "geometry/service/SweepDiskSpecificationValidator.h"
#include "spatial/model/InterfaceCompatibilityProfile.h"
#include "spatial/model/SpatialClearanceRequirement.h"
#include "spatial/model/SpatialInterfaceFrame.h"
#include "spatial/model/SpatialInterfaceRegion.h"
#include "vehicle/service/VehicleAssemblyCompositionService.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {

bool finite(const glm::vec2 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y);
}

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

std::vector<glm::vec2> create_symmetric_profile(
	const std::vector<glm::vec2> &right_half)
{
	std::vector<glm::vec2> profile = right_half;
	for (std::size_t index = right_half.size() - 1u; index-- > 1u;) {
		profile.emplace_back(-right_half[index].x, right_half[index].y);
	}
	return profile;
}

std::vector<glm::vec2> create_inner_profile(
	const std::vector<glm::vec2> &outer,
	float shell_thickness)
{
	float maximum_x = 0.0f;
	float minimum_y = outer.front().y;
	float maximum_y = outer.front().y;
	for (const glm::vec2 &point : outer) {
		maximum_x = std::max(maximum_x, std::fabs(point.x));
		minimum_y = std::min(minimum_y, point.y);
		maximum_y = std::max(maximum_y, point.y);
	}
	const float profile_height = maximum_y - minimum_y;
	const float center_y = (minimum_y + maximum_y) * 0.5f;
	const float x_scale = (maximum_x - shell_thickness) / maximum_x;
	const float y_scale = (profile_height - shell_thickness * 2.0f) / profile_height;
	std::vector<glm::vec2> inner;
	inner.reserve(outer.size());
	for (const glm::vec2 &point : outer) {
		inner.emplace_back(
			point.x * x_scale,
			center_y + (point.y - center_y) * y_scale);
	}
	return inner;
}

bool validate_section_set(
	const VehicleBodySpecification &specification,
	std::string *diagnostic)
{
	if (specification.objectIdentifier().empty() ||
	    specification.materialIdentifier().empty() ||
	    specification.sections().size() < 2u ||
	    !std::isfinite(specification.shellThickness()) ||
	    specification.shellThickness() <= 0.0f) {
		if (diagnostic != nullptr) {
			*diagnostic =
				"Vehicle body requires identifiers, at least two sections, and positive shell thickness.";
		}
		return false;
	}
	const std::size_t point_count =
		specification.sections().front().rightHalfProfile().size();
	if (point_count < 4u) {
		if (diagnostic != nullptr) {
			*diagnostic = "Vehicle body half-sections require at least four profile points.";
		}
		return false;
	}
	std::set<std::string> identifiers;
	for (const VehicleBodySection &section : specification.sections()) {
		if (section.identifier().empty() ||
		    !identifiers.insert(section.identifier()).second ||
		    !std::isfinite(section.stationZ()) ||
		    section.rightHalfProfile().size() != point_count) {
			if (diagnostic != nullptr) {
				*diagnostic =
					"Vehicle body sections require unique identifiers, finite stations, and corresponding profile points.";
			}
			return false;
		}
		const auto &profile = section.rightHalfProfile();
		if (std::fabs(profile.front().x) > 1.0e-5f ||
		    std::fabs(profile.back().x) > 1.0e-5f) {
			if (diagnostic != nullptr) {
				*diagnostic =
					"Vehicle body half-sections must begin and end on the symmetry plane.";
			}
			return false;
		}
		for (std::size_t point_index = 0u; point_index < profile.size(); ++point_index) {
			if (!finite(profile[point_index]) || profile[point_index].x < -1.0e-6f ||
			    (point_index > 0u && point_index + 1u < profile.size() &&
			     profile[point_index].x <= 0.0f)) {
				if (diagnostic != nullptr) {
					*diagnostic =
						"Vehicle body half-section coordinates must be finite and remain on the selected side of the symmetry plane.";
				}
				return false;
			}
		}
	}
	for (const VehicleSurfaceGuide &guide : specification.guides()) {
		if (guide.identifier().empty() || !guide.curve().isValid(diagnostic)) return false;
	}
	return true;
}

std::shared_ptr<const SweepDiskShapeSpecification> build_swept_curve_shape(
	const std::vector<glm::vec3> &path,
	float radius,
	GeometryDetailLevel detail_level,
	const GeometryComplexityLimits &complexity_limits,
	GeneratedPrimitiveMesh *mesh,
	std::string *diagnostic)
{
	SweepDiskShapeSpecificationCandidate candidate;
	candidate.path_points = path;
	candidate.radius = radius;
	candidate.circumferential_segments = 8;
	candidate.detail_level = detail_level;
	auto shape = SweepDiskSpecificationValidator(complexity_limits).validate(
		std::move(candidate), diagnostic);
	if (!shape) return {};
	GeometryBuildResult built = SweepDiskMeshGenerator(complexity_limits).build(*shape);
	if (!built.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = built.firstDiagnostic();
		return {};
	}
	*mesh = built.generatedMesh();
	return shape;
}

} // namespace

VehicleAssemblyBuildResult VehicleBodyAssemblyBuilder::build(
	const VehicleBodySpecification &specification) const
{
	std::string diagnostic;
	if (!validate_section_set(specification, &diagnostic)) {
		return VehicleAssemblyBuildResult::failed(diagnostic);
	}

	std::vector<VehicleBodySection> ordered_sections = specification.sections();
	std::sort(
		ordered_sections.begin(), ordered_sections.end(),
		[](const VehicleBodySection &first, const VehicleBodySection &second) {
			return first.stationZ() < second.stationZ();
		});
	for (std::size_t index = 1u; index < ordered_sections.size(); ++index) {
		if (ordered_sections[index].stationZ() -
		        ordered_sections[index - 1u].stationZ() <=
		    1.0e-5f) {
			return VehicleAssemblyBuildResult::failed(
				"Vehicle body section stations must be strictly increasing after canonical ordering.");
		}
	}

	ShellLoftShapeSpecificationCandidate shell_candidate;
	shell_candidate.detail_level = specification.detailLevel();
	for (const VehicleBodySection &section : ordered_sections) {
		const std::vector<glm::vec2> outer =
			create_symmetric_profile(section.rightHalfProfile());
		float maximum_x = 0.0f;
		float minimum_y = outer.front().y;
		float maximum_y = outer.front().y;
		for (const glm::vec2 &point : outer) {
			maximum_x = std::max(maximum_x, std::fabs(point.x));
			minimum_y = std::min(minimum_y, point.y);
			maximum_y = std::max(maximum_y, point.y);
		}
		if (maximum_x <= specification.shellThickness() * 2.0f ||
		    maximum_y - minimum_y <= specification.shellThickness() * 4.0f) {
			return VehicleAssemblyBuildResult::failed(
				"Vehicle body shell thickness is too large for at least one section.");
		}
		ShellLoftSectionCandidate shell_section;
		shell_section.axial_position = section.stationZ();
		shell_section.outer_loop = outer;
		shell_section.inner_loop = create_inner_profile(
			outer, specification.shellThickness());
		shell_candidate.sections.push_back(std::move(shell_section));
	}
	auto shell_shape = LoftSpecificationValidator(complexity_limits_).validateShellLoft(
		std::move(shell_candidate), &diagnostic);
	if (!shell_shape) return VehicleAssemblyBuildResult::failed(diagnostic);
	GeometryBuildResult shell_mesh = LoftMeshGenerator(complexity_limits_).build(
		*shell_shape);
	if (!shell_mesh.succeeded()) {
		return VehicleAssemblyBuildResult::failed(shell_mesh.firstDiagnostic());
	}

	std::vector<VehicleAssemblyPart> parts;
	parts.emplace_back(
		"body_shell", "BodyShell", specification.materialIdentifier(), shell_shape,
		shell_mesh.generatedMesh(), glm::mat4(1.0f),
		GeometryDetailRange(
			GeometryDetailLevel::CoarseShape,
			GeometryDetailLevel::FastenersAndSeals));

	const SpatialObjectId owner(specification.objectIdentifier());
	std::vector<SpatialInterface> interfaces;
	interfaces.emplace_back(
		SpatialInterfaceId("chassis_mount"), owner, SpatialInterfaceType::Mate,
		SpatialInterfaceFrame(
			glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
			glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::objectBoundaryFace("inner_floor"),
		InterfaceCompatibilityProfile(
			InterfaceShape::Rectangular, InterfaceGender::Neutral,
			std::nullopt, std::nullopt, std::nullopt, "vehicle-chassis-body"),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.003f));
	for (const auto &opening : std::vector<std::pair<std::string, glm::vec3>>{
		     {"windshield_opening", glm::vec3(0.0f, 1.08f, -0.54f)},
		     {"rear_window_opening", glm::vec3(0.0f, 1.10f, -2.48f)},
		     {"front_door_left_glass_opening", glm::vec3(-0.805f, 1.12f, -1.02f)},
		     {"front_door_right_glass_opening", glm::vec3(0.805f, 1.12f, -1.02f)},
		     {"rear_door_left_glass_opening", glm::vec3(-0.805f, 1.12f, -1.86f)},
		     {"rear_door_right_glass_opening", glm::vec3(0.805f, 1.12f, -1.86f)}}) {
		interfaces.emplace_back(
			SpatialInterfaceId(opening.first), owner, SpatialInterfaceType::Socket,
			SpatialInterfaceFrame(
				opening.second, glm::vec3(0.0f, 0.0f, 1.0f),
				glm::vec3(1.0f, 0.0f, 0.0f)),
			SpatialInterfaceRegion::objectBoundaryFace(opening.first),
			InterfaceCompatibilityProfile(
				InterfaceShape::Rectangular, InterfaceGender::Female,
				std::nullopt, std::nullopt, std::nullopt,
				"vehicle-glazing-seat"),
			SpatialClearanceRequirement(0.003f, 0.004f, 0.007f));
	}

	for (std::size_t arch_index = 0u;
	     arch_index < specification.wheelArches().size();
	     ++arch_index) {
			const WheelArchSpecification &arch = specification.wheelArches()[arch_index];
			if (arch.identifier().empty() || !finite(arch.wheelCenter()) ||
			    !std::isfinite(arch.tireRadius()) || !std::isfinite(arch.clearance()) ||
			    !std::isfinite(arch.archWidth()) || !std::isfinite(arch.flare()) ||
			    arch.tireRadius() <= 0.0f || arch.clearance() < 0.0f ||
			    arch.archWidth() <= 0.0f || arch.archRadius() < arch.tireRadius()) {
				return VehicleAssemblyBuildResult::failed(
					"Vehicle wheel arches require finite dimensions and radius >= tire radius + clearance.");
			}
			if (geometryDetailLevelRank(specification.detailLevel()) >=
			    geometryDetailLevelRank(GeometryDetailLevel::Assembly)) {
				std::vector<glm::vec3> path;
				path.reserve(17u);
				for (int sample_index = 0; sample_index <= 16; ++sample_index) {
					const float angle = glm::radians(
						15.0f + 150.0f * static_cast<float>(sample_index) / 16.0f);
					path.emplace_back(
						arch.wheelCenter().x,
						arch.wheelCenter().y + arch.archRadius() * std::sin(angle),
						arch.wheelCenter().z + arch.archRadius() * std::cos(angle));
				}
				GeneratedPrimitiveMesh arch_mesh(std::make_shared<Mesh>(), {});
				auto arch_shape = build_swept_curve_shape(
					path, 0.012f + arch.flare() * 0.2f,
					specification.detailLevel(), complexity_limits_, &arch_mesh, &diagnostic);
				if (!arch_shape) return VehicleAssemblyBuildResult::failed(diagnostic);
				parts.emplace_back(
					"wheel_arch_" + std::to_string(arch_index), "WheelArchEdge",
					specification.materialIdentifier(), arch_shape, arch_mesh,
					glm::mat4(1.0f),
					GeometryDetailRange(
						GeometryDetailLevel::Assembly,
						GeometryDetailLevel::FastenersAndSeals));
			}
			interfaces.emplace_back(
				SpatialInterfaceId(arch.identifier()), owner, SpatialInterfaceType::Socket,
				SpatialInterfaceFrame(
					arch.wheelCenter(), glm::vec3(1.0f, 0.0f, 0.0f),
					glm::vec3(0.0f, 1.0f, 0.0f)),
				SpatialInterfaceRegion::axisSegment(arch.archWidth()),
				InterfaceCompatibilityProfile(
					InterfaceShape::Circular, InterfaceGender::Female,
					arch.archRadius() * 2.0f, std::nullopt, std::nullopt,
					"vehicle-wheel-arch"),
				SpatialClearanceRequirement(
					arch.clearance(), arch.clearance(), arch.clearance() + 0.02f));
	}

	std::set<std::string> panel_identifiers;
	for (std::size_t panel_index = 0u;
	     panel_index < specification.panelCuts().size();
	     ++panel_index) {
			const VehiclePanelCutSpecification &panel = specification.panelCuts()[panel_index];
			if (panel.identifier().empty() ||
			    !panel_identifiers.insert(panel.identifier()).second ||
			    !panel.seamCurve().isValid(&diagnostic) ||
			    !std::isfinite(panel.gapWidth()) ||
			    !std::isfinite(panel.gapDepth()) || panel.gapWidth() <= 0.0f ||
			    panel.gapDepth() <= 0.0f) {
				return VehicleAssemblyBuildResult::failed(
					diagnostic.empty()
						? "Vehicle panel cuts require unique identifiers, valid curves, and positive gap dimensions."
						: diagnostic);
			}
			if (geometryDetailLevelRank(specification.detailLevel()) >=
			    geometryDetailLevelRank(GeometryDetailLevel::Component)) {
				GeneratedPrimitiveMesh gap_mesh(std::make_shared<Mesh>(), {});
				auto gap_shape = build_swept_curve_shape(
					panel.seamCurve().sample(16u), panel.gapWidth() * 0.5f,
					specification.detailLevel(), complexity_limits_, &gap_mesh, &diagnostic);
				if (!gap_shape) return VehicleAssemblyBuildResult::failed(diagnostic);
				parts.emplace_back(
					"panel_gap_" + std::to_string(panel_index), "PanelGap",
					"dark-rubber", gap_shape, gap_mesh, glm::mat4(1.0f),
					GeometryDetailRange(
						GeometryDetailLevel::Component,
						GeometryDetailLevel::FastenersAndSeals));
			}
			interfaces.emplace_back(
				SpatialInterfaceId(panel.identifier()), owner, SpatialInterfaceType::Seal,
				SpatialInterfaceFrame(
					panel.seamCurve().evaluate(0.5f),
					glm::vec3(0.0f, 0.0f, 1.0f),
					glm::normalize(panel.seamCurve().evaluateDerivative(0.5f))),
				SpatialInterfaceRegion::objectBoundaryFace(panel.identifier()),
				InterfaceCompatibilityProfile(
					InterfaceShape::Unspecified, InterfaceGender::Neutral,
					std::nullopt, std::nullopt, std::nullopt,
					"vehicle-panel-seal"),
				SpatialClearanceRequirement(
					panel.gapWidth(), panel.gapWidth() * 0.8f,
					panel.gapWidth() * 1.2f));
	}

	GeometryBuildResult combined =
		VehicleAssemblyCompositionService(complexity_limits_).compose(parts);
	if (!combined.succeeded()) {
		return VehicleAssemblyBuildResult::failed(combined.firstDiagnostic());
	}
	return VehicleAssemblyBuildResult::succeeded(
		VehicleAssemblyGeometry(
			std::move(parts), combined.generatedMesh(), std::move(interfaces),
			specification.detailLevel()));
}
