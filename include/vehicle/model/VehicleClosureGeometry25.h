#pragma once

#include "geometry/model/AxisAlignedBounds.h"
#include "geometry/model/Curve3D.h"
#include "vehicle/model/VehicleClassASurface.h"

#include <glm/glm.hpp>
#include <glm/geometric.hpp>

#include <string>
#include <utility>
#include <vector>

class AutomotiveScalarStation
{
public:
	AutomotiveScalarStation(float parameter, float value)
		: parameter_(parameter), value_(value)
	{
	}

	float parameter() const { return parameter_; }
	float value() const { return value_; }

private:
	float parameter_ = 0.0f;
	float value_ = 0.0f;
};

class AutomotiveFlange
{
public:
	AutomotiveFlange(
		std::string identifier,
		std::string contact_curve_identifier,
		float length,
		float angle_degrees)
		: identifier_(std::move(identifier)),
		  contact_curve_identifier_(std::move(contact_curve_identifier)),
		  length_(length),
		  angle_degrees_(angle_degrees)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &contactCurveIdentifier() const
	{
		return contact_curve_identifier_;
	}
	float length() const { return length_; }
	float angleDegrees() const { return angle_degrees_; }

private:
	std::string identifier_;
	std::string contact_curve_identifier_;
	float length_ = 0.0f;
	float angle_degrees_ = 0.0f;
};

class AutomotivePanelGap
{
public:
	AutomotivePanelGap(
		std::string identifier,
		Curve3D seam_curve,
		std::vector<AutomotiveScalarStation> gap_width_stations,
		std::vector<AutomotiveScalarStation> primary_radius_stations,
		std::vector<AutomotiveScalarStation> secondary_radius_stations,
		AutomotiveFlange primary_flange,
		AutomotiveFlange secondary_flange,
		PatchContinuityLevel primary_continuity,
		PatchContinuityLevel secondary_continuity,
		bool closeout)
		: identifier_(std::move(identifier)),
		  seam_curve_(std::move(seam_curve)),
		  gap_width_stations_(std::move(gap_width_stations)),
		  primary_radius_stations_(std::move(primary_radius_stations)),
		  secondary_radius_stations_(std::move(secondary_radius_stations)),
		  primary_flange_(std::move(primary_flange)),
		  secondary_flange_(std::move(secondary_flange)),
		  primary_continuity_(primary_continuity),
		  secondary_continuity_(secondary_continuity),
		  closeout_(closeout)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const Curve3D &seamCurve() const { return seam_curve_; }
	const std::vector<AutomotiveScalarStation> &gapWidthStations() const
	{
		return gap_width_stations_;
	}
	const std::vector<AutomotiveScalarStation> &primaryRadiusStations() const
	{
		return primary_radius_stations_;
	}
	const std::vector<AutomotiveScalarStation> &secondaryRadiusStations() const
	{
		return secondary_radius_stations_;
	}
	const AutomotiveFlange &primaryFlange() const { return primary_flange_; }
	const AutomotiveFlange &secondaryFlange() const { return secondary_flange_; }
	PatchContinuityLevel primaryContinuity() const { return primary_continuity_; }
	PatchContinuityLevel secondaryContinuity() const { return secondary_continuity_; }
	bool hasCloseout() const { return closeout_; }

private:
	std::string identifier_;
	Curve3D seam_curve_;
	std::vector<AutomotiveScalarStation> gap_width_stations_;
	std::vector<AutomotiveScalarStation> primary_radius_stations_;
	std::vector<AutomotiveScalarStation> secondary_radius_stations_;
	AutomotiveFlange primary_flange_;
	AutomotiveFlange secondary_flange_;
	PatchContinuityLevel primary_continuity_ = PatchContinuityLevel::G1;
	PatchContinuityLevel secondary_continuity_ = PatchContinuityLevel::G1;
	bool closeout_ = false;
};

