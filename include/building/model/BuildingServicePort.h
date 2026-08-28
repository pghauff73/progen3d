#pragma once

#include "building/model/BuildingEvidenceReference.h"
#include "building/model/BuildingModelApplicability.h"
#include "building/model/BuildingServiceFlowDirection.h"
#include "building/model/BuildingServiceMedium.h"
#include "building/model/BuildingServicePortId.h"
#include "building/model/BuildingServiceSystemId.h"
#include "spatial/model/SpatialObjectId.h"
#include "spatial/relationship/SpatialInterfaceReference.h"

#include <optional>
#include <utility>

class BuildingServicePort {
public:
	BuildingServicePort(BuildingServicePortId port_id,
	                    SpatialObjectId owner_object_id,
	                    BuildingServiceSystemId system_id,
	                    BuildingServiceMedium medium,
	                    BuildingServiceFlowDirection direction,
	                    std::optional<SpatialInterfaceReference> spatial_interface,
	                    BuildingEvidenceReference evidence,
	                    BuildingModelApplicability applicability =
	                        BuildingModelApplicability::Applicable)
		: port_id_(std::move(port_id)),
		  owner_object_id_(std::move(owner_object_id)),
		  system_id_(std::move(system_id)),
		  medium_(medium),
		  direction_(direction),
		  spatial_interface_(std::move(spatial_interface)),
		  evidence_(std::move(evidence)),
		  applicability_(applicability) {}

	const BuildingServicePortId &portId() const { return port_id_; }
	const SpatialObjectId &ownerObjectId() const { return owner_object_id_; }
	const BuildingServiceSystemId &systemId() const { return system_id_; }
	BuildingServiceMedium medium() const { return medium_; }
	BuildingServiceFlowDirection direction() const { return direction_; }
	const std::optional<SpatialInterfaceReference> &spatialInterface() const
	{
		return spatial_interface_;
	}
	const BuildingEvidenceReference &evidence() const { return evidence_; }
	BuildingModelApplicability applicability() const { return applicability_; }

private:
	BuildingServicePortId port_id_;
	SpatialObjectId owner_object_id_;
	BuildingServiceSystemId system_id_;
	BuildingServiceMedium medium_ = BuildingServiceMedium::ControlSignal;
	BuildingServiceFlowDirection direction_ = BuildingServiceFlowDirection::Bidirectional;
	std::optional<SpatialInterfaceReference> spatial_interface_;
	BuildingEvidenceReference evidence_;
	BuildingModelApplicability applicability_ = BuildingModelApplicability::Applicable;
};
