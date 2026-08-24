#pragma once

#include <string>
#include <utility>

enum class BodyInWhiteMemberGeometryFamily
{
	VariableSectionSweep,
	FormedPanel,
	CompoundShape
};

class BodyInWhiteStructuralMember
{
public:
	BodyInWhiteStructuralMember(
		std::string identifier,
		std::string role,
		BodyInWhiteMemberGeometryFamily geometry_family)
		: identifier_(std::move(identifier)),
		  role_(std::move(role)),
		  geometry_family_(geometry_family)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &role() const { return role_; }
	BodyInWhiteMemberGeometryFamily geometryFamily() const
	{
		return geometry_family_;
	}

private:
	std::string identifier_;
	std::string role_;
	BodyInWhiteMemberGeometryFamily geometry_family_ =
		BodyInWhiteMemberGeometryFamily::VariableSectionSweep;
};
