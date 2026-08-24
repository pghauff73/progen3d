#include "vehicle/service/VehicleInteriorAssemblyBuilder.h"

#include "spatial/model/InterfaceCompatibilityProfile.h"
#include "spatial/model/SpatialClearanceRequirement.h"
#include "spatial/model/SpatialInterfaceFrame.h"
#include "spatial/model/SpatialInterfaceRegion.h"
#include "vehicle/service/VehicleAssemblyCompositionService.h"
#include "vehicle/service/VehiclePrimitivePartFactory.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

VehicleAssemblyBuildResult compose_assembly(
	std::vector<VehicleAssemblyPart> parts,
	std::vector<SpatialInterface> interfaces,
	GeometryDetailLevel detail_level,
	const GeometryComplexityLimits &complexity_limits)
{
	if (parts.empty()) {
		return VehicleAssemblyBuildResult::succeeded(VehicleAssemblyGeometry(
			{}, GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {}),
			std::move(interfaces), detail_level));
	}
	GeometryBuildResult combined =
		VehicleAssemblyCompositionService(complexity_limits).compose(parts);
	if (!combined.succeeded()) {
		return VehicleAssemblyBuildResult::failed(combined.firstDiagnostic());
	}
	return VehicleAssemblyBuildResult::succeeded(VehicleAssemblyGeometry(
		std::move(parts), combined.generatedMesh(), std::move(interfaces),
		detail_level));
}

bool append_part(
	const VehiclePrimitivePartBuildResult &result,
	std::vector<VehicleAssemblyPart> *parts,
	std::string *diagnostic)
{
	if (!result.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = result.diagnostic();
		return false;
	}
	parts->push_back(*result.part());
	return true;
}

std::vector<SpatialInterface> seat_interfaces(const std::string &identifier)
{
	const SpatialObjectId owner(identifier);
	return {
		SpatialInterface(
			SpatialInterfaceId("rail_left"), owner,
			SpatialInterfaceType::Slide,
			SpatialInterfaceFrame(
				glm::vec3(-0.18f, -0.16f, 0.0f),
				glm::vec3(0.0f, -1.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, 1.0f)),
			SpatialInterfaceRegion::axisSegment(0.42f),
			InterfaceCompatibilityProfile(
				InterfaceShape::Axis, InterfaceGender::Male,
				std::nullopt, std::nullopt, std::nullopt,
				"vehicle-seat-rail"),
			SpatialClearanceRequirement(0.0f, 0.0f, 0.002f)),
		SpatialInterface(
			SpatialInterfaceId("rail_right"), owner,
			SpatialInterfaceType::Slide,
			SpatialInterfaceFrame(
				glm::vec3(0.18f, -0.16f, 0.0f),
				glm::vec3(0.0f, -1.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, 1.0f)),
			SpatialInterfaceRegion::axisSegment(0.42f),
			InterfaceCompatibilityProfile(
				InterfaceShape::Axis, InterfaceGender::Male,
				std::nullopt, std::nullopt, std::nullopt,
				"vehicle-seat-rail"),
			SpatialClearanceRequirement(0.0f, 0.0f, 0.002f)),
		SpatialInterface(
			SpatialInterfaceId("occupant_reference"), owner,
			SpatialInterfaceType::Mate,
			SpatialInterfaceFrame(
				glm::vec3(0.0f, 0.05f, 0.0f),
				glm::vec3(0.0f, 1.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, 1.0f)),
			SpatialInterfaceRegion::point(),
			InterfaceCompatibilityProfile(
				InterfaceShape::Point, InterfaceGender::Neutral,
				std::nullopt, std::nullopt, std::nullopt,
				"vehicle-occupant-hip-point"),
			SpatialClearanceRequirement(0.0f, 0.0f, 0.005f))};
}

std::vector<SpatialInterface> occupant_interfaces(
	const VehicleOccupantEnvelope &occupant)
{
	const SpatialObjectId owner(occupant.identifier());
	return {
		SpatialInterface(
			SpatialInterfaceId("hip_point"), owner,
			SpatialInterfaceType::Mate,
			SpatialInterfaceFrame(
				glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, 1.0f)),
			SpatialInterfaceRegion::point(),
			InterfaceCompatibilityProfile(
				InterfaceShape::Point, InterfaceGender::Neutral,
				std::nullopt, std::nullopt, std::nullopt,
				"vehicle-occupant-hip-point"),
			SpatialClearanceRequirement(0.0f, 0.0f, 0.005f)),
		SpatialInterface(
			SpatialInterfaceId("eye_point"), owner,
			SpatialInterfaceType::InspectionInterface,
			SpatialInterfaceFrame(
				occupant.eyePoint() - occupant.hipPoint(),
				glm::vec3(0.0f, 0.0f, 1.0f),
				glm::vec3(1.0f, 0.0f, 0.0f)),
			SpatialInterfaceRegion::point(),
			InterfaceCompatibilityProfile(
				InterfaceShape::Point, InterfaceGender::Neutral,
				std::nullopt, std::nullopt, std::nullopt,
				"vehicle-eye-point"),
			SpatialClearanceRequirement(
				occupant.headClearance(), occupant.headClearance(),
				occupant.headClearance() + 0.02f))};
}

} // namespace

