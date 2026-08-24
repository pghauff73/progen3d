#pragma once

#include "building/model/BuildingRoleId.h"

#include <string>
#include <utility>

class BuildingRoleDefinition {
public:
	BuildingRoleDefinition(BuildingRoleId role_id,
	                       std::string canonical_name,
	                       std::string purpose)
		: role_id_(std::move(role_id)),
		  canonical_name_(std::move(canonical_name)),
		  purpose_(std::move(purpose)) {}

	const BuildingRoleId &roleId() const { return role_id_; }
	const std::string &canonicalName() const { return canonical_name_; }
	const std::string &purpose() const { return purpose_; }

private:
	BuildingRoleId role_id_;
	std::string canonical_name_;
	std::string purpose_;
};