class RolledEdge
{
public:
	RolledEdge(
		std::string identifier,
		Curve3D contact_curve,
		float radius,
		float flange_length,
		float flange_angle_degrees,
		PatchContinuityLevel continuity)
		: identifier_(std::move(identifier)),
		  contact_curve_(std::move(contact_curve)),
		  radius_(radius),
		  flange_length_(flange_length),
		  flange_angle_degrees_(flange_angle_degrees),
		  continuity_(continuity)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const Curve3D &contactCurve() const { return contact_curve_; }
	float radius() const { return radius_; }
	float flangeLength() const { return flange_length_; }
	float flangeAngleDegrees() const { return flange_angle_degrees_; }
	PatchContinuityLevel continuity() const { return continuity_; }

private:
	std::string identifier_;
	Curve3D contact_curve_;
	float radius_ = 0.0f;
	float flange_length_ = 0.0f;
	float flange_angle_degrees_ = 0.0f;
	PatchContinuityLevel continuity_ = PatchContinuityLevel::G1;
};

class SideGlassSurface
{
public:
	SideGlassSurface(
		std::string identifier,
		std::vector<glm::vec3> perimeter_points,
		AxisAlignedBounds local_bounds)
		: identifier_(std::move(identifier)),
		  perimeter_points_(std::move(perimeter_points)),
		  local_bounds_(local_bounds)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::vector<glm::vec3> &perimeterPoints() const
	{
		return perimeter_points_;
	}
	const AxisAlignedBounds &localBounds() const { return local_bounds_; }

private:
	std::string identifier_;
	std::vector<glm::vec3> perimeter_points_;
	AxisAlignedBounds local_bounds_;
};

class BodySideAperture
{
public:
	BodySideAperture(
		std::string identifier,
		std::string hinge_pillar_identifier,
		std::string a_pillar_identifier,
		std::string roof_rail_identifier,
		std::string b_pillar_identifier,
		std::string rocker_identifier,
		std::string c_pillar_identifier,
		std::string dogleg_identifier,
		Curve3D b_line,
		std::string j_surface_identifier,
		std::string glass_surface_identifier,
		std::string flange_identifier,
		std::string seal_seat_identifier)
		: identifier_(std::move(identifier)),
		  hinge_pillar_identifier_(std::move(hinge_pillar_identifier)),
		  a_pillar_identifier_(std::move(a_pillar_identifier)),
		  roof_rail_identifier_(std::move(roof_rail_identifier)),
		  b_pillar_identifier_(std::move(b_pillar_identifier)),
		  rocker_identifier_(std::move(rocker_identifier)),
		  c_pillar_identifier_(std::move(c_pillar_identifier)),
		  dogleg_identifier_(std::move(dogleg_identifier)),
		  b_line_(std::move(b_line)),
		  j_surface_identifier_(std::move(j_surface_identifier)),
		  glass_surface_identifier_(std::move(glass_surface_identifier)),
		  flange_identifier_(std::move(flange_identifier)),
		  seal_seat_identifier_(std::move(seal_seat_identifier))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &hingePillarIdentifier() const { return hinge_pillar_identifier_; }
	const std::string &aPillarIdentifier() const { return a_pillar_identifier_; }
	const std::string &roofRailIdentifier() const { return roof_rail_identifier_; }
	const std::string &bPillarIdentifier() const { return b_pillar_identifier_; }
	const std::string &rockerIdentifier() const { return rocker_identifier_; }
	const std::string &cPillarIdentifier() const { return c_pillar_identifier_; }
	const std::string &doglegIdentifier() const { return dogleg_identifier_; }
	const Curve3D &bLine() const { return b_line_; }
	const std::string &jSurfaceIdentifier() const { return j_surface_identifier_; }
	const std::string &glassSurfaceIdentifier() const { return glass_surface_identifier_; }
	const std::string &flangeIdentifier() const { return flange_identifier_; }
	const std::string &sealSeatIdentifier() const { return seal_seat_identifier_; }

private:
	std::string identifier_;
	std::string hinge_pillar_identifier_;
	std::string a_pillar_identifier_;
	std::string roof_rail_identifier_;
	std::string b_pillar_identifier_;
	std::string rocker_identifier_;
	std::string c_pillar_identifier_;
	std::string dogleg_identifier_;
	Curve3D b_line_;
	std::string j_surface_identifier_;
	std::string glass_surface_identifier_;
	std::string flange_identifier_;
	std::string seal_seat_identifier_;
};

