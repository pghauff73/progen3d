#pragma once

#include "geometry/model/AxisAlignedBounds.h"
#include "geometry/model/Curve3D.h"
#include "vehicle/model/VehicleFittingEvidence.h"

#include <glm/glm.hpp>
#include <glm/geometric.hpp>

#include <string>
#include <utility>
#include <variant>
#include <vector>

enum class VehicleClosureType
{
	SideDoor,
	Bonnet,
	RearHatch,
	FuelFlap
};

class Aperture
{
public:
	Aperture(
		std::string identifier,
		std::string host_structure_identifier,
		std::string outer_boundary_identifier,
		std::string inner_boundary_identifier,
		float flange_depth,
		std::string seal_seat_identifier,
		std::vector<std::string> reinforcement_identifiers,
		std::vector<std::string> mounting_interface_identifiers)
		: identifier_(std::move(identifier)),
		  host_structure_identifier_(std::move(host_structure_identifier)),
		  outer_boundary_identifier_(std::move(outer_boundary_identifier)),
		  inner_boundary_identifier_(std::move(inner_boundary_identifier)),
		  flange_depth_(flange_depth),
		  seal_seat_identifier_(std::move(seal_seat_identifier)),
		  reinforcement_identifiers_(std::move(reinforcement_identifiers)),
		  mounting_interface_identifiers_(std::move(mounting_interface_identifiers))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &hostStructureIdentifier() const
	{
		return host_structure_identifier_;
	}
	const std::string &outerBoundaryIdentifier() const
	{
		return outer_boundary_identifier_;
	}
	const std::string &innerBoundaryIdentifier() const
	{
		return inner_boundary_identifier_;
	}
	float flangeDepth() const { return flange_depth_; }
	const std::string &sealSeatIdentifier() const { return seal_seat_identifier_; }
	const std::vector<std::string> &reinforcementIdentifiers() const
	{
		return reinforcement_identifiers_;
	}
	const std::vector<std::string> &mountingInterfaceIdentifiers() const
	{
		return mounting_interface_identifiers_;
	}

private:
	std::string identifier_;
	std::string host_structure_identifier_;
	std::string outer_boundary_identifier_;
	std::string inner_boundary_identifier_;
	float flange_depth_ = 0.0f;
	std::string seal_seat_identifier_;
	std::vector<std::string> reinforcement_identifiers_;
	std::vector<std::string> mounting_interface_identifiers_;
};

class HingePair
{
public:
	HingePair(
		std::string identifier,
		glm::vec3 lower_hinge_point,
		glm::vec3 upper_hinge_point,
		float maximum_angle_degrees,
		VehicleEvidenceClassification evidence_classification)
		: identifier_(std::move(identifier)),
		  lower_hinge_point_(lower_hinge_point),
		  upper_hinge_point_(upper_hinge_point),
		  maximum_angle_degrees_(maximum_angle_degrees),
		  evidence_classification_(evidence_classification)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const glm::vec3 &lowerHingePoint() const { return lower_hinge_point_; }
	const glm::vec3 &upperHingePoint() const { return upper_hinge_point_; }
	float maximumAngleDegrees() const { return maximum_angle_degrees_; }
	VehicleEvidenceClassification evidenceClassification() const
	{
		return evidence_classification_;
	}
	glm::vec3 axis() const
	{
		const glm::vec3 direction = upper_hinge_point_ - lower_hinge_point_;
		const float length = glm::length(direction);
		return length > 1.0e-6f ? direction / length : glm::vec3(0.0f);
	}

private:
	std::string identifier_;
	glm::vec3 lower_hinge_point_{0.0f};
	glm::vec3 upper_hinge_point_{0.0f, 1.0f, 0.0f};
	float maximum_angle_degrees_ = 0.0f;
	VehicleEvidenceClassification evidence_classification_ =
		VehicleEvidenceClassification::EngineeringInference;
};

