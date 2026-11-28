#pragma once

#include "EnergyCatalog.h"
#include "spatial/model/SpatialBuildingModel.h"
#include "spatial/model/SpatialInterfaceState.h"
#include "spatial/model/SpatialInterfaceType.h"

#include <string_view>
#include <utility>

namespace epg {

inline bool matches_native_type(const std::string &domain, SpatialInterfaceType type)
{
	if (domain == "electrical") return type == SpatialInterfaceType::ElectricalPort;
	if (domain == "gas" || domain == "hydraulic" || domain == "chemical") {
		return type == SpatialInterfaceType::PipePort;
	}
	if (domain == "shaft") return type == SpatialInterfaceType::Shaft;
	if (domain == "thermal") return type == SpatialInterfaceType::ThermalInterface;
	if (domain == "control") return type == SpatialInterfaceType::ControlPort;
	return (domain == "radiant" || domain == "microwave") &&
	       type == SpatialInterfaceType::ThermalInterface;
}

namespace detail {

inline bool has_power_role(const SpatialBuildingObject &object, std::string_view role)
{
	const auto &taxonomy = object.identity().taxonomyPath().segments();
	return taxonomy.size() >= 2 && taxonomy[0] == "Power" && taxonomy[1] == role;
}

inline bool is_unavailable(const SpatialBuildingObject &object)
{
	const auto &taxonomy = object.identity().taxonomyPath().segments();
	return std::any_of(taxonomy.begin(), taxonomy.end(), [](const std::string &segment) {
		return segment == "Open" || segment == "Disabled" || segment == "Inactive" ||
		       segment == "Unavailable" || segment == "Deenergized";
	});
}

inline bool is_electrical_motor_alias(std::string_view object_class)
{
	return object_class == "PermanentMagnetMotor" || object_class == "PermanentMagnetMotorStator" ||
	       object_class == "BrushedDCMotor" || object_class == "BrushlessDCMotor" ||
	       object_class == "InductionMotor" || object_class == "LinearElectricMotor" ||
	       object_class == "SynchronousMotor";
}

inline bool ends_with(std::string_view name, std::string_view suffix)
{
	return name.size() >= suffix.size() &&
	       name.substr(name.size() - suffix.size()) == suffix;
}

inline Endpoint to_endpoint(const SpatialInterfaceReference &reference)
{
	return {reference.objectId().value(), reference.interfaceId().value()};
}

inline bool usable(const SpatialConnection &connection)
{
	return connection.state() != SpatialConnectionState::Broken &&
	       connection.state() != SpatialConnectionState::Invalid;
}

class SpatialEnergyGraphProjection {
public:
	explicit SpatialEnergyGraphProjection(const SpatialBuildingModel &model) : model_(model) {}

	Result evaluate()
	{
		for (const auto &connection : model_.connectionGraph().connections()) {
			if (usable(connection) && connection.connectionType() == SpatialConnectionType::Powers) {
				power_outputs_.insert(to_endpoint(connection.sourceInterface()));
				power_inputs_.insert(to_endpoint(connection.targetInterface()));
			}
		}
		for (const auto &object : model_.objects().objects()) add_object(object);
		for (const auto &connection : model_.connectionGraph().connections()) add_link(connection);
		auto result = check(catalog_, objects_, links_);
		for (const auto &binding : port_bindings_) {
			const auto quality = result.quality.find(binding.second);
			if (binding.first != binding.second && quality != result.quality.end()) {
				result.quality.emplace(binding.first, quality->second);
			}
		}
		return result;
	}

private:
	enum class PortDirection { Input, Output, Unspecified };

	PortDirection direction(const Endpoint &endpoint) const
	{
		const auto &name = endpoint.second;
		if (ends_with(name, "Input") || ends_with(name, "In")) return PortDirection::Input;
		if (ends_with(name, "Output") || ends_with(name, "Out")) return PortDirection::Output;
		const bool input = power_inputs_.count(endpoint) != 0;
		const bool output = power_outputs_.count(endpoint) != 0;
		if (input != output) return input ? PortDirection::Input : PortDirection::Output;
		return PortDirection::Unspecified;
	}

	void bind_port(Object &node, const std::string &authored_name, const std::string &model_name)
	{
		node.ports.insert(model_name);
		port_bindings_.emplace(Endpoint{node.id, authored_name}, Endpoint{node.id, model_name});
	}

	void add_power_component(Object &node, const SpatialBuildingObject &object,
	                         const char *role, bool source_ports, const Model *native_model = nullptr)
	{
		Model definition{role, true, {}, InputRequirement::AnyDeclared};
		for (const auto &port : object.interfaces()) {
			if (port.state() == SpatialInterfaceState::Invalid) continue;
			const auto &name = port.interfaceId().value();
			if (port.type() != SpatialInterfaceType::ElectricalPort) {
				if (native_model) {
					const auto native_port = native_model->ports.find(name);
					if (native_port != native_model->ports.end() &&
					    matches_native_type(native_port->second.domain, port.type())) {
						definition.ports.insert(*native_port);
						bind_port(node, name, name);
					}
				}
				continue;
			}
			powers_only_ports_.emplace(node.id, name);
			// A PowerSource declares outlets. A named charging inlet keeps its direction.
			const auto kind = source_ports
				? (ends_with(name, "Input") || ends_with(name, "In")
				        ? PortDirection::Input : PortDirection::Output)
				: direction({node.id, name});
			if (kind == PortDirection::Unspecified) continue;
			definition.ports.emplace(name, Port{"electrical", kind == PortDirection::Input, false});
			bind_port(node, name, name);
		}
		node.model_override = std::move(definition);
	}