class DoorEgressSurface
{
public:
	DoorEgressSurface(
		std::string identifier,
		std::string b_line_identifier,
		std::string side_glass_identifier,
		std::string j_surface_identifier,
		std::string belt_line_identifier,
		float upper_offset,
		float lower_offset)
		: identifier_(std::move(identifier)),
		  b_line_identifier_(std::move(b_line_identifier)),
		  side_glass_identifier_(std::move(side_glass_identifier)),
		  j_surface_identifier_(std::move(j_surface_identifier)),
		  belt_line_identifier_(std::move(belt_line_identifier)),
		  upper_offset_(upper_offset),
		  lower_offset_(lower_offset)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &bLineIdentifier() const { return b_line_identifier_; }
	const std::string &sideGlassIdentifier() const { return side_glass_identifier_; }
	const std::string &jSurfaceIdentifier() const { return j_surface_identifier_; }
	const std::string &beltLineIdentifier() const { return belt_line_identifier_; }
	float upperOffset() const { return upper_offset_; }
	float lowerOffset() const { return lower_offset_; }

private:
	std::string identifier_;
	std::string b_line_identifier_;
	std::string side_glass_identifier_;
	std::string j_surface_identifier_;
	std::string belt_line_identifier_;
	float upper_offset_ = 0.0f;
	float lower_offset_ = 0.0f;
};

class ClosureHingeStudy
{
public:
	ClosureHingeStudy(
		std::string identifier,
		glm::vec3 upper_hinge,
		glm::vec3 lower_hinge,
		float minimum_opening_degrees,
		float maximum_opening_degrees,
		std::vector<glm::vec2> rise_curve,
		AxisAlignedBounds source_bounds,
		AxisAlignedBounds swept_bounds)
		: identifier_(std::move(identifier)),
		  upper_hinge_(upper_hinge),
		  lower_hinge_(lower_hinge),
		  minimum_opening_degrees_(minimum_opening_degrees),
		  maximum_opening_degrees_(maximum_opening_degrees),
		  rise_curve_(std::move(rise_curve)),
		  source_bounds_(source_bounds),
		  swept_bounds_(swept_bounds)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const glm::vec3 &upperHinge() const { return upper_hinge_; }
	const glm::vec3 &lowerHinge() const { return lower_hinge_; }
	glm::vec3 axis() const
	{
		const glm::vec3 direction = upper_hinge_ - lower_hinge_;
		const float length = glm::length(direction);
		return length > 1.0e-6f ? direction / length : glm::vec3(0.0f, 1.0f, 0.0f);
	}
	float minimumOpeningDegrees() const { return minimum_opening_degrees_; }
	float maximumOpeningDegrees() const { return maximum_opening_degrees_; }
	const std::vector<glm::vec2> &riseCurve() const { return rise_curve_; }
	const AxisAlignedBounds &sourceBounds() const { return source_bounds_; }
	const AxisAlignedBounds &sweptBounds() const { return swept_bounds_; }

private:
	std::string identifier_;
	glm::vec3 upper_hinge_{0.0f};
	glm::vec3 lower_hinge_{0.0f};
	float minimum_opening_degrees_ = 0.0f;
	float maximum_opening_degrees_ = 0.0f;
	std::vector<glm::vec2> rise_curve_;
	AxisAlignedBounds source_bounds_;
	AxisAlignedBounds swept_bounds_;
};

