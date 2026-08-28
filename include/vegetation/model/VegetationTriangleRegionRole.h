#pragma once

enum class VegetationTriangleRegionRole
{
	RootFlare,
	PrimaryStem,
	BranchJunction,
	BranchSegment,
	FoliageSilhouette,
	FoliageInterior,
	FlowerOrFruit,
	GroundContact,
	SupportStructure
};

inline const char *vegetationTriangleRegionRoleName(
	VegetationTriangleRegionRole role)
{
	switch (role) {
	case VegetationTriangleRegionRole::RootFlare: return "RootFlare";
	case VegetationTriangleRegionRole::PrimaryStem: return "PrimaryStem";
	case VegetationTriangleRegionRole::BranchJunction: return "BranchJunction";
	case VegetationTriangleRegionRole::BranchSegment: return "BranchSegment";
	case VegetationTriangleRegionRole::FoliageSilhouette: return "FoliageSilhouette";
	case VegetationTriangleRegionRole::FoliageInterior: return "FoliageInterior";
	case VegetationTriangleRegionRole::FlowerOrFruit: return "FlowerOrFruit";
	case VegetationTriangleRegionRole::GroundContact: return "GroundContact";
	case VegetationTriangleRegionRole::SupportStructure: return "SupportStructure";
	}
	return "Unknown";
}

inline bool vegetationTriangleRegionPreservesOrganArea(
	VegetationTriangleRegionRole role)
{
	return role == VegetationTriangleRegionRole::FoliageSilhouette ||
	       role == VegetationTriangleRegionRole::FoliageInterior ||
	       role == VegetationTriangleRegionRole::FlowerOrFruit;
}
