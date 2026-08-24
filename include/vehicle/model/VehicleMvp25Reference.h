#pragma once

#include "geometry/model/AxisAlignedBounds.h"

#include <glm/glm.hpp>

#include <string>
#include <utility>
#include <vector>

enum class VehicleReferenceConvention
{
	SaeDesignIntent
};

class VehicleFiducialDatum
{
public:
	VehicleFiducialDatum(
		std::string identifier,
		glm::vec3 point,
		glm::vec3 direction)
		: identifier_(std::move(identifier)), point_(point), direction_(direction)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const glm::vec3 &point() const { return point_; }
	const glm::vec3 &direction() const { return direction_; }

private:
	std::string identifier_;
	glm::vec3 point_{0.0f};
	glm::vec3 direction_{0.0f, 1.0f, 0.0f};
};

class VehicleReferenceFrame
{
public:
	VehicleReferenceFrame(
		VehicleReferenceConvention convention,
		glm::vec3 origin,
		glm::vec3 longitudinal_axis,
		glm::vec3 lateral_axis,
		glm::vec3 vertical_axis,
		float longitudinal_zero_plane,
		float lateral_zero_plane,
		float vertical_zero_plane,
		float ground_plane,
		std::vector<VehicleFiducialDatum> fiducial_datums)
		: convention_(convention),
		  origin_(origin),
		  longitudinal_axis_(longitudinal_axis),
		  lateral_axis_(lateral_axis),
		  vertical_axis_(vertical_axis),
		  longitudinal_zero_plane_(longitudinal_zero_plane),
		  lateral_zero_plane_(lateral_zero_plane),
		  vertical_zero_plane_(vertical_zero_plane),
		  ground_plane_(ground_plane),
		  fiducial_datums_(std::move(fiducial_datums))
	{
	}

	VehicleReferenceConvention convention() const { return convention_; }
	const glm::vec3 &origin() const { return origin_; }
	const glm::vec3 &longitudinalAxis() const { return longitudinal_axis_; }
	const glm::vec3 &lateralAxis() const { return lateral_axis_; }
	const glm::vec3 &verticalAxis() const { return vertical_axis_; }
	float longitudinalZeroPlane() const { return longitudinal_zero_plane_; }
	float lateralZeroPlane() const { return lateral_zero_plane_; }
	float verticalZeroPlane() const { return vertical_zero_plane_; }
	float groundPlane() const { return ground_plane_; }
	const std::vector<VehicleFiducialDatum> &fiducialDatums() const
	{
		return fiducial_datums_;
	}

private:
	VehicleReferenceConvention convention_ = VehicleReferenceConvention::SaeDesignIntent;
	glm::vec3 origin_{0.0f};
	glm::vec3 longitudinal_axis_{0.0f, 0.0f, 1.0f};
	glm::vec3 lateral_axis_{1.0f, 0.0f, 0.0f};
	glm::vec3 vertical_axis_{0.0f, 1.0f, 0.0f};
	float longitudinal_zero_plane_ = 0.0f;
	float lateral_zero_plane_ = 0.0f;
	float vertical_zero_plane_ = 0.0f;
	float ground_plane_ = 0.0f;
	std::vector<VehicleFiducialDatum> fiducial_datums_;
};

enum class VehicleDriveEvidence
{
	FrontWheelDrive,
	RearWheelDrive,
	AllWheelDrive
};

class VehiclePackageEvidence
{
public:
	VehiclePackageEvidence(
		float overall_length,
		float overall_width,
		float overall_height,
		float wheelbase,
		float front_track,
		float rear_track,
		float front_overhang,
		float rear_overhang,
		glm::vec3 front_axle_centre,
		glm::vec3 rear_axle_centre,
		VehicleDriveEvidence drive,
		float tolerance)
		: overall_length_(overall_length),
		  overall_width_(overall_width),
		  overall_height_(overall_height),
		  wheelbase_(wheelbase),
		  front_track_(front_track),
		  rear_track_(rear_track),
		  front_overhang_(front_overhang),
		  rear_overhang_(rear_overhang),
		  front_axle_centre_(front_axle_centre),
		  rear_axle_centre_(rear_axle_centre),
		  drive_(drive),
		  tolerance_(tolerance)
	{
	}

	float overallLength() const { return overall_length_; }
	float overallWidth() const { return overall_width_; }
	float overallHeight() const { return overall_height_; }
	float wheelbase() const { return wheelbase_; }
	float frontTrack() const { return front_track_; }
	float rearTrack() const { return rear_track_; }
	float frontOverhang() const { return front_overhang_; }
	float rearOverhang() const { return rear_overhang_; }
	const glm::vec3 &frontAxleCentre() const { return front_axle_centre_; }
	const glm::vec3 &rearAxleCentre() const { return rear_axle_centre_; }
	VehicleDriveEvidence drive() const { return drive_; }
	float tolerance() const { return tolerance_; }

private:
	float overall_length_ = 0.0f;
	float overall_width_ = 0.0f;
	float overall_height_ = 0.0f;
	float wheelbase_ = 0.0f;
	float front_track_ = 0.0f;
	float rear_track_ = 0.0f;
	float front_overhang_ = 0.0f;
	float rear_overhang_ = 0.0f;
	glm::vec3 front_axle_centre_{0.0f};
	glm::vec3 rear_axle_centre_{0.0f};
	VehicleDriveEvidence drive_ = VehicleDriveEvidence::AllWheelDrive;
	float tolerance_ = 0.0f;
};

class OccupantPackage
{
public:
	OccupantPackage(
		std::string identifier,
		glm::vec3 h_point,
		glm::vec3 eye_point,
		glm::vec3 heel_point,
		AxisAlignedBounds knee_envelope,
		AxisAlignedBounds head_envelope,
		glm::vec3 torso_direction,
		AxisAlignedBounds reach_envelope)
		: identifier_(std::move(identifier)),
		  h_point_(h_point),
		  eye_point_(eye_point),
		  heel_point_(heel_point),
		  knee_envelope_(knee_envelope),
		  head_envelope_(head_envelope),
		  torso_direction_(torso_direction),
		  reach_envelope_(reach_envelope)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const glm::vec3 &hPoint() const { return h_point_; }
	const glm::vec3 &eyePoint() const { return eye_point_; }
	const glm::vec3 &heelPoint() const { return heel_point_; }
	const AxisAlignedBounds &kneeEnvelope() const { return knee_envelope_; }
	const AxisAlignedBounds &headEnvelope() const { return head_envelope_; }
	const glm::vec3 &torsoDirection() const { return torso_direction_; }
	const AxisAlignedBounds &reachEnvelope() const { return reach_envelope_; }

private:
	std::string identifier_;
	glm::vec3 h_point_{0.0f};
	glm::vec3 eye_point_{0.0f};
	glm::vec3 heel_point_{0.0f};
	AxisAlignedBounds knee_envelope_;
	AxisAlignedBounds head_envelope_;
	glm::vec3 torso_direction_{0.0f, 1.0f, 0.0f};
	AxisAlignedBounds reach_envelope_;
};