class BarrelSurface
{
public:
	BarrelSurface(
		std::string identifier,
		glm::vec3 axis_origin,
		glm::vec3 axis_direction,
		float radius,
		float axial_length)
		: identifier_(std::move(identifier)),
		  axis_origin_(axis_origin),
		  axis_direction_(axis_direction),
		  radius_(radius),
		  axial_length_(axial_length)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const glm::vec3 &axisOrigin() const { return axis_origin_; }
	const glm::vec3 &axisDirection() const { return axis_direction_; }
	float radius() const { return radius_; }
	float axialLength() const { return axial_length_; }

private:
	std::string identifier_;
	glm::vec3 axis_origin_{0.0f};
	glm::vec3 axis_direction_{0.0f, 1.0f, 0.0f};
	float radius_ = 0.0f;
	float axial_length_ = 0.0f;
};

class GlassChannel
{
public:
	GlassChannel(
		std::string identifier,
		Curve3D centre_line,
		float channel_width,
		float channel_depth,
		float clearance)
		: identifier_(std::move(identifier)),
		  centre_line_(std::move(centre_line)),
		  channel_width_(channel_width),
		  channel_depth_(channel_depth),
		  clearance_(clearance)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const Curve3D &centreLine() const { return centre_line_; }
	float channelWidth() const { return channel_width_; }
	float channelDepth() const { return channel_depth_; }
	float clearance() const { return clearance_; }

private:
	std::string identifier_;
	Curve3D centre_line_;
	float channel_width_ = 0.0f;
	float channel_depth_ = 0.0f;
	float clearance_ = 0.0f;
};

class HelicalGlassDrop
{
public:
	HelicalGlassDrop(
		std::string identifier,
		std::string parent_closure_identifier,
		SideGlassSurface glass_surface,
		BarrelSurface barrel_surface,
		glm::vec3 helix_axis,
		float pitch,
		float rotation_rate_degrees,
		GlassChannel front_channel,
		GlassChannel rear_channel,
		float upper_limit,
		float lower_limit,
		AxisAlignedBounds door_cavity)
		: identifier_(std::move(identifier)),
		  parent_closure_identifier_(std::move(parent_closure_identifier)),
		  glass_surface_(std::move(glass_surface)),
		  barrel_surface_(std::move(barrel_surface)),
		  helix_axis_(helix_axis),
		  pitch_(pitch),
		  rotation_rate_degrees_(rotation_rate_degrees),
		  front_channel_(std::move(front_channel)),
		  rear_channel_(std::move(rear_channel)),
		  upper_limit_(upper_limit),
		  lower_limit_(lower_limit),
		  door_cavity_(door_cavity)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &parentClosureIdentifier() const
	{
		return parent_closure_identifier_;
	}
	const SideGlassSurface &glassSurface() const { return glass_surface_; }
	const BarrelSurface &barrelSurface() const { return barrel_surface_; }
	const glm::vec3 &helixAxis() const { return helix_axis_; }
	float pitch() const { return pitch_; }
	float rotationRateDegrees() const { return rotation_rate_degrees_; }
	const GlassChannel &frontChannel() const { return front_channel_; }
	const GlassChannel &rearChannel() const { return rear_channel_; }
	float upperLimit() const { return upper_limit_; }
	float lowerLimit() const { return lower_limit_; }
	const AxisAlignedBounds &doorCavity() const { return door_cavity_; }

private:
	std::string identifier_;
	std::string parent_closure_identifier_;
	SideGlassSurface glass_surface_;
	BarrelSurface barrel_surface_;
	glm::vec3 helix_axis_{0.0f, -1.0f, 0.0f};
	float pitch_ = 0.0f;
	float rotation_rate_degrees_ = 0.0f;
	GlassChannel front_channel_;
	GlassChannel rear_channel_;
	float upper_limit_ = 0.0f;
	float lower_limit_ = 1.0f;
	AxisAlignedBounds door_cavity_;
};