class FourBarJoint
{
public:
	FourBarJoint(
		std::string identifier,
		glm::vec3 body_mount_a,
		glm::vec3 body_mount_b,
		glm::vec3 closure_mount_a,
		glm::vec3 closure_mount_b,
		float maximum_angle_degrees,
		glm::vec3 open_translation,
		VehicleEvidenceClassification evidence_classification)
		: identifier_(std::move(identifier)),
		  body_mount_a_(body_mount_a),
		  body_mount_b_(body_mount_b),
		  closure_mount_a_(closure_mount_a),
		  closure_mount_b_(closure_mount_b),
		  maximum_angle_degrees_(maximum_angle_degrees),
		  open_translation_(open_translation),
		  evidence_classification_(evidence_classification)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const glm::vec3 &bodyMountA() const { return body_mount_a_; }
	const glm::vec3 &bodyMountB() const { return body_mount_b_; }
	const glm::vec3 &closureMountA() const { return closure_mount_a_; }
	const glm::vec3 &closureMountB() const { return closure_mount_b_; }
	float maximumAngleDegrees() const { return maximum_angle_degrees_; }
	const glm::vec3 &openTranslation() const { return open_translation_; }
	VehicleEvidenceClassification evidenceClassification() const
	{
		return evidence_classification_;
	}

private:
	std::string identifier_;
	glm::vec3 body_mount_a_{0.0f};
	glm::vec3 body_mount_b_{1.0f, 0.0f, 0.0f};
	glm::vec3 closure_mount_a_{0.0f};
	glm::vec3 closure_mount_b_{1.0f, 0.0f, 0.0f};
	float maximum_angle_degrees_ = 0.0f;
	glm::vec3 open_translation_{0.0f};
	VehicleEvidenceClassification evidence_classification_ =
		VehicleEvidenceClassification::EngineeringInference;
};

using ClosureKinematicRelationship = std::variant<HingePair, FourBarJoint>;

class GuideRailJoint
{
public:
	GuideRailJoint(
		std::string identifier,
		Curve3D front_rail,
		Curve3D rear_rail,
		glm::vec3 front_follower,
		glm::vec3 rear_follower,
		float lower_limit,
		float upper_limit,
		VehicleEvidenceClassification evidence_classification)
		: identifier_(std::move(identifier)),
		  front_rail_(std::move(front_rail)),
		  rear_rail_(std::move(rear_rail)),
		  front_follower_(front_follower),
		  rear_follower_(rear_follower),
		  lower_limit_(lower_limit),
		  upper_limit_(upper_limit),
		  evidence_classification_(evidence_classification)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const Curve3D &frontRail() const { return front_rail_; }
	const Curve3D &rearRail() const { return rear_rail_; }
	const glm::vec3 &frontFollower() const { return front_follower_; }
	const glm::vec3 &rearFollower() const { return rear_follower_; }
	float lowerLimit() const { return lower_limit_; }
	float upperLimit() const { return upper_limit_; }
	VehicleEvidenceClassification evidenceClassification() const
	{
		return evidence_classification_;
	}

private:
	std::string identifier_;
	Curve3D front_rail_{Curve3DType::Line, {glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)}};
	Curve3D rear_rail_{Curve3DType::Line, {glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f)}};
	glm::vec3 front_follower_{0.0f};
	glm::vec3 rear_follower_{1.0f, 0.0f, 0.0f};
	float lower_limit_ = 0.0f;
	float upper_limit_ = 1.0f;
	VehicleEvidenceClassification evidence_classification_ =
		VehicleEvidenceClassification::EngineeringInference;
};

class BeltSeal
{
public:
	BeltSeal(
		std::string identifier,
		std::string path_identifier,
		float profile_width,
		float profile_height)
		: identifier_(std::move(identifier)),
		  path_identifier_(std::move(path_identifier)),
		  profile_width_(profile_width),
		  profile_height_(profile_height)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &pathIdentifier() const { return path_identifier_; }
	float profileWidth() const { return profile_width_; }
	float profileHeight() const { return profile_height_; }

private:
	std::string identifier_;
	std::string path_identifier_;
	float profile_width_ = 0.0f;
	float profile_height_ = 0.0f;
};

