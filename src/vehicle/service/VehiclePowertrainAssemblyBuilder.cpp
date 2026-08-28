#include "vehicle/service/VehiclePowertrainAssemblyBuilder.h"

#include "spatial/model/InterfaceCompatibilityProfile.h"
#include "spatial/model/SpatialClearanceRequirement.h"
#include "spatial/model/SpatialInterfaceFrame.h"
#include "spatial/model/SpatialInterfaceRegion.h"
#include "vehicle/service/VehicleAssemblyCompositionService.h"
#include "vehicle/service/VehiclePrimitivePartFactory.h"

#include <glm/gtc/matrix_transform.hpp>

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

SpatialInterface point_interface(
	const std::string &owner_identifier,
	const std::string &interface_identifier,
	SpatialInterfaceType type,
	const std::string &compatibility_class,
	InterfaceGender gender,
	glm::vec3 origin,
	glm::vec3 normal)
{
	return SpatialInterface(
		SpatialInterfaceId(interface_identifier), SpatialObjectId(owner_identifier),
		type,
		SpatialInterfaceFrame(origin, normal, glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::point(),
		InterfaceCompatibilityProfile(
			InterfaceShape::Point, gender,
			std::nullopt, std::nullopt, std::nullopt, compatibility_class),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.003f));
}

} // namespace

