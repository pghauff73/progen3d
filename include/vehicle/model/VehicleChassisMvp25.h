#pragma once

#include "geometry/model/AxisAlignedBounds.h"
#include "vehicle/model/SuspensionCornerSpecification.h"
#include "vehicle/model/VehicleMvp25Evidence.h"

#include <glm/glm.hpp>

#include <string>
#include <utility>
#include <vector>

enum class SuspensionHardpointRole
{
	WheelCentre,
	UpperBodyMount,
	LowerBodyMount,
	UpperBallJoint,
	LowerBallJoint,
	TieRodInner,
	TieRodOuter,
	SpringMount,
	DamperMount,
	TrailingArmMount,
	ToeLinkMount
};

class SuspensionHardpoint
{
public:
	SuspensionHardpoint(
		std::string identifier,
		SuspensionHardpointRole role,
		VehicleCornerLocation corner,
		glm::vec3 position,
		VehicleEvidenceClassification evidence_classification,
		float tolerance)
		: identifier_(std::move(identifier)),
		  role_(role),
		  corner_(corner),
		  position_(position),
		  evidence_classification_(evidence_classification),
		  tolerance_(tolerance)
	{
	}

	const std::string &identifier() const { return identifier_; }
	SuspensionHardpointRole role() const { return role_; }
	VehicleCornerLocation corner() const { return corner_; }
	const glm::vec3 &position() const { return position_; }
	VehicleEvidenceClassification evidenceClassification() const
	{
		return evidence_classification_;
	}
	float tolerance() const { return tolerance_; }

private:
	std::string identifier_;
	SuspensionHardpointRole role_ = SuspensionHardpointRole::WheelCentre;
	VehicleCornerLocation corner_ = VehicleCornerLocation::FrontLeft;
	glm::vec3 position_{0.0f};
	VehicleEvidenceClassification evidence_classification_ =
		VehicleEvidenceClassification::EngineeringInference;
	float tolerance_ = 0.0f;
};

class KinematicLink
{
public:
	KinematicLink(
		std::string identifier,
		std::string first_hardpoint_identifier,
		std::string second_hardpoint_identifier,
		float nominal_length)
		: identifier_(std::move(identifier)),
		  first_hardpoint_identifier_(std::move(first_hardpoint_identifier)),
		  second_hardpoint_identifier_(std::move(second_hardpoint_identifier)),
		  nominal_length_(nominal_length)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &firstHardpointIdentifier() const
	{
		return first_hardpoint_identifier_;
	}
	const std::string &secondHardpointIdentifier() const
	{
		return second_hardpoint_identifier_;
	}
	float nominalLength() const { return nominal_length_; }

private:
	std::string identifier_;
	std::string first_hardpoint_identifier_;
	std::string second_hardpoint_identifier_;
	float nominal_length_ = 0.0f;
};

class SuspensionHardpointModel
{
public:
	SuspensionHardpointModel(
		std::string identifier,
		VehicleCornerLocation corner,
		std::vector<SuspensionHardpoint> hardpoints,
		std::vector<KinematicLink> links,
		float minimum_travel,
		float maximum_travel,
		float minimum_steering_degrees,
		float maximum_steering_degrees)
		: identifier_(std::move(identifier)),
		  corner_(corner),
		  hardpoints_(std::move(hardpoints)),
		  links_(std::move(links)),
		  minimum_travel_(minimum_travel),
		  maximum_travel_(maximum_travel),
		  minimum_steering_degrees_(minimum_steering_degrees),
		  maximum_steering_degrees_(maximum_steering_degrees)
	{
	}

	const std::string &identifier() const { return identifier_; }
	VehicleCornerLocation corner() const { return corner_; }
	const std::vector<SuspensionHardpoint> &hardpoints() const { return hardpoints_; }
	const std::vector<KinematicLink> &links() const { return links_; }
	float minimumTravel() const { return minimum_travel_; }
	float maximumTravel() const { return maximum_travel_; }
	float minimumSteeringDegrees() const { return minimum_steering_degrees_; }
	float maximumSteeringDegrees() const { return maximum_steering_degrees_; }
	const SuspensionHardpoint *findHardpoint(SuspensionHardpointRole role) const
	{
		for (const SuspensionHardpoint &hardpoint : hardpoints_) {
			if (hardpoint.role() == role) return &hardpoint;
		}
		return nullptr;
	}

private:
	std::string identifier_;
	VehicleCornerLocation corner_ = VehicleCornerLocation::FrontLeft;
	std::vector<SuspensionHardpoint> hardpoints_;
	std::vector<KinematicLink> links_;
	float minimum_travel_ = 0.0f;
	float maximum_travel_ = 0.0f;
	float minimum_steering_degrees_ = 0.0f;
	float maximum_steering_degrees_ = 0.0f;
};

