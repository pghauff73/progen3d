#pragma once

#include "building/model/BuildingEvidenceReference.h"
#include "building/model/BuildingServiceFlowId.h"
#include "building/model/BuildingServiceMedium.h"
#include "building/model/BuildingServicePortId.h"

#include <utility>

class BuildingServiceFlow {
public:
	BuildingServiceFlow(BuildingServiceFlowId flow_id,
	                    BuildingServicePortId source_port_id,
	                    BuildingServicePortId target_port_id,
	                    BuildingServiceMedium medium,
	                    BuildingEvidenceReference evidence)
		: flow_id_(std::move(flow_id)),
		  source_port_id_(std::move(source_port_id)),
		  target_port_id_(std::move(target_port_id)),
		  medium_(medium),
		  evidence_(std::move(evidence)) {}

	const BuildingServiceFlowId &flowId() const { return flow_id_; }
	const BuildingServicePortId &sourcePortId() const { return source_port_id_; }
	const BuildingServicePortId &targetPortId() const { return target_port_id_; }
	BuildingServiceMedium medium() const { return medium_; }
	const BuildingEvidenceReference &evidence() const { return evidence_; }

private:
	BuildingServiceFlowId flow_id_;
	BuildingServicePortId source_port_id_;
	BuildingServicePortId target_port_id_;
	BuildingServiceMedium medium_ = BuildingServiceMedium::ControlSignal;
	BuildingEvidenceReference evidence_;
};
