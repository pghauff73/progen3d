#pragma once

#include "vehicle/mcsmv2/model/BodyInWhiteJointRelationship.h"
#include "vehicle/mcsmv2/model/BodyInWhiteStructuralMember.h"

#include <string>
#include <utility>
#include <vector>

class VehicleBodyInWhiteAssembly
{
public:
	VehicleBodyInWhiteAssembly(
		std::string variant_identifier,
		std::vector<BodyInWhiteStructuralMember> members,
		std::vector<BodyInWhiteJointRelationship> joints)
		: variant_identifier_(std::move(variant_identifier)),
		  members_(std::move(members)),
		  joints_(std::move(joints))
	{
	}

	const std::string &variantIdentifier() const { return variant_identifier_; }
	const std::vector<BodyInWhiteStructuralMember> &members() const
	{
		return members_;
	}
	const std::vector<BodyInWhiteJointRelationship> &joints() const
	{
		return joints_;
	}

private:
	std::string variant_identifier_;
	std::vector<BodyInWhiteStructuralMember> members_;
	std::vector<BodyInWhiteJointRelationship> joints_;
};
