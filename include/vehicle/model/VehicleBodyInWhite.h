#pragma once

#include "geometry/model/AxisAlignedBounds.h"
#include "geometry/model/Curve3D.h"

#include <string>
#include <utility>
#include <vector>

class AutomotiveStructuralSectionStation
{
public:
	AutomotiveStructuralSectionStation(
		float parameter,
		float width,
		float height,
		float wall_thickness,
		float flange_width)
		: parameter_(parameter),
		  width_(width),
		  height_(height),
		  wall_thickness_(wall_thickness),
		  flange_width_(flange_width)
	{
	}

	float parameter() const { return parameter_; }
	float width() const { return width_; }
	float height() const { return height_; }
	float wallThickness() const { return wall_thickness_; }
	float flangeWidth() const { return flange_width_; }

private:
	float parameter_ = 0.0f;
	float width_ = 0.0f;
	float height_ = 0.0f;
	float wall_thickness_ = 0.0f;
	float flange_width_ = 0.0f;
};

class AutomotiveStructuralMember
{
public:
	AutomotiveStructuralMember(
		std::string identifier,
		std::string structural_role,
		Curve3D centre_line,
		std::vector<AutomotiveStructuralSectionStation> section_stations,
		std::vector<AxisAlignedBounds> holes,
		std::vector<std::string> reinforcement_identifiers,
		std::string material,
		std::string provenance)
		: identifier_(std::move(identifier)),
		  structural_role_(std::move(structural_role)),
		  centre_line_(std::move(centre_line)),
		  section_stations_(std::move(section_stations)),
		  holes_(std::move(holes)),
		  reinforcement_identifiers_(std::move(reinforcement_identifiers)),
		  material_(std::move(material)),
		  provenance_(std::move(provenance))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &structuralRole() const { return structural_role_; }
	const Curve3D &centreLine() const { return centre_line_; }
	const std::vector<AutomotiveStructuralSectionStation> &sectionStations() const
	{
		return section_stations_;
	}
	const std::vector<AxisAlignedBounds> &holes() const { return holes_; }
	const std::vector<std::string> &reinforcementIdentifiers() const
	{
		return reinforcement_identifiers_;
	}
	const std::string &material() const { return material_; }
	const std::string &provenance() const { return provenance_; }

private:
	std::string identifier_;
	std::string structural_role_;
	Curve3D centre_line_;
	std::vector<AutomotiveStructuralSectionStation> section_stations_;
	std::vector<AxisAlignedBounds> holes_;
	std::vector<std::string> reinforcement_identifiers_;
	std::string material_;
	std::string provenance_;
};

enum class BIWJoiningMethod
{
	SpotWeld,
	LaserWeld,
	StructuralAdhesive,
	Rivet,
	Bolt,
	Mixed
};

class BIWJoint
{
public:
	BIWJoint(
		std::string identifier,
		std::vector<std::string> member_identifiers,
		std::vector<std::string> overlap_surface_identifiers,
		BIWJoiningMethod joining_method,
		std::string local_reinforcement_identifier,
		glm::vec3 joint_position)
		: identifier_(std::move(identifier)),
		  member_identifiers_(std::move(member_identifiers)),
		  overlap_surface_identifiers_(std::move(overlap_surface_identifiers)),
		  joining_method_(joining_method),
		  local_reinforcement_identifier_(std::move(local_reinforcement_identifier)),
		  joint_position_(joint_position)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::vector<std::string> &memberIdentifiers() const
	{
		return member_identifiers_;
	}
	const std::vector<std::string> &overlapSurfaceIdentifiers() const
	{
		return overlap_surface_identifiers_;
	}
	BIWJoiningMethod joiningMethod() const { return joining_method_; }
	const std::string &localReinforcementIdentifier() const
	{
		return local_reinforcement_identifier_;
	}
	const glm::vec3 &jointPosition() const { return joint_position_; }

private:
	std::string identifier_;
	std::vector<std::string> member_identifiers_;
	std::vector<std::string> overlap_surface_identifiers_;
	BIWJoiningMethod joining_method_ = BIWJoiningMethod::SpotWeld;
	std::string local_reinforcement_identifier_;
	glm::vec3 joint_position_{0.0f};
};

class BodyInWhite
{
public:
	BodyInWhite(
		std::string identifier,
		std::vector<AutomotiveStructuralMember> members,
		std::vector<BIWJoint> joints)
		: identifier_(std::move(identifier)),
		  members_(std::move(members)),
		  joints_(std::move(joints))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::vector<AutomotiveStructuralMember> &members() const { return members_; }
	const std::vector<BIWJoint> &joints() const { return joints_; }
	const AutomotiveStructuralMember *findMember(const std::string &identifier) const
	{
		for (const AutomotiveStructuralMember &member : members_) {
			if (member.identifier() == identifier) return &member;
		}
		return nullptr;
	}

private:
	std::string identifier_;
	std::vector<AutomotiveStructuralMember> members_;
	std::vector<BIWJoint> joints_;
};
