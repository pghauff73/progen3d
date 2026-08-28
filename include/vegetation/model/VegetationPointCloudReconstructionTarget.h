#pragma once

enum class VegetationPointCloudReconstructionTarget
{
	ShootArchitecture,
	CanopyOptics
};

inline const char *vegetationPointCloudReconstructionTargetName(
	VegetationPointCloudReconstructionTarget target)
{
	switch (target) {
	case VegetationPointCloudReconstructionTarget::ShootArchitecture:
		return "ShootArchitecture";
	case VegetationPointCloudReconstructionTarget::CanopyOptics:
		return "CanopyOptics";
	}
	return "Unknown";
}
