#include "spatial/energy/SpatialEnergyWarnings.h"

#include <cassert>
#include <initializer_list>
#include <iostream>
#include <string>

namespace {

using Interface = SpatialInterfaceType;
using Connection = SpatialConnectionType;
using PortSpec = std::pair<const char *, Interface>;

SpatialBuildingObject device(const char *id, const char *object_class,
                             std::vector<std::string> taxonomy,
                             std::initializer_list<PortSpec> ports)
{
	std::vector<SpatialInterface> interfaces;
	for (const auto &port : ports) {
		interfaces.emplace_back(SpatialInterfaceId(port.first), SpatialObjectId(id), port.second,
			SpatialInterfaceFrame(glm::vec3(0), glm::vec3(1, 0, 0), glm::vec3(0, 1, 0)),
			SpatialInterfaceRegion::point(), InterfaceCompatibilityProfile{},
			SpatialClearanceRequirement(0, 0, 0));
	}
	const glm::mat4 frame(1);
	return SpatialBuildingObject(
		SpatialObjectIdentity(SpatialObjectId(id), id, SpatialObjectClass(object_class),
			SpatialTaxonomyPath(std::move(taxonomy)), 0, 0, {}),
		SpatialFrameState(frame, frame, frame, SpatialFrameReference::root(), {}), {},
		std::move(interfaces), CollisionParticipationPolicy(CollisionLayer::Temporary, {}),
		SpatialObjectState::Unplaced);
}

SpatialConnection link(const char *id, const char *source, const char *from,
                       const char *target, const char *to, Connection type = Connection::Powers)
{
	return SpatialConnection(
		SpatialConnectionId(id), SpatialInterfaceReference(SpatialObjectId(source), SpatialInterfaceId(from)),
		SpatialInterfaceReference(SpatialObjectId(target), SpatialInterfaceId(to)),
		type, SpatialClearanceRequirement(0, 0, 0), 0, {});
}

epg::Result project(std::vector<SpatialBuildingObject> objects, std::vector<SpatialConnection> connections)
{
	return epg::check_spatial_model(SpatialBuildingModel(
		SpatialObjectRegistry(std::move(objects)), {}, SpatialConnectionGraph(std::move(connections)),
		{}, {}, {}));
}

void expect_unfed(const epg::Result &result, const char *id)
{
	assert(result.warnings.size() == 1);
	assert(result.warnings.front().code == "EPG001" && result.warnings.front().object == id);
}

void check_utility_switch(bool open)
{
	const auto result = project({
		device("AC", "PowerSource", {"Power", "Source"}, {{"Out", Interface::ElectricalPort}}),
		device("Switch", "ElectricalSwitch",
		       open ? std::vector<std::string>{"Power", "Switch", "Open"}
		            : std::vector<std::string>{"Power", "Transfer", "Switch", "Closed"},
		       {{"In", Interface::ElectricalPort}, {"Out", Interface::ElectricalPort}}),
		device("Transformer", "VoltageTransformer", {"Power", "Transfer"},
		       {{"In", Interface::ElectricalPort}, {"Out", Interface::ElectricalPort}}),
		device("Motor", "ElectricMotor", {}, {{"In", Interface::ElectricalPort}, {"PowerIn", Interface::ElectricalPort}}),
	}, {
		link("ACToSwitch", "AC", "Out", "Switch", "In"),
		link("SwitchToTransformer", "Switch", "Out", "Transformer", "In"),
		link("TransformerToMotor", "Transformer", "Out", "Motor", "In"),
	});
	if (open) {
		expect_unfed(result, "Motor");
	} else {
		assert(result.warnings.empty() && result.quality.at({"Motor", "In"}) == 2);
	}
}

void check_u11_distribution()
{
	const auto result = project({
		device("Battery", "PowerSource", {"Power", "Source"}, {{"Output", Interface::ElectricalPort}}),
		device("Bus", "PowerDistribution", {"Power", "Transfer", "DistributionBus"},
		       {{"BatteryIn", Interface::ElectricalPort}, {"BackupIn", Interface::ElectricalPort},
		        {"DriveOut", Interface::ElectricalPort}, {"AvionicsOut", Interface::ElectricalPort}}),
		device("Inverter", "MotorInverter", {"Power", "Transfer", "MotorInverter"},
		       {{"DcIn", Interface::ElectricalPort}, {"PhaseOut", Interface::ElectricalPort}}),
		device("Stator", "PermanentMagnetMotorStator", {}, {{"PowerIn", Interface::ElectricalPort}}),
		device("Avionics", "AvionicsController", {}, {{"PowerIn", Interface::ElectricalPort}}),
	}, {
		link("BatteryToBus", "Battery", "Output", "Bus", "BatteryIn"),
		link("BusToDrive", "Bus", "DriveOut", "Inverter", "DcIn"),
		link("DriveToStator", "Inverter", "PhaseOut", "Stator", "PowerIn"),
		link("BusToAvionics", "Bus", "AvionicsOut", "Avionics", "PowerIn"),
	});
	assert(result.warnings.empty() && result.quality.at({"Stator", "PowerIn"}) == 2);
}

void check_unavailable_supply()
{
	for (const char *source_class : {"PowerSource", "PowerSourceCatalogEntry"}) {
		expect_unfed(project({
			device("Supply", source_class, {"Power", "Source", "Inactive"}, {{"Out", Interface::ElectricalPort}}),
			device("Motor", "PermanentMagnetMotorStator", {}, {{"PowerIn", Interface::ElectricalPort}}),
		}, {link("Feed", "Supply", "Out", "Motor", "PowerIn")}), "Motor");
	}
	const auto unrelated = project({
		device("Supply", "PowerSource", {"Power", "Source"}, {{"Out", Interface::ElectricalPort}}),
		device("Motor", "DriveModule", {"Power", "ElectricMotor"}, {{"Input", Interface::ElectricalPort}}),
	}, {link("PhysicalJoin", "Supply", "Out", "Motor", "Input", Connection::ConnectedTo)});
	expect_unfed(unrelated, "Motor");
}

void check_native_network()
{
	const auto result = project({
		device("Tank", "PressurisedGasCylinder", {}, {{"GasOut", Interface::PipePort}}),
		device("AirMotor", "PneumaticMotor", {}, {{"GasIn", Interface::PipePort}}),
		device("Battery", "BatteryPack", {}, {{"Output", Interface::ElectricalPort}}),
		device("Motor", "DCMotor", {}, {{"PowerIn", Interface::ElectricalPort}}),
	}, {
		link("AirFeed", "Tank", "GasOut", "AirMotor", "GasIn", Connection::ConnectedTo),
		link("ElectricFeed", "Battery", "Output", "Motor", "PowerIn", Connection::ConnectedTo),
	});
	assert(result.warnings.empty());
	assert(result.quality.at({"AirMotor", "GasIn"}) == 2 && result.quality.at({"Motor", "PowerIn"}) == 2);
}

void check_cycle_without_source()
{
	expect_unfed(project({
		device("A", "PowerDistribution", {"Power", "Transfer"},
		       {{"FromB", Interface::ElectricalPort}, {"ToB", Interface::ElectricalPort}}),
		device("B", "PowerDistribution", {"Power", "Transfer"},
		       {{"FromA", Interface::ElectricalPort}, {"ToA", Interface::ElectricalPort},
		        {"ToMotor", Interface::ElectricalPort}}),
		device("Motor", "ElectricMotor", {}, {{"PowerIn", Interface::ElectricalPort}}),
	}, {
		link("AtoB", "A", "ToB", "B", "FromA"),
		link("BtoA", "B", "ToA", "A", "FromB"),
		link("BtoMotor", "B", "ToMotor", "Motor", "PowerIn"),
	}), "Motor");
}

void check_legacy_port_direction_and_required_inputs()
{
	const auto backwards = project({
		device("Battery", "BatteryPack", {}, {{"Output", Interface::ElectricalPort}}),
		device("Motor", "ElectricMotor", {}, {{"PowerIn", Interface::ElectricalPort}}),
	}, {link("Backwards", "Motor", "PowerIn", "Battery", "Output", Connection::ConnectedTo)});
	assert(backwards.warnings.size() == 2);
	assert(backwards.warnings[0].code == "EPG003" && backwards.warnings[0].object == "Backwards");
	assert(backwards.warnings[1].code == "EPG001" && backwards.warnings[1].object == "Motor");

	expect_unfed(project({
		device("FuelTank", "FuelTank", {}, {{"FuelOut", Interface::PipePort}}),
		device("FuelCell", "FuelCell", {"Power", "Transfer"},
		       {{"FuelIn", Interface::PipePort}, {"OxidantIn", Interface::PipePort},
		        {"PowerOut", Interface::ElectricalPort}}),
		device("Motor", "ElectricMotor", {}, {{"PowerIn", Interface::ElectricalPort}}),
	}, {
		link("FuelFeed", "FuelTank", "FuelOut", "FuelCell", "FuelIn", Connection::ConnectedTo),
		link("MotorFeed", "FuelCell", "PowerOut", "Motor", "PowerIn"),
	}), "Motor");
}

void check_switch_control_does_not_supply_power()
{
	const auto result = project({
		device("Button", "SwitchInput", {}, {{"SignalOut", Interface::ControlPort}}),
		device("Switch", "ClosedSwitch", {"Power", "Transfer"},
		       {{"In", Interface::ElectricalPort}, {"Out", Interface::ElectricalPort},
		        {"ControlIn", Interface::ControlPort}}),
		device("Motor", "ElectricMotor", {}, {{"PowerIn", Interface::ElectricalPort}}),
	}, {
		link("Control", "Button", "SignalOut", "Switch", "ControlIn", Connection::ConnectedTo),
		link("Power", "Switch", "Out", "Motor", "PowerIn"),
	});
	assert(result.quality.at({"Switch", "ControlIn"}) == 2);
	expect_unfed(result, "Motor");
}

} // namespace

int main()
{
	check_utility_switch(false);
	check_utility_switch(true);
	check_u11_distribution();
	check_unavailable_supply();
	check_native_network();
	check_cycle_without_source();
	check_legacy_port_direction_and_required_inputs();
	check_switch_control_does_not_supply_power();
	std::cout << "Spatial energy warning checks passed.\n";
}