VehicleSubsystemAssemblyBuildResult VehiclePowertrainAssemblyBuilder::build(
	const VehiclePowertrainSpecification &specification,
	GeometryDetailLevel detail_level) const
{
	VehiclePrimitivePartFactory part_factory(complexity_limits_);
	std::vector<VehiclePlacedAssembly> assemblies;
	std::string diagnostic;

	{
		std::vector<VehicleAssemblyPart> parts;
		if (geometryDetailLevelRank(detail_level) >=
		    geometryDetailLevelRank(GeometryDetailLevel::ConstructionDetail)) {
			if (!append_part(
					part_factory.buildCenteredRoundedBox(
						"floor_pan", "UnderbodyFloorPan", "underbody-composite",
						specification.underbodyDimensions(), 0.055f,
						glm::mat4(1.0f),
						GeometryDetailRange(
							GeometryDetailLevel::ConstructionDetail,
							GeometryDetailLevel::FastenersAndSeals),
						detail_level),
					&parts, &diagnostic) ||
			    !append_part(
					part_factory.buildCenteredRoundedBox(
						"front_subframe", "FrontSubframe", "subframe-metal",
						glm::vec3(1.20f, 0.10f, 0.48f), 0.035f,
						glm::translate(
							glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 1.13f)),
						GeometryDetailRange(
							GeometryDetailLevel::ConstructionDetail,
							GeometryDetailLevel::FastenersAndSeals),
						detail_level),
					&parts, &diagnostic) ||
			    !append_part(
					part_factory.buildCenteredRoundedBox(
						"rear_subframe", "RearSubframe", "subframe-metal",
						glm::vec3(1.18f, 0.10f, 0.48f), 0.035f,
						glm::translate(
							glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -1.32f)),
						GeometryDetailRange(
							GeometryDetailLevel::ConstructionDetail,
							GeometryDetailLevel::FastenersAndSeals),
						detail_level),
					&parts, &diagnostic)) {
				return VehicleSubsystemAssemblyBuildResult::failed(diagnostic);
			}
		}
		const SpatialObjectId owner("Underbody");
		std::vector<SpatialInterface> interfaces = {
			SpatialInterface(
				SpatialInterfaceId("cabin_floor_upper"), owner,
				SpatialInterfaceType::Support,
				SpatialInterfaceFrame(
					glm::vec3(0.0f, specification.underbodyDimensions().y * 0.5f, 0.0f),
					glm::vec3(0.0f, 1.0f, 0.0f),
					glm::vec3(1.0f, 0.0f, 0.0f)),
				SpatialInterfaceRegion::planeRectangle(
					specification.underbodyDimensions().x,
					specification.underbodyDimensions().z),
				InterfaceCompatibilityProfile(
					InterfaceShape::Rectangular, InterfaceGender::Female,
					std::nullopt, std::nullopt, std::nullopt,
					"vehicle-seat-floor"),
				SpatialClearanceRequirement(0.0f, 0.0f, 0.002f)),
			SpatialInterface(
				SpatialInterfaceId("battery_bay"), owner,
				SpatialInterfaceType::Seat,
				SpatialInterfaceFrame(
					glm::vec3(0.0f, -specification.underbodyDimensions().y * 0.5f, 0.0f),
					glm::vec3(0.0f, -1.0f, 0.0f),
					glm::vec3(1.0f, 0.0f, 0.0f)),
				SpatialInterfaceRegion::planeRectangle(1.36f, 2.18f),
				InterfaceCompatibilityProfile(
					InterfaceShape::Rectangular, InterfaceGender::Female,
					std::nullopt, std::nullopt, std::nullopt,
					"vehicle-battery-mount"),
				SpatialClearanceRequirement(0.0f, 0.0f, 0.003f))};
		VehicleAssemblyBuildResult built = compose_assembly(
			std::move(parts), std::move(interfaces), detail_level,
			complexity_limits_);
		if (!built.succeeded()) {
			return VehicleSubsystemAssemblyBuildResult::failed(built.diagnostic());
		}
		assemblies.emplace_back(
			"Underbody", *built.geometry(),
			glm::translate(
				glm::mat4(1.0f), specification.underbodyOrigin()));
	}

	const VehiclePackageEnvelope &storage =
		specification.energyStorageEnvelope();
	const glm::vec3 storage_dimensions = storage.maximum() - storage.minimum();
	const glm::vec3 storage_origin =
		(storage.minimum() + storage.maximum()) * 0.5f;
	{
		const bool electric = specification.powertrainType() ==
			VehiclePowertrainType::BatteryElectric;
		const std::string identifier = electric ? "BatteryPack" : "FuelTank";
		std::vector<VehicleAssemblyPart> parts;
		if (geometryDetailLevelRank(detail_level) >=
		    geometryDetailLevelRank(GeometryDetailLevel::Assembly)) {
			if (!append_part(
					part_factory.buildCenteredRoundedBox(
						"lower_tray", electric ? "BatteryLowerTray" : "FuelTankShell",
						electric ? "battery-tray" : "fuel-tank-polymer",
						storage_dimensions, 0.045f, glm::mat4(1.0f),
						GeometryDetailRange(
							GeometryDetailLevel::Assembly,
							GeometryDetailLevel::FastenersAndSeals),
						detail_level),
					&parts, &diagnostic)) {
				return VehicleSubsystemAssemblyBuildResult::failed(diagnostic);
			}
		}
		if (electric && geometryDetailLevelRank(detail_level) >=
		    geometryDetailLevelRank(GeometryDetailLevel::ConstructionDetail)) {
			VehiclePrimitivePartBuildResult module_source =
				part_factory.buildCenteredRoundedBox(
					"module_source", "BatteryModule", "battery-module",
					glm::vec3(0.23f, 0.10f, 0.34f), 0.025f,
					glm::mat4(1.0f),
					GeometryDetailRange(
						GeometryDetailLevel::ConstructionDetail,
						GeometryDetailLevel::FastenersAndSeals),
					detail_level);
			if (!module_source.succeeded()) {
				return VehicleSubsystemAssemblyBuildResult::failed(
					module_source.diagnostic());
			}
			std::vector<glm::mat4> module_transforms;
			for (int z_index = 0; z_index < 5; ++z_index) {
				for (int x_index = 0; x_index < 5; ++x_index) {
					module_transforms.push_back(glm::translate(
						glm::mat4(1.0f),
						glm::vec3(
							-0.48f + 0.24f * static_cast<float>(x_index),
							0.02f,
							-0.74f + 0.37f * static_cast<float>(z_index))));
				}
			}
			if (!append_part(
					part_factory.buildInstanceArray(
						"module_grid", "BatteryModuleGrid", "battery-module",
						*module_source.part(), std::move(module_transforms),
						glm::mat4(1.0f),
						GeometryDetailRange(
							GeometryDetailLevel::ConstructionDetail,
							GeometryDetailLevel::FastenersAndSeals),
						detail_level),
					&parts, &diagnostic) ||
			    !append_part(
					part_factory.buildCenteredRoundedBox(
						"cooling_plate", "BatteryCoolingPlate", "cooling-aluminium",
						glm::vec3(1.20f, 0.025f, 1.82f), 0.010f,
						glm::translate(
							glm::mat4(1.0f), glm::vec3(0.0f, -0.075f, 0.0f)),
						GeometryDetailRange(
							GeometryDetailLevel::ConstructionDetail,
							GeometryDetailLevel::FastenersAndSeals),
						detail_level),
					&parts, &diagnostic)) {
				return VehicleSubsystemAssemblyBuildResult::failed(diagnostic);
			}
		}
		std::vector<SpatialInterface> interfaces = {
			point_interface(
				identifier, "chassis_mount", SpatialInterfaceType::Mate,
				"vehicle-battery-mount", InterfaceGender::Male,
				glm::vec3(0.0f, storage_dimensions.y * 0.5f, 0.0f),
				glm::vec3(0.0f, 1.0f, 0.0f))};
		if (electric) {
			interfaces.push_back(point_interface(
				identifier, "hv_port", SpatialInterfaceType::ElectricalPort,
				"vehicle-high-voltage", InterfaceGender::Female,
				glm::vec3(0.52f, 0.0f, 0.82f), glm::vec3(0.0f, 0.0f, 1.0f)));
			interfaces.push_back(point_interface(
				identifier, "coolant_in", SpatialInterfaceType::ThermalInterface,
				"vehicle-battery-coolant", InterfaceGender::Female,
				glm::vec3(-0.52f, 0.0f, 0.82f), glm::vec3(0.0f, 0.0f, 1.0f)));
			interfaces.push_back(point_interface(
				identifier, "coolant_out", SpatialInterfaceType::ThermalInterface,
				"vehicle-battery-coolant", InterfaceGender::Male,
				glm::vec3(-0.42f, 0.0f, 0.82f), glm::vec3(0.0f, 0.0f, 1.0f)));
		}
		VehicleAssemblyBuildResult built = compose_assembly(
			std::move(parts), std::move(interfaces), detail_level,
			complexity_limits_);
		if (!built.succeeded()) {
			return VehicleSubsystemAssemblyBuildResult::failed(built.diagnostic());
		}
		assemblies.emplace_back(
			identifier, *built.geometry(),
			glm::translate(glm::mat4(1.0f), storage_origin));
	}

	for (const VehicleDriveUnitSpecification &drive_unit :
	     specification.driveUnits()) {
		std::vector<VehicleAssemblyPart> parts;
		if (geometryDetailLevelRank(detail_level) >=
		    geometryDetailLevelRank(GeometryDetailLevel::Component)) {
			if (specification.powertrainType() ==
			    VehiclePowertrainType::BatteryElectric) {
				if (!append_part(
						part_factory.buildRevolvedSolid(
							"motor", "ElectricMotor", "motor-metal",
							{{0.0f, -0.14f}, {0.18f, -0.14f},
							 {0.18f, 0.14f}, {0.0f, 0.14f}},
							32, glm::rotate(
								glm::mat4(1.0f), glm::radians(90.0f),
								glm::vec3(0.0f, 0.0f, 1.0f)),
							GeometryDetailRange(
								GeometryDetailLevel::Component,
								GeometryDetailLevel::FastenersAndSeals),
							detail_level),
						&parts, &diagnostic) ||
				    !append_part(
						part_factory.buildCenteredRoundedBox(
							"reducer", "EDriveReducer", "motor-metal",
							glm::vec3(0.24f, 0.22f, 0.26f), 0.045f,
							glm::translate(
								glm::mat4(1.0f), glm::vec3(0.22f, 0.0f, 0.0f)),
							GeometryDetailRange(
								GeometryDetailLevel::Component,
								GeometryDetailLevel::FastenersAndSeals),
							detail_level),
						&parts, &diagnostic)) {
					return VehicleSubsystemAssemblyBuildResult::failed(diagnostic);
				}
			}
			else if (!append_part(
					part_factory.buildCenteredRoundedBox(
						"engine_block", "InternalCombustionEnginePlaceholder",
						"engine-metal", glm::vec3(0.72f, 0.46f, 0.62f),
						0.075f, glm::mat4(1.0f),
						GeometryDetailRange(
							GeometryDetailLevel::Component,
							GeometryDetailLevel::FastenersAndSeals),
						detail_level),
					&parts, &diagnostic)) {
				return VehicleSubsystemAssemblyBuildResult::failed(diagnostic);
			}
		}
		std::vector<SpatialInterface> interfaces = {
			point_interface(
				drive_unit.identifier(), "chassis_mount", SpatialInterfaceType::Mate,
				"vehicle-drive-unit-mount", InterfaceGender::Male,
				glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
			point_interface(
				drive_unit.identifier(), "axle_output", SpatialInterfaceType::Shaft,
				"vehicle-axle-output", InterfaceGender::Male,
				glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f))};
		if (specification.powertrainType() ==
		    VehiclePowertrainType::BatteryElectric) {
			interfaces.push_back(point_interface(
				drive_unit.identifier(), "hv_port",
				SpatialInterfaceType::ElectricalPort,
				"vehicle-high-voltage", InterfaceGender::Male,
				glm::vec3(0.0f, 0.16f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
		}
		VehicleAssemblyBuildResult built = compose_assembly(
			std::move(parts), std::move(interfaces), detail_level,
			complexity_limits_);
		if (!built.succeeded()) {
			return VehicleSubsystemAssemblyBuildResult::failed(built.diagnostic());
		}
		assemblies.emplace_back(
			drive_unit.identifier(), *built.geometry(),
			glm::translate(glm::mat4(1.0f), drive_unit.origin()));
	}

	return VehicleSubsystemAssemblyBuildResult::succeeded(
		VehicleSubsystemAssemblySet(std::move(assemblies)));
}
