#pragma once

#include <glm/glm.hpp>

#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class VehicleDatumType
{
	VehicleCentreline,
	GroundPlane,
	FrontAxlePlane,
	RearAxlePlane,
	FrontWheelCentreLeft,
	FrontWheelCentreRight,
	RearWheelCentreLeft,
	RearWheelCentreRight,
	BeltLine,
	RockerLine,
	RoofDatum,
	HoodDatum,
	DashboardPlane,
	SeatReferencePlane
};

class VehicleDatum
{
public:
	VehicleDatum(
		VehicleDatumType type,
		glm::vec3 origin,
		glm::vec3 normal,
		std::string identifier)
		: type_(type),
		  origin_(origin),
		  normal_(normal),
		  identifier_(std::move(identifier))
	{
	}

	VehicleDatumType type() const { return type_; }
	const glm::vec3 &origin() const { return origin_; }
	const glm::vec3 &normal() const { return normal_; }
	const std::string &identifier() const { return identifier_; }

private:
	VehicleDatumType type_ = VehicleDatumType::GroundPlane;
	glm::vec3 origin_{0.0f};
	glm::vec3 normal_{0.0f, 1.0f, 0.0f};
	std::string identifier_;
};

class VehicleDatumSet
{
public:
	explicit VehicleDatumSet(std::vector<VehicleDatum> datums)
		: datums_(std::move(datums))
	{
	}

	const std::vector<VehicleDatum> &datums() const { return datums_; }
	const VehicleDatum *find(VehicleDatumType type) const;

private:
	std::vector<VehicleDatum> datums_;
};