class WheelPoseState
{
public:
	WheelPoseState(float suspension_travel, float steering_degrees)
		: suspension_travel_(suspension_travel), steering_degrees_(steering_degrees)
	{
	}

	float suspensionTravel() const { return suspension_travel_; }
	float steeringDegrees() const { return steering_degrees_; }

private:
	float suspension_travel_ = 0.0f;
	float steering_degrees_ = 0.0f;
};

class SolvedWheelPose
{
public:
	SolvedWheelPose(
		WheelPoseState state,
		glm::vec3 centre,
		glm::vec3 axle_direction,
		float camber_degrees,
		float toe_degrees)
		: state_(state),
		  centre_(centre),
		  axle_direction_(axle_direction),
		  camber_degrees_(camber_degrees),
		  toe_degrees_(toe_degrees)
	{
	}

	const WheelPoseState &state() const { return state_; }
	const glm::vec3 &centre() const { return centre_; }
	const glm::vec3 &axleDirection() const { return axle_direction_; }
	float camberDegrees() const { return camber_degrees_; }
	float toeDegrees() const { return toe_degrees_; }

private:
	WheelPoseState state_;
	glm::vec3 centre_{0.0f};
	glm::vec3 axle_direction_{1.0f, 0.0f, 0.0f};
	float camber_degrees_ = 0.0f;
	float toe_degrees_ = 0.0f;
};

class WheelPoseFunction
{
public:
	WheelPoseFunction(
		std::string identifier,
		std::string hardpoint_model_identifier,
		std::vector<SolvedWheelPose> sampled_poses)
		: identifier_(std::move(identifier)),
		  hardpoint_model_identifier_(std::move(hardpoint_model_identifier)),
		  sampled_poses_(std::move(sampled_poses))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &hardpointModelIdentifier() const
	{
		return hardpoint_model_identifier_;
	}
	const std::vector<SolvedWheelPose> &sampledPoses() const { return sampled_poses_; }

private:
	std::string identifier_;
	std::string hardpoint_model_identifier_;
	std::vector<SolvedWheelPose> sampled_poses_;
};

class WheelSweptEnvelope
{
public:
	WheelSweptEnvelope(
		std::string identifier,
		std::vector<AxisAlignedBounds> sampled_tyre_bounds,
		AxisAlignedBounds swept_bounds)
		: identifier_(std::move(identifier)),
		  sampled_tyre_bounds_(std::move(sampled_tyre_bounds)),
		  swept_bounds_(swept_bounds)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::vector<AxisAlignedBounds> &sampledTyreBounds() const
	{
		return sampled_tyre_bounds_;
	}
	const AxisAlignedBounds &sweptBounds() const { return swept_bounds_; }

private:
	std::string identifier_;
	std::vector<AxisAlignedBounds> sampled_tyre_bounds_;
	AxisAlignedBounds swept_bounds_;
};

class TyreGeometry
{
public:
	TyreGeometry(
		std::string identifier,
		float section_width,
		float aspect_ratio,
		float rim_radius,
		float crown_radius,
		float shoulder_radius,
		float sidewall_bulge,
		float tread_depth,
		float unloaded_radius,
		float loaded_radius)
		: identifier_(std::move(identifier)),
		  section_width_(section_width),
		  aspect_ratio_(aspect_ratio),
		  rim_radius_(rim_radius),
		  crown_radius_(crown_radius),
		  shoulder_radius_(shoulder_radius),
		  sidewall_bulge_(sidewall_bulge),
		  tread_depth_(tread_depth),
		  unloaded_radius_(unloaded_radius),
		  loaded_radius_(loaded_radius)
	{
	}

	const std::string &identifier() const { return identifier_; }
	float sectionWidth() const { return section_width_; }
	float aspectRatio() const { return aspect_ratio_; }
	float rimRadius() const { return rim_radius_; }
	float crownRadius() const { return crown_radius_; }
	float shoulderRadius() const { return shoulder_radius_; }
	float sidewallBulge() const { return sidewall_bulge_; }
	float treadDepth() const { return tread_depth_; }
	float unloadedRadius() const { return unloaded_radius_; }
	float loadedRadius() const { return loaded_radius_; }

private:
	std::string identifier_;
	float section_width_ = 0.0f;
	float aspect_ratio_ = 0.0f;
	float rim_radius_ = 0.0f;
	float crown_radius_ = 0.0f;
	float shoulder_radius_ = 0.0f;
	float sidewall_bulge_ = 0.0f;
	float tread_depth_ = 0.0f;
	float unloaded_radius_ = 0.0f;
	float loaded_radius_ = 0.0f;
};