	void add_native_ports(Object &node, const SpatialBuildingObject &object, const Model &definition)
	{
		const bool electrical_motor = definition.role == "motor" && definition.ports.count("PowerIn");
		for (const auto &port : object.interfaces()) {
			if (port.state() == SpatialInterfaceState::Invalid) continue;
			const auto &name = port.interfaceId().value();
			const auto fixed = definition.ports.find(name);
			if (fixed != definition.ports.end() && matches_native_type(fixed->second.domain, port.type())) {
				bind_port(node, name, name);
			} else if (electrical_motor && port.type() == SpatialInterfaceType::ElectricalPort &&
			           direction({node.id, name}) == PortDirection::Input) {
				bind_port(node, name, "PowerIn");
			}
		}
	}

	void add_object(const SpatialBuildingObject &object)
	{
		const auto &identity = object.identity();
		const auto &object_class = identity.objectClass().value();
		Object node{identity.objectId().value(), object_class, {}, std::nullopt};
		const auto known = catalog_.find(object_class);
		const Model *native_model = known == catalog_.end() ? nullptr : &known->second;
		const bool supports_electrical_transfer = !native_model ||
			((native_model->role == "transfer" || native_model->role == "blocked") &&
			 std::all_of(native_model->ports.begin(), native_model->ports.end(), [](const auto &port) {
				return port.second.domain == "electrical" ||
				       (port.second.domain == "control" && port.second.optional);
			 }));
		if (object_class == "PowerSourceCatalogEntry") {
			add_power_component(node, object, "blocked", true);
		} else if (native_model && native_model->role == "motor") {
			add_native_ports(node, object, *native_model);
		} else if (is_electrical_motor_alias(object_class) || has_power_role(object, "ElectricMotor")) {
			node.model_override = catalog_.at("ElectricMotor");
			add_native_ports(node, object, *node.model_override);
		} else if (object_class == "PowerSource") {
			add_power_component(node, object, is_unavailable(object) ? "blocked" : "source", true);
		} else if (supports_electrical_transfer &&
		           (has_power_role(object, "Transfer") || object_class == "OpenElectricalSwitch" ||
		            (has_power_role(object, "Switch") && is_unavailable(object)))) {
			const bool blocked = is_unavailable(object) || object_class == "OpenSwitch" ||
			                     object_class == "OpenElectricalSwitch" ||
			                     (native_model && native_model->role == "blocked");
			add_power_component(node, object, blocked ? "blocked" : "transfer", false, native_model);
		} else if (native_model) {
			add_native_ports(node, object, *native_model);
		} else {
			return;
		}
		modeled_objects_.insert(node.id);
		objects_.push_back(std::move(node));
	}

	bool is_unmodeled_electrical_load(const SpatialInterfaceReference &reference) const
	{
		if (modeled_objects_.count(reference.objectId().value())) return false;
		const auto *object = model_.objects().find(reference.objectId());
		const auto *port = object ? object->findInterface(reference.interfaceId()) : nullptr;
		return port && port->type() == SpatialInterfaceType::ElectricalPort &&
		       port->state() != SpatialInterfaceState::Invalid;
	}

	void add_link(const SpatialConnection &connection)
	{
		const auto type = connection.connectionType();
		if (!usable(connection) ||
		    (type != SpatialConnectionType::Powers && type != SpatialConnectionType::ConnectedTo)) return;
		const auto a = to_endpoint(connection.sourceInterface());
		const auto b = to_endpoint(connection.targetInterface());
		const auto from = port_bindings_.find(a);
		const auto to = port_bindings_.find(b);
		if (from == port_bindings_.end() && to == port_bindings_.end()) return;
		const bool authored_power = powers_only_ports_.count(a) || powers_only_ports_.count(b);
		if (authored_power && type != SpatialConnectionType::Powers) return;
		// An ordinary consumer with a real electrical inlet is outside the EPG catalog.
		if (authored_power && from != port_bindings_.end() && to == port_bindings_.end() &&
		    is_unmodeled_electrical_load(connection.targetInterface())) return;
		links_.push_back({
			connection.connectionId().value(),
			from == port_bindings_.end() ? a : from->second,
			to == port_bindings_.end() ? b : to->second,
		});
	}

	const SpatialBuildingModel &model_;
	const Catalog catalog_ = default_catalog();
	std::vector<Object> objects_;
	std::vector<Link> links_;
	std::set<Endpoint> power_inputs_, power_outputs_, powers_only_ports_;
	std::map<Endpoint, Endpoint> port_bindings_;
	std::set<std::string> modeled_objects_;
};

} // namespace detail

inline Result check_spatial_model(const SpatialBuildingModel &model)
{
	return detail::SpatialEnergyGraphProjection(model).evaluate();
}

} // namespace epg
