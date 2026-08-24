#pragma once

#include <glm/glm.hpp>

#include <string>
#include <utility>

enum class VehicleOccupantEnvelopeRole
{
	Head,
	Torso
};

class VehicleOccupantEnvelope
{
public:
	VehicleOccupantEnvelope(
		std::string identifier,
		VehicleOccupantEnvelopeRole role,
		glm::dvec3 centre,
		glm::dvec3 half_extents)
		: identifier_(std::move(identifier)),
		  role_(role),
		  centre_(centre),
		  half_extents_(half_extents)
	{
	}

	const std::string &identifier() const { return identifier_; }
	VehicleOccupantEnvelopeRole role() const { return role_; }
	const glm::dvec3 &centre() const { return centre_; }
	const glm::dvec3 &halfExtents() const { return half_extents_; }

private:
	std::string identifier_;
	VehicleOccupantEnvelopeRole role_ = VehicleOccupantEnvelopeRole::Torso;
	glm::dvec3 centre_{0.0};
	glm::dvec3 half_extents_{0.0};
};