class AutomotiveRimGeometry
{
public:
	AutomotiveRimGeometry(
		std::string identifier,
		VehicleCornerLocation corner,
		float outer_radius,
		float barrel_radius,
		float width,
		std::size_t spoke_count,
		std::string material)
		: identifier_(std::move(identifier)),
		  corner_(corner),
		  outer_radius_(outer_radius),
		  barrel_radius_(barrel_radius),
		  width_(width),
		  spoke_count_(spoke_count),
		  material_(std::move(material))
	{
	}

	const std::string &identifier() const { return identifier_; }
	VehicleCornerLocation corner() const { return corner_; }
	float outerRadius() const { return outer_radius_; }
	float barrelRadius() const { return barrel_radius_; }
	float width() const { return width_; }
	std::size_t spokeCount() const { return spoke_count_; }
	const std::string &material() const { return material_; }

private:
	std::string identifier_;
	VehicleCornerLocation corner_ = VehicleCornerLocation::FrontLeft;
	float outer_radius_ = 0.0f;
	float barrel_radius_ = 0.0f;
	float width_ = 0.0f;
	std::size_t spoke_count_ = 0u;
	std::string material_;
};

class AutomotiveBrakeGeometry
{
public:
	AutomotiveBrakeGeometry(
		std::string identifier,
		VehicleCornerLocation corner,
		float rotor_radius,
		float rotor_thickness,
		AxisAlignedBounds caliper_bounds,
		std::size_t caliper_piston_count,
		std::string rotor_material)
		: identifier_(std::move(identifier)),
		  corner_(corner),
		  rotor_radius_(rotor_radius),
		  rotor_thickness_(rotor_thickness),
		  caliper_bounds_(caliper_bounds),
		  caliper_piston_count_(caliper_piston_count),
		  rotor_material_(std::move(rotor_material))
	{
	}

	const std::string &identifier() const { return identifier_; }
	VehicleCornerLocation corner() const { return corner_; }
	float rotorRadius() const { return rotor_radius_; }
	float rotorThickness() const { return rotor_thickness_; }
	const AxisAlignedBounds &caliperBounds() const { return caliper_bounds_; }
	std::size_t caliperPistonCount() const { return caliper_piston_count_; }
	const std::string &rotorMaterial() const { return rotor_material_; }

private:
	std::string identifier_;
	VehicleCornerLocation corner_ = VehicleCornerLocation::FrontLeft;
	float rotor_radius_ = 0.0f;
	float rotor_thickness_ = 0.0f;
	AxisAlignedBounds caliper_bounds_;
	std::size_t caliper_piston_count_ = 0u;
	std::string rotor_material_;
};

class WheelHouse
{
public:
	WheelHouse(
		std::string identifier,
		VehicleCornerLocation corner,
		AxisAlignedBounds cavity_bounds,
		float minimum_clearance)
		: identifier_(std::move(identifier)),
		  corner_(corner),
		  cavity_bounds_(cavity_bounds),
		  minimum_clearance_(minimum_clearance)
	{
	}

	const std::string &identifier() const { return identifier_; }
	VehicleCornerLocation corner() const { return corner_; }
	const AxisAlignedBounds &cavityBounds() const { return cavity_bounds_; }
	float minimumClearance() const { return minimum_clearance_; }

private:
	std::string identifier_;
	VehicleCornerLocation corner_ = VehicleCornerLocation::FrontLeft;
	AxisAlignedBounds cavity_bounds_;
	float minimum_clearance_ = 0.0f;
};

enum class AeroGeometryRole
{
	UpperBody,
	Wheel,
	WheelHouse,
	Underbody,
	Splitter,
	CoolingOpening,
	Diffuser,
	Spoiler
};

class AeroGeometryComponent
{
public:
	AeroGeometryComponent(
		std::string identifier,
		AeroGeometryRole role,
		AxisAlignedBounds bounds,
		float validation_weight)
		: identifier_(std::move(identifier)),
		  role_(role),
		  bounds_(bounds),
		  validation_weight_(validation_weight)
	{
	}

	const std::string &identifier() const { return identifier_; }
	AeroGeometryRole role() const { return role_; }
	const AxisAlignedBounds &bounds() const { return bounds_; }
	float validationWeight() const { return validation_weight_; }

private:
	std::string identifier_;
	AeroGeometryRole role_ = AeroGeometryRole::UpperBody;
	AxisAlignedBounds bounds_;
	float validation_weight_ = 0.0f;
};

class AeroGeometry
{
public:
	explicit AeroGeometry(std::vector<AeroGeometryComponent> components)
		: components_(std::move(components))
	{
	}

	const std::vector<AeroGeometryComponent> &components() const { return components_; }

private:
	std::vector<AeroGeometryComponent> components_;
};