class VariableSealSectionStation
{
public:
	VariableSealSectionStation(
		float parameter,
		float width,
		float height,
		float compression_target)
		: parameter_(parameter),
		  width_(width),
		  height_(height),
		  compression_target_(compression_target)
	{
	}

	float parameter() const { return parameter_; }
	float width() const { return width_; }
	float height() const { return height_; }
	float compressionTarget() const { return compression_target_; }

private:
	float parameter_ = 0.0f;
	float width_ = 0.0f;
	float height_ = 0.0f;
	float compression_target_ = 0.0f;
};

class VariableSealSweep
{
public:
	VariableSealSweep(
		std::string identifier,
		Curve3D path,
		std::vector<VariableSealSectionStation> section_stations,
		std::string material,
		std::vector<std::string> contact_target_identifiers)
		: identifier_(std::move(identifier)),
		  path_(std::move(path)),
		  section_stations_(std::move(section_stations)),
		  material_(std::move(material)),
		  contact_target_identifiers_(std::move(contact_target_identifiers))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const Curve3D &path() const { return path_; }
	const std::vector<VariableSealSectionStation> &sectionStations() const
	{
		return section_stations_;
	}
	const std::string &material() const { return material_; }
	const std::vector<std::string> &contactTargetIdentifiers() const
	{
		return contact_target_identifiers_;
	}

private:
	std::string identifier_;
	Curve3D path_;
	std::vector<VariableSealSectionStation> section_stations_;
	std::string material_;
	std::vector<std::string> contact_target_identifiers_;
};

enum class AutomotiveClosureType
{
	FrontDoor,
	RearDoor,
	Bonnet,
	RearHatch,
	FuelFlap
};

class AutomotiveClosure
{
public:
	AutomotiveClosure(
		std::string identifier,
		AutomotiveClosureType type,
		std::string outer_class_a_patch_identifier,
		std::string aperture_identifier,
		std::string j_surface_identifier,
		std::string side_glass_identifier,
		ClosureHingeStudy hinge_study,
		std::vector<std::string> child_object_identifiers,
		AxisAlignedBounds closed_bounds)
		: identifier_(std::move(identifier)),
		  type_(type),
		  outer_class_a_patch_identifier_(std::move(outer_class_a_patch_identifier)),
		  aperture_identifier_(std::move(aperture_identifier)),
		  j_surface_identifier_(std::move(j_surface_identifier)),
		  side_glass_identifier_(std::move(side_glass_identifier)),
		  hinge_study_(std::move(hinge_study)),
		  child_object_identifiers_(std::move(child_object_identifiers)),
		  closed_bounds_(closed_bounds)
	{
	}

	const std::string &identifier() const { return identifier_; }
	AutomotiveClosureType type() const { return type_; }
	const std::string &outerClassAPatchIdentifier() const
	{
		return outer_class_a_patch_identifier_;
	}
	const std::string &apertureIdentifier() const { return aperture_identifier_; }
	const std::string &jSurfaceIdentifier() const { return j_surface_identifier_; }
	const std::string &sideGlassIdentifier() const { return side_glass_identifier_; }
	const ClosureHingeStudy &hingeStudy() const { return hinge_study_; }
	const std::vector<std::string> &childObjectIdentifiers() const
	{
		return child_object_identifiers_;
	}
	const AxisAlignedBounds &closedBounds() const { return closed_bounds_; }

private:
	std::string identifier_;
	AutomotiveClosureType type_ = AutomotiveClosureType::FrontDoor;
	std::string outer_class_a_patch_identifier_;
	std::string aperture_identifier_;
	std::string j_surface_identifier_;
	std::string side_glass_identifier_;
	ClosureHingeStudy hinge_study_;
	std::vector<std::string> child_object_identifiers_;
	AxisAlignedBounds closed_bounds_;
};
