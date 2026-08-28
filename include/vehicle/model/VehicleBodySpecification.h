#pragma once

#include "geometry/model/Curve3D.h"
#include "geometry/model/GeometryDetailLevel.h"

#include <glm/glm.hpp>

#include <string>
#include <utility>
#include <vector>

class VehicleBodySection
{
public:
	VehicleBodySection(
		std::string identifier,
		float station_z,
		std::vector<glm::vec2> right_half_profile)
		: identifier_(std::move(identifier)),
		  station_z_(station_z),
		  right_half_profile_(std::move(right_half_profile))
	{
	}

	const std::string &identifier() const { return identifier_; }
	float stationZ() const { return station_z_; }
	const std::vector<glm::vec2> &rightHalfProfile() const
	{
		return right_half_profile_;
	}

private:
	std::string identifier_;
	float station_z_ = 0.0f;
	std::vector<glm::vec2> right_half_profile_;
};

class VehicleSurfaceGuide
{
public:
	VehicleSurfaceGuide(std::string identifier, Curve3D curve)
		: identifier_(std::move(identifier)), curve_(std::move(curve))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const Curve3D &curve() const { return curve_; }

private:
	std::string identifier_;
	Curve3D curve_{Curve3DType::Line, {glm::vec3(0.0f), glm::vec3(1.0f)}};
};

class WheelArchSpecification
{
public:
	WheelArchSpecification(
		std::string identifier,
		glm::vec3 wheel_center,
		float tire_radius,
		float clearance,
		float arch_width,
		float flare)
		: identifier_(std::move(identifier)),
		  wheel_center_(wheel_center),
		  tire_radius_(tire_radius),
		  clearance_(clearance),
		  arch_width_(arch_width),
		  flare_(flare)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const glm::vec3 &wheelCenter() const { return wheel_center_; }
	float tireRadius() const { return tire_radius_; }
	float clearance() const { return clearance_; }
	float archWidth() const { return arch_width_; }
	float flare() const { return flare_; }
	float archRadius() const { return tire_radius_ + clearance_; }

private:
	std::string identifier_;
	glm::vec3 wheel_center_{0.0f};
	float tire_radius_ = 0.0f;
	float clearance_ = 0.0f;
	float arch_width_ = 0.0f;
	float flare_ = 0.0f;
};

class VehiclePanelCutSpecification
{
public:
	VehiclePanelCutSpecification(
		std::string identifier,
		Curve3D seam_curve,
		float gap_width,
		float gap_depth)
		: identifier_(std::move(identifier)),
		  seam_curve_(std::move(seam_curve)),
		  gap_width_(gap_width),
		  gap_depth_(gap_depth)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const Curve3D &seamCurve() const { return seam_curve_; }
	float gapWidth() const { return gap_width_; }
	float gapDepth() const { return gap_depth_; }

private:
	std::string identifier_;
	Curve3D seam_curve_{Curve3DType::Line, {glm::vec3(0.0f), glm::vec3(1.0f)}};
	float gap_width_ = 0.0f;
	float gap_depth_ = 0.0f;
};

class VehicleBodySpecification
{
public:
	VehicleBodySpecification(
		std::string object_identifier,
		std::string material_identifier,
		std::vector<VehicleBodySection> sections,
		std::vector<VehicleSurfaceGuide> guides,
		std::vector<WheelArchSpecification> wheel_arches,
		std::vector<VehiclePanelCutSpecification> panel_cuts,
		float shell_thickness,
		GeometryDetailLevel detail_level)
		: object_identifier_(std::move(object_identifier)),
		  material_identifier_(std::move(material_identifier)),
		  sections_(std::move(sections)),
		  guides_(std::move(guides)),
		  wheel_arches_(std::move(wheel_arches)),
		  panel_cuts_(std::move(panel_cuts)),
		  shell_thickness_(shell_thickness),
		  detail_level_(detail_level)
	{
	}

	const std::string &objectIdentifier() const { return object_identifier_; }
	const std::string &materialIdentifier() const { return material_identifier_; }
	const std::vector<VehicleBodySection> &sections() const { return sections_; }
	const std::vector<VehicleSurfaceGuide> &guides() const { return guides_; }
	const std::vector<WheelArchSpecification> &wheelArches() const
	{
		return wheel_arches_;
	}
	const std::vector<VehiclePanelCutSpecification> &panelCuts() const
	{
		return panel_cuts_;
	}
	float shellThickness() const { return shell_thickness_; }
	GeometryDetailLevel detailLevel() const { return detail_level_; }

private:
	std::string object_identifier_;
	std::string material_identifier_;
	std::vector<VehicleBodySection> sections_;
	std::vector<VehicleSurfaceGuide> guides_;
	std::vector<WheelArchSpecification> wheel_arches_;
	std::vector<VehiclePanelCutSpecification> panel_cuts_;
	float shell_thickness_ = 0.0f;
	GeometryDetailLevel detail_level_ = GeometryDetailLevel::Component;
};
