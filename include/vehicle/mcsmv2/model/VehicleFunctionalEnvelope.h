#pragma once

#include <glm/glm.hpp>

#include <string>
#include <utility>

class VehicleFunctionalEnvelope
{
public:
	VehicleFunctionalEnvelope(
		std::string identifier,
		std::string purpose,
		glm::dvec3 centre,
		glm::dvec3 dimensions)
		: identifier_(std::move(identifier)),
		  purpose_(std::move(purpose)),
		  centre_(centre),
		  dimensions_(dimensions)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &purpose() const { return purpose_; }
	const glm::dvec3 &centre() const { return centre_; }
	const glm::dvec3 &dimensions() const { return dimensions_; }

private:
	std::string identifier_;
	std::string purpose_;
	glm::dvec3 centre_{0.0};
	glm::dvec3 dimensions_{0.0};
};
