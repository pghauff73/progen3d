#pragma once

#include <glm/glm.hpp>

enum class BuildingOrientationPolicyKind {
	AuthoredFixed,
	DerivedFromParent,
	KeepUpright,
	HostAligned,
	NotApplicable
};

enum class BuildingRotationalSymmetryKind {
	None,
	HalfTurn,
	QuarterTurn,
	ContinuousYaw,
	Axial,
	NotApplicable
};

class BuildingOrientationProfile {
public:
	BuildingOrientationProfile(
		BuildingOrientationPolicyKind policy,
		BuildingRotationalSymmetryKind symmetry,
		glm::vec3 local_front_axis,
		glm::vec3 local_right_axis,
		glm::vec3 local_up_axis)
		: policy_(policy),
		  symmetry_(symmetry),
		  local_front_axis_(local_front_axis),
		  local_right_axis_(local_right_axis),
		  local_up_axis_(local_up_axis) {}

	static BuildingOrientationProfile notApplicable()
	{
		return BuildingOrientationProfile(
			BuildingOrientationPolicyKind::NotApplicable,
			BuildingRotationalSymmetryKind::NotApplicable,
			glm::vec3(0.0f),
			glm::vec3(0.0f),
			glm::vec3(0.0f));
	}

	BuildingOrientationPolicyKind policy() const { return policy_; }
	BuildingRotationalSymmetryKind symmetry() const { return symmetry_; }
	const glm::vec3 &localFrontAxis() const { return local_front_axis_; }
	const glm::vec3 &localRightAxis() const { return local_right_axis_; }
	const glm::vec3 &localUpAxis() const { return local_up_axis_; }
	bool isApplicable() const
	{
		return policy_ != BuildingOrientationPolicyKind::NotApplicable;
	}

private:
	BuildingOrientationPolicyKind policy_ =
		BuildingOrientationPolicyKind::NotApplicable;
	BuildingRotationalSymmetryKind symmetry_ =
		BuildingRotationalSymmetryKind::NotApplicable;
	glm::vec3 local_front_axis_{0.0f};
	glm::vec3 local_right_axis_{0.0f};
	glm::vec3 local_up_axis_{0.0f};
};
