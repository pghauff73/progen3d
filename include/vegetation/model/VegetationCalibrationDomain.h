#pragma once

#include <optional>
#include <string>

enum class VegetationCalibrationDomain
{
	TaxonomicIdentity,
	ShootTopology,
	RootArchitecture,
	CanopyOptics,
	Phenology,
	Biomechanics,
	EnvironmentalResponse,
	SemanticAnnotation,
	FunctionalTraits,
	Hydraulics,
	SizeAllometry,
	SubstrateSuitability,
	BuildingPlacement,
	RepresentationFidelity
};

inline const char *vegetationCalibrationDomainName(
	VegetationCalibrationDomain domain)
{
	switch (domain) {
	case VegetationCalibrationDomain::TaxonomicIdentity:
		return "TaxonomicIdentity";
	case VegetationCalibrationDomain::ShootTopology:
		return "ShootTopology";
	case VegetationCalibrationDomain::RootArchitecture:
		return "RootArchitecture";
	case VegetationCalibrationDomain::CanopyOptics:
		return "CanopyOptics";
	case VegetationCalibrationDomain::Phenology:
		return "Phenology";
	case VegetationCalibrationDomain::Biomechanics:
		return "Biomechanics";
	case VegetationCalibrationDomain::EnvironmentalResponse:
		return "EnvironmentalResponse";
	case VegetationCalibrationDomain::SemanticAnnotation:
		return "SemanticAnnotation";
	case VegetationCalibrationDomain::FunctionalTraits:
		return "FunctionalTraits";
	case VegetationCalibrationDomain::Hydraulics:
		return "Hydraulics";
	case VegetationCalibrationDomain::SizeAllometry:
		return "SizeAllometry";
	case VegetationCalibrationDomain::SubstrateSuitability:
		return "SubstrateSuitability";
	case VegetationCalibrationDomain::BuildingPlacement:
		return "BuildingPlacement";
	case VegetationCalibrationDomain::RepresentationFidelity:
		return "RepresentationFidelity";
	}
	return "Unknown";
}

inline std::optional<VegetationCalibrationDomain>
vegetationCalibrationDomainFromName(const std::string &name)
{
	for (VegetationCalibrationDomain domain : {
	     VegetationCalibrationDomain::TaxonomicIdentity,
	     VegetationCalibrationDomain::ShootTopology,
	     VegetationCalibrationDomain::RootArchitecture,
	     VegetationCalibrationDomain::CanopyOptics,
	     VegetationCalibrationDomain::Phenology,
	     VegetationCalibrationDomain::Biomechanics,
	     VegetationCalibrationDomain::EnvironmentalResponse,
	     VegetationCalibrationDomain::SemanticAnnotation,
	     VegetationCalibrationDomain::FunctionalTraits,
	     VegetationCalibrationDomain::Hydraulics,
	     VegetationCalibrationDomain::SizeAllometry,
	     VegetationCalibrationDomain::SubstrateSuitability,
	     VegetationCalibrationDomain::BuildingPlacement,
	     VegetationCalibrationDomain::RepresentationFidelity}) {
		if (name == vegetationCalibrationDomainName(domain)) return domain;
	}
	return std::nullopt;
}
