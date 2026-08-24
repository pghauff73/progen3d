#pragma once

#include <glm/glm.hpp>

enum class BranchJunctionArmRole
{
	Parent,
	Child
};

inline const char *branchJunctionArmRoleName(BranchJunctionArmRole role)
{
	switch (role) {
	case BranchJunctionArmRole::Parent: return "Parent";
	case BranchJunctionArmRole::Child: return "Child";
	}
	return "Child";
}

class BranchJunctionArmSpecification
{
public:
	BranchJunctionArmSpecification(
		BranchJunctionArmRole role,
		glm::vec3 direction,
		float radius,
		float transition_length)
		: role_(role),
		  direction_(direction),
		  radius_(radius),
		  transition_length_(transition_length)
	{
	}

	BranchJunctionArmRole role() const { return role_; }
	const glm::vec3 &direction() const { return direction_; }
	float radius() const { return radius_; }
	float transitionLength() const { return transition_length_; }

private:
	BranchJunctionArmRole role_ = BranchJunctionArmRole::Child;
	glm::vec3 direction_{0.0f, 1.0f, 0.0f};
	float radius_ = 0.0f;
	float transition_length_ = 0.0f;
};
