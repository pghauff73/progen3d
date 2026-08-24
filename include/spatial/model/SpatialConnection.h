#pragma once

#include "spatial/model/ConstraintDegreeOfFreedomState.h"
#include "spatial/model/SpatialClearanceRequirement.h"
#include "spatial/model/SpatialConnectionId.h"
#include "spatial/model/SpatialConnectionState.h"
#include "spatial/model/SpatialConnectionType.h"
#include "spatial/relationship/SpatialInterfaceReference.h"

#include <utility>

class SpatialConnection {
public:
	SpatialConnection(SpatialConnectionId connection_id,
	                  SpatialInterfaceReference source_interface,
	                  SpatialInterfaceReference target_interface,
	                  SpatialConnectionType connection_type,
	                  SpatialClearanceRequirement clearance,
	                  float insertion_depth,
	                  ConstraintDegreeOfFreedomState locked_degrees_of_freedom,
	                  SpatialConnectionState state = SpatialConnectionState::Proposed)
		: connection_id_(std::move(connection_id)),
		  source_interface_(std::move(source_interface)),
		  target_interface_(std::move(target_interface)),
		  connection_type_(connection_type),
		  clearance_(clearance),
		  insertion_depth_(insertion_depth),
		  locked_degrees_of_freedom_(locked_degrees_of_freedom),
		  state_(state) {}

	const SpatialConnectionId &connectionId() const { return connection_id_; }
	const SpatialInterfaceReference &sourceInterface() const { return source_interface_; }
	const SpatialInterfaceReference &targetInterface() const { return target_interface_; }
	SpatialConnectionType connectionType() const { return connection_type_; }
	const SpatialClearanceRequirement &clearance() const { return clearance_; }
	float insertionDepth() const { return insertion_depth_; }
	const ConstraintDegreeOfFreedomState &lockedDegreesOfFreedom() const
	{
		return locked_degrees_of_freedom_;
	}
	SpatialConnectionState state() const { return state_; }

private:
	SpatialConnectionId connection_id_;
	SpatialInterfaceReference source_interface_{{}, {}};
	SpatialInterfaceReference target_interface_{{}, {}};
	SpatialConnectionType connection_type_ = SpatialConnectionType::ConnectedTo;
	SpatialClearanceRequirement clearance_{0.0f, 0.0f, 0.0f};
	float insertion_depth_ = 0.0f;
	ConstraintDegreeOfFreedomState locked_degrees_of_freedom_;
	SpatialConnectionState state_ = SpatialConnectionState::Proposed;
};