class TelescopingLink
{
public:
	TelescopingLink(
		std::string identifier,
		glm::vec3 body_mount,
		glm::vec3 closure_mount,
		float minimum_length,
		float maximum_length,
		VehicleEvidenceClassification evidence_classification)
		: identifier_(std::move(identifier)),
		  body_mount_(body_mount),
		  closure_mount_(closure_mount),
		  minimum_length_(minimum_length),
		  maximum_length_(maximum_length),
		  evidence_classification_(evidence_classification)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const glm::vec3 &bodyMount() const { return body_mount_; }
	const glm::vec3 &closureMount() const { return closure_mount_; }
	float minimumLength() const { return minimum_length_; }
	float maximumLength() const { return maximum_length_; }
	VehicleEvidenceClassification evidenceClassification() const
	{
		return evidence_classification_;
	}

private:
	std::string identifier_;
	glm::vec3 body_mount_{0.0f};
	glm::vec3 closure_mount_{0.0f};
	float minimum_length_ = 0.0f;
	float maximum_length_ = 0.0f;
	VehicleEvidenceClassification evidence_classification_ =
		VehicleEvidenceClassification::EngineeringInference;
};

class SweptVolume
{
public:
	SweptVolume(
		AxisAlignedBounds source_bounds,
		AxisAlignedBounds swept_bounds,
		float minimum_state,
		float maximum_state,
		std::size_t sample_count)
		: source_bounds_(source_bounds),
		  swept_bounds_(swept_bounds),
		  minimum_state_(minimum_state),
		  maximum_state_(maximum_state),
		  sample_count_(sample_count)
	{
	}

	const AxisAlignedBounds &sourceBounds() const { return source_bounds_; }
	const AxisAlignedBounds &sweptBounds() const { return swept_bounds_; }
	float minimumState() const { return minimum_state_; }
	float maximumState() const { return maximum_state_; }
	std::size_t sampleCount() const { return sample_count_; }

private:
	AxisAlignedBounds source_bounds_;
	AxisAlignedBounds swept_bounds_;
	float minimum_state_ = 0.0f;
	float maximum_state_ = 1.0f;
	std::size_t sample_count_ = 0u;
};

class DropGlassAssembly
{
public:
	DropGlassAssembly(
		std::string identifier,
		std::string parent_closure_identifier,
		std::string glass_panel_identifier,
		GuideRailJoint guide_rail_joint,
		BeltSeal inner_belt_seal,
		BeltSeal outer_belt_seal,
		AxisAlignedBounds door_inner_volume,
		float state)
		: identifier_(std::move(identifier)),
		  parent_closure_identifier_(std::move(parent_closure_identifier)),
		  glass_panel_identifier_(std::move(glass_panel_identifier)),
		  guide_rail_joint_(std::move(guide_rail_joint)),
		  inner_belt_seal_(std::move(inner_belt_seal)),
		  outer_belt_seal_(std::move(outer_belt_seal)),
		  door_inner_volume_(door_inner_volume),
		  state_(state)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &parentClosureIdentifier() const
	{
		return parent_closure_identifier_;
	}
	const std::string &glassPanelIdentifier() const { return glass_panel_identifier_; }
	const GuideRailJoint &guideRailJoint() const { return guide_rail_joint_; }
	const BeltSeal &innerBeltSeal() const { return inner_belt_seal_; }
	const BeltSeal &outerBeltSeal() const { return outer_belt_seal_; }
	const AxisAlignedBounds &doorInnerVolume() const { return door_inner_volume_; }
	float state() const { return state_; }

private:
	std::string identifier_;
	std::string parent_closure_identifier_;
	std::string glass_panel_identifier_;
	GuideRailJoint guide_rail_joint_;
	BeltSeal inner_belt_seal_;
	BeltSeal outer_belt_seal_;
	AxisAlignedBounds door_inner_volume_;
	float state_ = 1.0f;
};

class ClosureAssembly
{
public:
	ClosureAssembly(
		std::string identifier,
		VehicleClosureType closure_type,
		std::string host_object_identifier,
		std::string aperture_identifier,
		std::string outer_panel_identifier,
		std::string inner_panel_identifier,
		ClosureKinematicRelationship kinematic_relationship,
		std::vector<std::string> seal_identifiers,
		std::vector<std::string> latch_identifiers,
		std::vector<std::string> child_identifiers,
		std::vector<TelescopingLink> support_links,
		VehicleViewEvidenceWeights view_evidence_weights,
		float state,
		SweptVolume swept_volume)
		: identifier_(std::move(identifier)),
		  closure_type_(closure_type),
		  host_object_identifier_(std::move(host_object_identifier)),
		  aperture_identifier_(std::move(aperture_identifier)),
		  outer_panel_identifier_(std::move(outer_panel_identifier)),
		  inner_panel_identifier_(std::move(inner_panel_identifier)),
		  kinematic_relationship_(std::move(kinematic_relationship)),
		  seal_identifiers_(std::move(seal_identifiers)),
		  latch_identifiers_(std::move(latch_identifiers)),
		  child_identifiers_(std::move(child_identifiers)),
		  support_links_(std::move(support_links)),
		  view_evidence_weights_(view_evidence_weights),
		  state_(state),
		  swept_volume_(std::move(swept_volume))
	{
	}

