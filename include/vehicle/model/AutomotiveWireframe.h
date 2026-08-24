#pragma once

#include "vehicle/model/VehicleMvp25Evidence.h"

#include <string>
#include <utility>
#include <vector>

enum class AutomotiveWireframeEdgeRole
{
	CentreSpine,
	RoofRail,
	BeltRail,
	ShoulderRail,
	RockerRail,
	WheelBase,
	WheelArchCrown,
	GlassPerimeter,
	DoorAperture,
	BumperBound
};

class AutomotiveWireframeEdge
{
public:
	AutomotiveWireframeEdge(
		std::string identifier,
		std::string first_landmark_identifier,
		std::string second_landmark_identifier,
		AutomotiveWireframeEdgeRole role)
		: identifier_(std::move(identifier)),
		  first_landmark_identifier_(std::move(first_landmark_identifier)),
		  second_landmark_identifier_(std::move(second_landmark_identifier)),
		  role_(role)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &firstLandmarkIdentifier() const
	{
		return first_landmark_identifier_;
	}
	const std::string &secondLandmarkIdentifier() const
	{
		return second_landmark_identifier_;
	}
	AutomotiveWireframeEdgeRole role() const { return role_; }

private:
	std::string identifier_;
	std::string first_landmark_identifier_;
	std::string second_landmark_identifier_;
	AutomotiveWireframeEdgeRole role_ = AutomotiveWireframeEdgeRole::CentreSpine;
};

class VehicleShapePrior
{
public:
	VehicleShapePrior(
		std::string vehicle_class,
		std::string topology_identifier,
		float weight,
		VehicleConstraintStrength strength)
		: vehicle_class_(std::move(vehicle_class)),
		  topology_identifier_(std::move(topology_identifier)),
		  weight_(weight),
		  strength_(strength)
	{
	}

	const std::string &vehicleClass() const { return vehicle_class_; }
	const std::string &topologyIdentifier() const { return topology_identifier_; }
	float weight() const { return weight_; }
	VehicleConstraintStrength strength() const { return strength_; }

private:
	std::string vehicle_class_;
	std::string topology_identifier_;
	float weight_ = 0.0f;
	VehicleConstraintStrength strength_ = VehicleConstraintStrength::Soft;
};

class AutomotiveWireframe
{
public:
	AutomotiveWireframe(
		std::string identifier,
		std::vector<VehicleLandmark> landmarks,
		std::vector<AutomotiveWireframeEdge> edges,
		VehicleShapePrior shape_prior)
		: identifier_(std::move(identifier)),
		  landmarks_(std::move(landmarks)),
		  edges_(std::move(edges)),
		  shape_prior_(std::move(shape_prior))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::vector<VehicleLandmark> &landmarks() const { return landmarks_; }
	const std::vector<AutomotiveWireframeEdge> &edges() const { return edges_; }
	const VehicleShapePrior &shapePrior() const { return shape_prior_; }
	const VehicleLandmark *findLandmark(const std::string &identifier) const
	{
		for (const VehicleLandmark &landmark : landmarks_) {
			if (landmark.identifier() == identifier) return &landmark;
		}
		return nullptr;
	}

private:
	std::string identifier_;
	std::vector<VehicleLandmark> landmarks_;
	std::vector<AutomotiveWireframeEdge> edges_;
	VehicleShapePrior shape_prior_;
};
