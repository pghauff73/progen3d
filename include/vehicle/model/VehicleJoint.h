#pragma once

#include <glm/glm.hpp>

#include <string>
#include <utility>

enum class VehicleJointType
{
	Fixed,
	Revolute,
	Prismatic,
	Ball
};

class VehicleJoint
{
public:
	VehicleJoint(
		std::string joint_identifier,
		VehicleJointType type,
		std::string source_object_identifier,
		std::string source_interface_identifier,
		std::string target_object_identifier,
		std::string target_interface_identifier,
		glm::vec3 axis,
		float minimum_state,
		float maximum_state,
		float current_state)
		: joint_identifier_(std::move(joint_identifier)),
		  type_(type),
		  source_object_identifier_(std::move(source_object_identifier)),
		  source_interface_identifier_(std::move(source_interface_identifier)),
		  target_object_identifier_(std::move(target_object_identifier)),
		  target_interface_identifier_(std::move(target_interface_identifier)),
		  axis_(axis),
		  minimum_state_(minimum_state),
		  maximum_state_(maximum_state),
		  current_state_(current_state)
	{
	}

	const std::string &jointIdentifier() const { return joint_identifier_; }
	VehicleJointType type() const { return type_; }
	const std::string &sourceObjectIdentifier() const
	{
		return source_object_identifier_;
	}
	const std::string &sourceInterfaceIdentifier() const
	{
		return source_interface_identifier_;
	}
	const std::string &targetObjectIdentifier() const
	{
		return target_object_identifier_;
	}
	const std::string &targetInterfaceIdentifier() const
	{
		return target_interface_identifier_;
	}
	const glm::vec3 &axis() const { return axis_; }
	float minimumState() const { return minimum_state_; }
	float maximumState() const { return maximum_state_; }
	float currentState() const { return current_state_; }

	VehicleJoint withCurrentState(float current_state) const
	{
		return VehicleJoint(
			joint_identifier_, type_, source_object_identifier_,
			source_interface_identifier_, target_object_identifier_,
			target_interface_identifier_, axis_, minimum_state_, maximum_state_,
			current_state);
	}

private:
	std::string joint_identifier_;
	VehicleJointType type_ = VehicleJointType::Fixed;
	std::string source_object_identifier_;
	std::string source_interface_identifier_;
	std::string target_object_identifier_;
	std::string target_interface_identifier_;
	glm::vec3 axis_{0.0f, 1.0f, 0.0f};
	float minimum_state_ = 0.0f;
	float maximum_state_ = 0.0f;
	float current_state_ = 0.0f;
};
