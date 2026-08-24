#pragma once

#include "vehicle/mcsmv2/model/VehicleSectionInterpolationRelationship.h"

#include <glm/glm.hpp>

#include <string>
#include <utility>
#include <vector>

enum class VehicleCharacterCurveRole
{
	CentreSpine,
	RoofCentre,
	RoofRailLeft,
	RoofRailRight,
	GlassShoulderLeft,
	GlassShoulderRight,
	BeltLeft,
	BeltRight,
	ShoulderLeft,
	ShoulderRight,
	RockerLeft,
	RockerRight,
	UnderbodyEdgeLeft,
	UnderbodyEdgeRight
};

const char *vehicleCharacterCurveRoleName(VehicleCharacterCurveRole role);

class VehicleCharacterCurve
{
public:
	VehicleCharacterCurve(
		std::string identifier,
		VehicleCharacterCurveRole role,
		std::vector<glm::dvec3> source_points,
		std::vector<VehicleSectionInterpolationRelationship> section_relationships)
		: identifier_(std::move(identifier)),
		  role_(role),
		  source_points_(std::move(source_points)),
		  section_relationships_(std::move(section_relationships))
	{
	}

	const std::string &identifier() const { return identifier_; }
	VehicleCharacterCurveRole role() const { return role_; }
	const std::vector<glm::dvec3> &sourcePoints() const { return source_points_; }
	const std::vector<VehicleSectionInterpolationRelationship> &sectionRelationships() const
	{
		return section_relationships_;
	}

private:
	std::string identifier_;
	VehicleCharacterCurveRole role_ = VehicleCharacterCurveRole::CentreSpine;
	std::vector<glm::dvec3> source_points_;
	std::vector<VehicleSectionInterpolationRelationship> section_relationships_;
};
