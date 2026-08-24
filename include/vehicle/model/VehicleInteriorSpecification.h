#pragma once

#include <glm/glm.hpp>

#include <string>
#include <utility>
#include <vector>

class VehicleOccupantEnvelope
{
public:
	VehicleOccupantEnvelope(
		std::string identifier,
		glm::vec3 hip_point,
		glm::vec3 eye_point,
		float head_clearance,
		glm::vec3 leg_envelope,
		float shoulder_width)
		: identifier_(std::move(identifier)),
		  hip_point_(hip_point),
		  eye_point_(eye_point),
		  head_clearance_(head_clearance),
		  leg_envelope_(leg_envelope),
		  shoulder_width_(shoulder_width)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const glm::vec3 &hipPoint() const { return hip_point_; }
	const glm::vec3 &eyePoint() const { return eye_point_; }
	float headClearance() const { return head_clearance_; }
	const glm::vec3 &legEnvelope() const { return leg_envelope_; }
	float shoulderWidth() const { return shoulder_width_; }

private:
	std::string identifier_;
	glm::vec3 hip_point_{0.0f};
	glm::vec3 eye_point_{0.0f};
	float head_clearance_ = 0.0f;
	glm::vec3 leg_envelope_{0.0f};
	float shoulder_width_ = 0.0f;
};

class VehicleInteriorSpecification
{
public:
	VehicleInteriorSpecification(
		std::vector<VehicleOccupantEnvelope> occupant_envelopes,
		glm::vec3 dashboard_origin,
		glm::vec3 steering_wheel_origin,
		glm::vec3 centre_console_origin)
		: occupant_envelopes_(std::move(occupant_envelopes)),
		  dashboard_origin_(dashboard_origin),
		  steering_wheel_origin_(steering_wheel_origin),
		  centre_console_origin_(centre_console_origin)
	{
	}

	const std::vector<VehicleOccupantEnvelope> &occupantEnvelopes() const
	{
		return occupant_envelopes_;
	}
	const glm::vec3 &dashboardOrigin() const { return dashboard_origin_; }
	const glm::vec3 &steeringWheelOrigin() const
	{
		return steering_wheel_origin_;
	}
	const glm::vec3 &centreConsoleOrigin() const
	{
		return centre_console_origin_;
	}

private:
	std::vector<VehicleOccupantEnvelope> occupant_envelopes_;
	glm::vec3 dashboard_origin_{0.0f};
	glm::vec3 steering_wheel_origin_{0.0f};
	glm::vec3 centre_console_origin_{0.0f};
};
