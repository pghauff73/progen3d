#pragma once

#include "geometry/model/GeometryDetailLevel.h"

#include <glm/glm.hpp>

#include <string>
#include <utility>

enum class VehicleSuspensionType
{
	MacPherson,
	DoubleWishbone,
	MultiLink,
	TrailingArm
};

enum class VehicleCornerLocation
{
	FrontLeft,
	FrontRight,
	RearLeft,
	RearRight
};

class SuspensionCornerSpecification
{
public:
	SuspensionCornerSpecification(
		std::string object_identifier,
		VehicleCornerLocation location,
		VehicleSuspensionType suspension_type,
		glm::vec3 wheel_center,
		glm::vec3 upper_body_mount,
		glm::vec3 lower_body_mount,
		float upright_height,
		float member_radius,
		float compression_travel,
		float rebound_travel,
		GeometryDetailLevel detail_level)
		: object_identifier_(std::move(object_identifier)),
		  location_(location),
		  suspension_type_(suspension_type),
		  wheel_center_(wheel_center),
		  upper_body_mount_(upper_body_mount),
		  lower_body_mount_(lower_body_mount),
		  upright_height_(upright_height),
		  member_radius_(member_radius),
		  compression_travel_(compression_travel),
		  rebound_travel_(rebound_travel),
		  detail_level_(detail_level)
	{
	}

	const std::string &objectIdentifier() const { return object_identifier_; }
	VehicleCornerLocation location() const { return location_; }
	VehicleSuspensionType suspensionType() const { return suspension_type_; }
	const glm::vec3 &wheelCenter() const { return wheel_center_; }
	const glm::vec3 &upperBodyMount() const { return upper_body_mount_; }
	const glm::vec3 &lowerBodyMount() const { return lower_body_mount_; }
	float uprightHeight() const { return upright_height_; }
	float memberRadius() const { return member_radius_; }
	float compressionTravel() const { return compression_travel_; }
	float reboundTravel() const { return rebound_travel_; }
	GeometryDetailLevel detailLevel() const { return detail_level_; }

private:
	std::string object_identifier_;
	VehicleCornerLocation location_ = VehicleCornerLocation::FrontLeft;
	VehicleSuspensionType suspension_type_ = VehicleSuspensionType::MacPherson;
	glm::vec3 wheel_center_{0.0f};
	glm::vec3 upper_body_mount_{0.0f};
	glm::vec3 lower_body_mount_{0.0f};
	float upright_height_ = 0.0f;
	float member_radius_ = 0.0f;
	float compression_travel_ = 0.0f;
	float rebound_travel_ = 0.0f;
	GeometryDetailLevel detail_level_ = GeometryDetailLevel::Component;
};
