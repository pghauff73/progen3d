#pragma once

#include <glm/glm.hpp>

#include <string>
#include <utility>

class McsMv22SuspensionHardpointDefinition
{
public:
	McsMv22SuspensionHardpointDefinition(
		std::string identifier,
		std::string axle,
		std::string side,
		std::string role,
		glm::dvec3 source_position,
		std::string evidence_status,
		double confidence)
		: identifier_(std::move(identifier)),
		  axle_(std::move(axle)),
		  side_(std::move(side)),
		  role_(std::move(role)),
		  source_position_(source_position),
		  evidence_status_(std::move(evidence_status)),
		  confidence_(confidence)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &axle() const { return axle_; }
	const std::string &side() const { return side_; }
	const std::string &role() const { return role_; }
	const glm::dvec3 &sourcePosition() const { return source_position_; }
	const std::string &evidenceStatus() const { return evidence_status_; }
	double confidence() const { return confidence_; }

private:
	std::string identifier_;
	std::string axle_;
	std::string side_;
	std::string role_;
	glm::dvec3 source_position_{0.0};
	std::string evidence_status_;
	double confidence_ = 0.0;
};