	const std::string &identifier() const { return identifier_; }
	VehicleClosureType closureType() const { return closure_type_; }
	const std::string &hostObjectIdentifier() const { return host_object_identifier_; }
	const std::string &apertureIdentifier() const { return aperture_identifier_; }
	const std::string &outerPanelIdentifier() const { return outer_panel_identifier_; }
	const std::string &innerPanelIdentifier() const { return inner_panel_identifier_; }
	const ClosureKinematicRelationship &kinematicRelationship() const
	{
		return kinematic_relationship_;
	}
	const std::vector<std::string> &sealIdentifiers() const { return seal_identifiers_; }
	const std::vector<std::string> &latchIdentifiers() const { return latch_identifiers_; }
	const std::vector<std::string> &childIdentifiers() const { return child_identifiers_; }
	const std::vector<TelescopingLink> &supportLinks() const { return support_links_; }
	const VehicleViewEvidenceWeights &viewEvidenceWeights() const
	{
		return view_evidence_weights_;
	}
	float state() const { return state_; }
	const SweptVolume &sweptVolume() const { return swept_volume_; }

private:
	std::string identifier_;
	VehicleClosureType closure_type_ = VehicleClosureType::SideDoor;
	std::string host_object_identifier_;
	std::string aperture_identifier_;
	std::string outer_panel_identifier_;
	std::string inner_panel_identifier_;
	ClosureKinematicRelationship kinematic_relationship_;
	std::vector<std::string> seal_identifiers_;
	std::vector<std::string> latch_identifiers_;
	std::vector<std::string> child_identifiers_;
	std::vector<TelescopingLink> support_links_;
	VehicleViewEvidenceWeights view_evidence_weights_{0, 0, 0, 0, 0};
	float state_ = 0.0f;
	SweptVolume swept_volume_{{}, {}, 0.0f, 1.0f, 0u};
};

class VehicleClosureState
{
public:
	VehicleClosureState(
		float front_left_door,
		float front_right_door,
		float rear_left_door,
		float rear_right_door,
		float bonnet,
		float hatch,
		float front_left_window,
		float front_right_window,
		float rear_left_window,
		float rear_right_window)
		: front_left_door_(front_left_door),
		  front_right_door_(front_right_door),
		  rear_left_door_(rear_left_door),
		  rear_right_door_(rear_right_door),
		  bonnet_(bonnet),
		  hatch_(hatch),
		  front_left_window_(front_left_window),
		  front_right_window_(front_right_window),
		  rear_left_window_(rear_left_window),
		  rear_right_window_(rear_right_window)
	{
	}

	float frontLeftDoor() const { return front_left_door_; }
	float frontRightDoor() const { return front_right_door_; }
	float rearLeftDoor() const { return rear_left_door_; }
	float rearRightDoor() const { return rear_right_door_; }
	float bonnet() const { return bonnet_; }
	float hatch() const { return hatch_; }
	float frontLeftWindow() const { return front_left_window_; }
	float frontRightWindow() const { return front_right_window_; }
	float rearLeftWindow() const { return rear_left_window_; }
	float rearRightWindow() const { return rear_right_window_; }

	std::vector<float> orderedValues() const
	{
		return {front_left_door_, front_right_door_, rear_left_door_,
			rear_right_door_, bonnet_, hatch_, front_left_window_,
			front_right_window_, rear_left_window_, rear_right_window_};
	}

private:
	float front_left_door_ = 0.0f;
	float front_right_door_ = 0.0f;
	float rear_left_door_ = 0.0f;
	float rear_right_door_ = 0.0f;
	float bonnet_ = 0.0f;
	float hatch_ = 0.0f;
	float front_left_window_ = 1.0f;
	float front_right_window_ = 1.0f;
	float rear_left_window_ = 1.0f;
	float rear_right_window_ = 1.0f;
};
