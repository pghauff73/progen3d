#include "building/service/BuildingServiceTopologyValidationService.h"

#include "spatial/model/SpatialBuildingModel.h"

#include <algorithm>
#include <map>
#include <queue>
#include <set>

namespace {

bool supports_medium(const BuildingServiceSystem &system, BuildingServiceMedium medium)
{
	return std::find(system.supportedMedia().begin(), system.supportedMedia().end(), medium) !=
	       system.supportedMedia().end();
}

bool service_flow_cycle_exists(
	const BuildingServicePortId &port_id,
	const std::multimap<BuildingServicePortId, BuildingServicePortId> &adjacency,
	std::set<BuildingServicePortId> *visiting,
	std::set<BuildingServicePortId> *visited)
{
	if (visited->find(port_id) != visited->end()) return false;
	if (!visiting->insert(port_id).second) return true;
	const auto range = adjacency.equal_range(port_id);
	for (auto edge = range.first; edge != range.second; ++edge) {
		if (service_flow_cycle_exists(edge->second, adjacency, visiting, visited)) {
			return true;
		}
	}
	visiting->erase(port_id);
	visited->insert(port_id);
	return false;
}

} // namespace

BuildingModelValidationReport BuildingServiceTopologyValidationService::validate(
	const BuildingServiceModel &service_model,
	const SpatialBuildingModel &spatial_model,
	const BuildingModelSafetyLimits &limits) const
{
	BuildingModelValidationReport report;
	if (service_model.systems().size() > limits.maximum_service_systems ||
	    service_model.ports().size() > limits.maximum_service_ports ||
	    service_model.flows().size() > limits.maximum_service_flows) {
		report.addIssue(BuildingModelValidationIssue(
			BuildingModelValidationCode::SafetyCeilingExceeded,
			"Building service model exceeds a configured safety ceiling."));
	}
	std::map<BuildingServiceSystemId, const BuildingServiceSystem *> systems;
	for (const BuildingServiceSystem &system : service_model.systems()) {
		if (!systems.emplace(system.systemId(), &system).second) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::DuplicateServiceSystemId,
				"Duplicate building service system ID '" + system.systemId().value() + "'."));
		}
		if (spatial_model.objects().find(system.ownerObjectId()) == nullptr) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::UndefinedServiceSystemOwner,
				"Building service system references an undefined owner object.",
				{system.ownerObjectId()}));
		}
		if (system.supportedMedia().empty()) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::UnsupportedServiceMedium,
				"Building service system must declare at least one supported medium.",
				{system.ownerObjectId()}));
		}
	}
	std::map<BuildingServicePortId, const BuildingServicePort *> ports;
	for (const BuildingServicePort &port : service_model.ports()) {
		if (!ports.emplace(port.portId(), &port).second) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::DuplicateServicePortId,
				"Duplicate building service port ID '" + port.portId().value() + "'."));
		}
		const SpatialBuildingObject *owner_object =
			spatial_model.objects().find(port.ownerObjectId());
		if (owner_object == nullptr) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::UndefinedServicePortOwner,
				"Building service port references an undefined owner object.",
				{port.ownerObjectId()}));
		} else if (port.spatialInterface().has_value() &&
		           (port.spatialInterface()->objectId() != port.ownerObjectId() ||
		            owner_object->findInterface(
			            port.spatialInterface()->interfaceId()) == nullptr)) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::UndefinedServicePortOwner,
				"Building service port references an undefined interface on its owner.",
				{port.ownerObjectId()}));
		}
		const auto system = systems.find(port.systemId());
		if (system == systems.end()) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::UndefinedServicePortSystem,
				"Building service port references an undefined service system."));
		} else if (!supports_medium(*system->second, port.medium())) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::UnsupportedServiceMedium,
				"Building service port medium is unsupported by its service system.",
				{port.ownerObjectId()}));
		}
	}
	std::set<BuildingServiceFlowId> flow_ids;
	std::set<BuildingServicePortId> connected_port_ids;
	std::multimap<BuildingServicePortId, BuildingServicePortId> adjacency;
	for (const BuildingServiceFlow &flow : service_model.flows()) {
		if (!flow_ids.insert(flow.flowId()).second) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::DuplicateServiceFlowId,
				"Duplicate building service flow ID '" + flow.flowId().value() + "'."));
		}
		const auto source = ports.find(flow.sourcePortId());
		const auto target = ports.find(flow.targetPortId());
		if (source == ports.end() || target == ports.end()) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::UndefinedServiceFlowEndpoint,
				"Building service flow references an undefined endpoint."));
			continue;
		}
		connected_port_ids.insert(flow.sourcePortId());
		connected_port_ids.insert(flow.targetPortId());
		adjacency.emplace(flow.sourcePortId(), flow.targetPortId());
		if (flow.sourcePortId() == flow.targetPortId() ||
		    source->second->direction() == BuildingServiceFlowDirection::Sink ||
		    target->second->direction() == BuildingServiceFlowDirection::Source) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::InvalidServiceFlowDirection,
				"Building service flow has an invalid source or target direction."));
		}
		if (source->second->medium() != flow.medium() ||
		    target->second->medium() != flow.medium()) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::ServiceMediumMismatch,
				"Building service flow medium does not match both endpoint ports."));
		}
	}
	std::set<BuildingServicePortId> visiting;
	std::set<BuildingServicePortId> visited;
	for (const auto &[port_id, port] : ports) {
		(void)port;
		if (service_flow_cycle_exists(port_id, adjacency, &visiting, &visited)) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::ServiceFlowCycle,
				"Building service flow graph contains a prohibited cycle."));
			break;
		}
	}
	for (const BuildingServicePort &port : service_model.ports()) {
		if (port.applicability() == BuildingModelApplicability::Applicable &&
		    connected_port_ids.find(port.portId()) == connected_port_ids.end()) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::OrphanRequiredServicePort,
				"Applicable building service port is not connected to a declared flow.",
				{port.ownerObjectId()}));
		}
	}
	return report;
}

BuildingServiceContinuityResult
BuildingServiceTopologyValidationService::evaluateContinuity(
	const BuildingServiceModel &service_model,
	const BuildingServicePortId &source_port_id,
	const BuildingServicePortId &target_port_id) const
{
	std::multimap<BuildingServicePortId, BuildingServicePortId> adjacency;
	for (const BuildingServiceFlow &flow : service_model.flows()) {
		adjacency.emplace(flow.sourcePortId(), flow.targetPortId());
	}
	std::queue<BuildingServicePortId> pending;
	std::set<BuildingServicePortId> visited;
	pending.push(source_port_id);
	visited.insert(source_port_id);
	std::size_t traversed = 0;
	while (!pending.empty()) {
		const BuildingServicePortId current = pending.front();
		pending.pop();
		if (current == target_port_id) {
			return BuildingServiceContinuityResult(
				source_port_id, target_port_id, true, traversed);
		}
		const auto range = adjacency.equal_range(current);
		for (auto edge = range.first; edge != range.second; ++edge) {
			++traversed;
			if (visited.insert(edge->second).second) pending.push(edge->second);
		}
	}
	return BuildingServiceContinuityResult(
		source_port_id, target_port_id, false, traversed);
}
