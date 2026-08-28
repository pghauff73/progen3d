#pragma once

#include "spatial/model/SpatialClearanceRequirement.h"
#include "spatial/model/SpatialInterfaceId.h"
#include "spatial/model/SpatialInterfaceType.h"

#include <string>
#include <utility>

enum class BuildingConnectionPointOriginKind {
	AuthoredInterface,
	BoundaryFaceCenter,
	BoundaryCenter,
	BoundaryPerimeter,
	BoundaryEdge,
	AggregateCentroid
};

enum class BuildingConnectionPointPurpose {
	Inspection,
	SpatialReference,
	Support,
	Bearing,
	AssemblyMate,
	HostedSeat,
	PerimeterSeal,
	HingeAxis,
	ServiceInlet,
	ServiceOutlet,
	Access,
	EquipmentMount,
	WaterService,
	DrainageService,
	AirService,
	ElectricalService,
	ControlService,
	RootSeat,
	GrowthClearance,
	AggregateConnection,
	PrimaryAttachment
};

class BuildingConnectionPoint {
public:
	BuildingConnectionPoint(
		SpatialInterfaceId connection_point_id,
		BuildingConnectionPointPurpose purpose,
		SpatialInterfaceType interface_type,
		std::string connection_family,
		BuildingConnectionPointOriginKind origin_kind,
		std::string boundary_feature,
		SpatialClearanceRequirement clearance_requirement,
		bool connectable)
		: connection_point_id_(std::move(connection_point_id)),
		  purpose_(purpose),
		  interface_type_(interface_type),
		  connection_family_(std::move(connection_family)),
		  origin_kind_(origin_kind),
		  boundary_feature_(std::move(boundary_feature)),
		  clearance_requirement_(clearance_requirement),
		  connectable_(connectable) {}

	const SpatialInterfaceId &connectionPointId() const { return connection_point_id_; }
	BuildingConnectionPointPurpose purpose() const { return purpose_; }
	SpatialInterfaceType interfaceType() const { return interface_type_; }
	const std::string &connectionFamily() const { return connection_family_; }
	BuildingConnectionPointOriginKind originKind() const { return origin_kind_; }
	const std::string &boundaryFeature() const { return boundary_feature_; }
	const SpatialClearanceRequirement &clearanceRequirement() const
	{
		return clearance_requirement_;
	}
	bool isConnectable() const { return connectable_; }

private:
	SpatialInterfaceId connection_point_id_;
	BuildingConnectionPointPurpose purpose_ = BuildingConnectionPointPurpose::Inspection;
	SpatialInterfaceType interface_type_ = SpatialInterfaceType::InspectionInterface;
	std::string connection_family_;
	BuildingConnectionPointOriginKind origin_kind_ =
		BuildingConnectionPointOriginKind::AuthoredInterface;
	std::string boundary_feature_;
	SpatialClearanceRequirement clearance_requirement_{0.0f, 0.0f, 0.0f};
	bool connectable_ = false;
};