VehicleSubsystemAssemblyBuildResult VehicleInteriorAssemblyBuilder::build(
	const VehicleInteriorSpecification &specification,
	GeometryDetailLevel detail_level) const
{
	if (specification.occupantEnvelopes().size() < 3u) {
		return VehicleSubsystemAssemblyBuildResult::failed(
			"Vehicle interior requires driver, front passenger, and rear passenger envelopes.");
	}
	VehiclePrimitivePartFactory part_factory(complexity_limits_);
	std::vector<VehiclePlacedAssembly> assemblies;
	std::string diagnostic;

	for (const VehicleOccupantEnvelope &occupant :
	     specification.occupantEnvelopes()) {
		VehicleAssemblyBuildResult built = compose_assembly(
			{}, occupant_interfaces(occupant), detail_level, complexity_limits_);
		assemblies.emplace_back(
			occupant.identifier(), *built.geometry(),
			glm::translate(glm::mat4(1.0f), occupant.hipPoint()));
	}

	auto add_seat = [&](
		const std::string &identifier,
		glm::vec3 position,
		float width,
		int headrest_count) -> bool {
		std::vector<VehicleAssemblyPart> parts;
		if (geometryDetailLevelRank(detail_level) >=
		    geometryDetailLevelRank(GeometryDetailLevel::Assembly)) {
			if (!append_part(
					part_factory.buildCenteredRoundedBox(
						"lower_cushion", "SeatLowerCushion", "seat-upholstery",
						glm::vec3(width, 0.16f, 0.48f), 0.055f,
						glm::mat4(1.0f),
						GeometryDetailRange(
							GeometryDetailLevel::Assembly,
							GeometryDetailLevel::FastenersAndSeals),
						detail_level),
					&parts, &diagnostic) ||
			    !append_part(
					part_factory.buildCenteredRoundedBox(
						"back_cushion", "SeatBackCushion", "seat-upholstery",
						glm::vec3(width, 0.58f, 0.14f), 0.050f,
						glm::translate(
							glm::mat4(1.0f), glm::vec3(0.0f, 0.28f, 0.16f)),
						GeometryDetailRange(
							GeometryDetailLevel::Assembly,
							GeometryDetailLevel::FastenersAndSeals),
						detail_level),
					&parts, &diagnostic)) return false;
		}
		if (geometryDetailLevelRank(detail_level) >=
		    geometryDetailLevelRank(GeometryDetailLevel::Component)) {
			for (int headrest_index = 0;
			     headrest_index < headrest_count;
			     ++headrest_index) {
				const float fraction = headrest_count == 1
					? 0.0f
					: static_cast<float>(headrest_index) /
					      static_cast<float>(headrest_count - 1) - 0.5f;
				if (!append_part(
						part_factory.buildCenteredRoundedBox(
							"headrest_" + std::to_string(headrest_index),
							"SeatHeadrest", "seat-upholstery",
							glm::vec3(0.24f, 0.18f, 0.10f), 0.035f,
							glm::translate(
								glm::mat4(1.0f),
								glm::vec3(
									fraction * (width - 0.24f), 0.68f, 0.18f)),
							GeometryDetailRange(
								GeometryDetailLevel::Component,
								GeometryDetailLevel::FastenersAndSeals),
							detail_level),
						&parts, &diagnostic)) return false;
			}
		}
		if (geometryDetailLevelRank(detail_level) >=
		    geometryDetailLevelRank(GeometryDetailLevel::ConstructionDetail)) {
			for (int rail_index = 0; rail_index < 2; ++rail_index) {
				const float x = rail_index == 0 ? -width * 0.28f : width * 0.28f;
				if (!append_part(
						part_factory.buildCenteredRoundedBox(
							"rail_" + std::to_string(rail_index), "SeatRail",
							"seat-frame-metal", glm::vec3(0.035f, 0.035f, 0.44f),
							0.008f,
							glm::translate(
								glm::mat4(1.0f), glm::vec3(x, -0.115f, 0.0f)),
							GeometryDetailRange(
								GeometryDetailLevel::ConstructionDetail,
								GeometryDetailLevel::FastenersAndSeals),
							detail_level),
						&parts, &diagnostic)) return false;
			}
		}
		VehicleAssemblyBuildResult built = compose_assembly(
			std::move(parts), seat_interfaces(identifier), detail_level,
			complexity_limits_);
		if (!built.succeeded()) {
			diagnostic = built.diagnostic();
			return false;
		}
		assemblies.emplace_back(
			identifier, *built.geometry(),
			glm::translate(glm::mat4(1.0f), position));
		return true;
	};

	if (!add_seat("DriverSeat", glm::vec3(-0.43f, 0.52f, -1.12f), 0.48f, 1) ||
	    !add_seat("FrontPassengerSeat", glm::vec3(0.43f, 0.52f, -1.12f), 0.48f, 1) ||
	    !add_seat("RearSeatBench", glm::vec3(0.0f, 0.52f, -2.20f), 1.34f, 3)) {
		return VehicleSubsystemAssemblyBuildResult::failed(diagnostic);
	}

	auto add_fixed_cabin_assembly = [&](
		const std::string &identifier,
		std::vector<VehicleAssemblyPart> parts,
		glm::vec3 position,
		std::vector<SpatialInterface> interfaces) -> bool {
		VehicleAssemblyBuildResult built = compose_assembly(
			std::move(parts), std::move(interfaces), detail_level,
			complexity_limits_);
		if (!built.succeeded()) {
			diagnostic = built.diagnostic();
			return false;
		}
		assemblies.emplace_back(
			identifier, *built.geometry(),
			glm::translate(glm::mat4(1.0f), position));
		return true;
	};

	{
		std::vector<VehicleAssemblyPart> parts;
		if (geometryDetailLevelRank(detail_level) >=
		    geometryDetailLevelRank(GeometryDetailLevel::Assembly)) {
			if (!append_part(
					part_factory.buildCenteredRoundedBox(
						"dashboard_shell", "DashboardShell", "dashboard-soft-touch",
						glm::vec3(1.56f, 0.28f, 0.34f), 0.075f,
						glm::mat4(1.0f),
						GeometryDetailRange(
							GeometryDetailLevel::Assembly,
							GeometryDetailLevel::FastenersAndSeals),
						detail_level),
					&parts, &diagnostic)) {
				return VehicleSubsystemAssemblyBuildResult::failed(diagnostic);
			}
		}
		if (geometryDetailLevelRank(detail_level) >=
		    geometryDetailLevelRank(GeometryDetailLevel::Component)) {
			if (!append_part(
					part_factory.buildCenteredRoundedBox(
						"instrument_display", "InstrumentDisplay", "display-glass",
						glm::vec3(0.36f, 0.14f, 0.018f), 0.025f,
						glm::translate(
							glm::mat4(1.0f), glm::vec3(-0.42f, 0.035f, 0.18f)),
						GeometryDetailRange(
							GeometryDetailLevel::Component,
							GeometryDetailLevel::FastenersAndSeals),
						detail_level),
					&parts, &diagnostic) ||
			    !append_part(
					part_factory.buildCenteredRoundedBox(
						"centre_display", "CentreDisplay", "display-glass",
						glm::vec3(0.25f, 0.20f, 0.018f), 0.025f,
						glm::translate(
							glm::mat4(1.0f), glm::vec3(0.08f, 0.05f, 0.18f)),
						GeometryDetailRange(
							GeometryDetailLevel::Component,
							GeometryDetailLevel::FastenersAndSeals),
						detail_level),
					&parts, &diagnostic)) {
				return VehicleSubsystemAssemblyBuildResult::failed(diagnostic);
			}
		}
		const SpatialObjectId owner("Dashboard");
		std::vector<SpatialInterface> interfaces = {
			SpatialInterface(
				SpatialInterfaceId("body_mount"), owner,
				SpatialInterfaceType::FixedTo,
				SpatialInterfaceFrame(
					glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
					glm::vec3(1.0f, 0.0f, 0.0f)),
				SpatialInterfaceRegion::point(),
				InterfaceCompatibilityProfile(
					InterfaceShape::Point, InterfaceGender::Male,
					std::nullopt, std::nullopt, std::nullopt,
					"vehicle-cabin-mount"),
				SpatialClearanceRequirement(0.0f, 0.0f, 0.003f)),
			SpatialInterface(
				SpatialInterfaceId("steering_column_mount"), owner,
				SpatialInterfaceType::Socket,
				SpatialInterfaceFrame(
					glm::vec3(-0.42f, -0.02f, 0.18f),
					glm::vec3(0.0f, 0.0f, 1.0f),
					glm::vec3(0.0f, 1.0f, 0.0f)),
				SpatialInterfaceRegion::axisSegment(0.18f),
				InterfaceCompatibilityProfile(
					InterfaceShape::Axis, InterfaceGender::Female,
					std::nullopt, std::nullopt, std::nullopt,
					"vehicle-steering-axis"),
				SpatialClearanceRequirement(0.0f, 0.0f, 0.002f))};
		if (!add_fixed_cabin_assembly(
				"Dashboard", std::move(parts), specification.dashboardOrigin(),
				std::move(interfaces))) {
			return VehicleSubsystemAssemblyBuildResult::failed(diagnostic);
		}
	}

	{
		std::vector<VehicleAssemblyPart> parts;
		if (geometryDetailLevelRank(detail_level) >=
		    geometryDetailLevelRank(GeometryDetailLevel::Assembly)) {
			if (!append_part(
					part_factory.buildCenteredRoundedBox(
						"console_shell", "CentreConsole", "dashboard-soft-touch",
						glm::vec3(0.28f, 0.28f, 0.88f), 0.065f,
						glm::mat4(1.0f),
						GeometryDetailRange(
							GeometryDetailLevel::Assembly,
							GeometryDetailLevel::FastenersAndSeals),
						detail_level),
					&parts, &diagnostic)) {
				return VehicleSubsystemAssemblyBuildResult::failed(diagnostic);
			}
		}
		if (!add_fixed_cabin_assembly(
				"CentreConsole", std::move(parts),
				specification.centreConsoleOrigin(), {})) {
			return VehicleSubsystemAssemblyBuildResult::failed(diagnostic);
		}
	}

	{
		std::vector<VehicleAssemblyPart> parts;
		if (geometryDetailLevelRank(detail_level) >=
		    geometryDetailLevelRank(GeometryDetailLevel::Assembly)) {
			std::vector<glm::vec2> torus_profile;
			for (int index = 0; index < 14; ++index) {
				const float angle = glm::two_pi<float>() *
				                    static_cast<float>(index) / 14.0f;
				torus_profile.emplace_back(
					0.165f + 0.018f * std::cos(angle),
					0.018f * std::sin(angle));
			}
			const glm::mat4 wheel_orientation = glm::rotate(
				glm::mat4(1.0f), glm::radians(90.0f),
				glm::vec3(1.0f, 0.0f, 0.0f));
			if (!append_part(
					part_factory.buildRevolvedSolid(
						"wheel_rim", "SteeringWheelRim", "steering-wheel-leather",
						std::move(torus_profile), 40, wheel_orientation,
						GeometryDetailRange(
							GeometryDetailLevel::Assembly,
							GeometryDetailLevel::FastenersAndSeals),
						detail_level),
					&parts, &diagnostic) ||
			    !append_part(
					part_factory.buildSweepDisk(
						"steering_column", "SteeringColumn", "steering-metal",
						{{0.0f, 0.0f, -0.18f}, {0.0f, 0.0f, 0.0f}}, 0.018f,
						glm::mat4(1.0f),
						GeometryDetailRange(
							GeometryDetailLevel::Assembly,
							GeometryDetailLevel::FastenersAndSeals),
						detail_level),
					&parts, &diagnostic)) {
				return VehicleSubsystemAssemblyBuildResult::failed(diagnostic);
			}
		}
		const SpatialObjectId owner("SteeringWheel");
		std::vector<SpatialInterface> interfaces = {
			SpatialInterface(
				SpatialInterfaceId("steering_axis"), owner,
				SpatialInterfaceType::Shaft,
				SpatialInterfaceFrame(
					glm::vec3(0.0f, 0.0f, -0.18f),
					glm::vec3(0.0f, 0.0f, 1.0f),
					glm::vec3(0.0f, 1.0f, 0.0f)),
				SpatialInterfaceRegion::axisSegment(0.18f),
				InterfaceCompatibilityProfile(
					InterfaceShape::Axis, InterfaceGender::Male,
					std::nullopt, std::nullopt, std::nullopt,
					"vehicle-steering-axis"),
				SpatialClearanceRequirement(0.0f, 0.0f, 0.002f))};
		if (!add_fixed_cabin_assembly(
				"SteeringWheel", std::move(parts),
				specification.steeringWheelOrigin(), std::move(interfaces))) {
			return VehicleSubsystemAssemblyBuildResult::failed(diagnostic);
		}
	}

	return VehicleSubsystemAssemblyBuildResult::succeeded(
		VehicleSubsystemAssemblySet(std::move(assemblies)));
}
