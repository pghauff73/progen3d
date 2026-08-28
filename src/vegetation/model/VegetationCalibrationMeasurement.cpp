#include "vegetation/model/VegetationCalibrationMeasurement.h"

#include <array>

namespace {

using MeasurementKind = VegetationCalibrationMeasurementKind;

struct MeasurementKindName
{
	MeasurementKind kind;
	const char *name;
};

constexpr std::array<MeasurementKindName, 42> measurement_kind_names{{
	{MeasurementKind::ScientificName, "ScientificName"},
	{MeasurementKind::CultivarName, "CultivarName"},
	{MeasurementKind::SpecimenIdentifier, "SpecimenIdentifier"},
	{MeasurementKind::ShootTopologyArtifactIdentifier,
	 "ShootTopologyArtifactIdentifier"},
	{MeasurementKind::MaximumBranchOrder, "MaximumBranchOrder"},
	{MeasurementKind::RootGraphIdentifier, "RootGraphIdentifier"},
	{MeasurementKind::MaximumRootDepthMetres, "MaximumRootDepthMetres"},
	{MeasurementKind::MaximumRootRadialSpreadMetres,
	 "MaximumRootRadialSpreadMetres"},
	{MeasurementKind::MaximumRootOrder, "MaximumRootOrder"},
	{MeasurementKind::LeafAreaIndex, "LeafAreaIndex"},
	{MeasurementKind::CrownGapFraction, "CrownGapFraction"},
	{MeasurementKind::MeanLeafInclinationDegrees,
	 "MeanLeafInclinationDegrees"},
	{MeasurementKind::SeasonalScheduleArtifactIdentifier,
	 "SeasonalScheduleArtifactIdentifier"},
	{MeasurementKind::MassDensityKilogramsPerCubicMetre,
	 "MassDensityKilogramsPerCubicMetre"},
	{MeasurementKind::ElasticModulusPascals, "ElasticModulusPascals"},
	{MeasurementKind::DampingRatio, "DampingRatio"},
	{MeasurementKind::DragCoefficient, "DragCoefficient"},
	{MeasurementKind::EnvironmentalResponseArtifactIdentifier,
	 "EnvironmentalResponseArtifactIdentifier"},
	{MeasurementKind::OntologyIdentifier, "OntologyIdentifier"},
	{MeasurementKind::OntologyReleaseIdentifier, "OntologyReleaseIdentifier"},
	{MeasurementKind::AnatomicalEntityIdentifiers,
	 "AnatomicalEntityIdentifiers"},
	{MeasurementKind::DevelopmentStageIdentifiers,
	 "DevelopmentStageIdentifiers"},
	{MeasurementKind::SpecificLeafAreaSquareMetresPerKilogram,
	 "SpecificLeafAreaSquareMetresPerKilogram"},
	{MeasurementKind::LeafDryMatterContentKilogramsPerKilogram,
	 "LeafDryMatterContentKilogramsPerKilogram"},
	{MeasurementKind::LeafNitrogenContentKilogramsPerKilogram,
	 "LeafNitrogenContentKilogramsPerKilogram"},
	{MeasurementKind::MaximumMatureHeightMetres,
	 "MaximumMatureHeightMetres"},
	{MeasurementKind::MaximumLeafSpecificConductanceMillimolesPerSquareMetrePerSecondPerMegapascal,
	 "MaximumLeafSpecificConductanceMillimolesPerSquareMetrePerSecondPerMegapascal"},
	{MeasurementKind::HydraulicCapacitanceKilogramsPerMegapascal,
	 "HydraulicCapacitanceKilogramsPerMegapascal"},
	{MeasurementKind::XylemWaterPotentialAtFiftyPercentConductivityLossMegapascals,
	 "XylemWaterPotentialAtFiftyPercentConductivityLossMegapascals"},
	{MeasurementKind::StomatalWaterPotentialAtFiftyPercentConductanceLossMegapascals,
	 "StomatalWaterPotentialAtFiftyPercentConductanceLossMegapascals"},
	{MeasurementKind::LeafToSapwoodAreaRatio, "LeafToSapwoodAreaRatio"},
	{MeasurementKind::ReferenceHeightMetres, "ReferenceHeightMetres"},
	{MeasurementKind::ReferenceHorizontalRadiusMetres,
	 "ReferenceHorizontalRadiusMetres"},
	{MeasurementKind::ReferenceSupportingAxisDiameterMetres,
	 "ReferenceSupportingAxisDiameterMetres"},
	{MeasurementKind::SizeRelationshipModelIdentifier,
	 "SizeRelationshipModelIdentifier"},
	{MeasurementKind::MinimumRootableVolumeCubicMetres,
	 "MinimumRootableVolumeCubicMetres"},
	{MeasurementKind::MinimumRootableDepthMetres,
	 "MinimumRootableDepthMetres"},
	{MeasurementKind::MaximumBulkDensityKilogramsPerCubicMetre,
	 "MaximumBulkDensityKilogramsPerCubicMetre"},
	{MeasurementKind::MinimumAirFilledPorosityFraction,
	 "MinimumAirFilledPorosityFraction"},
	{MeasurementKind::MinimumAvailableWaterCapacityFraction,
	 "MinimumAvailableWaterCapacityFraction"},
	{MeasurementKind::GrammarEvidenceIdentifier,
	 "GrammarEvidenceIdentifier"},
	{MeasurementKind::CameraEvidenceIdentifier,
	 "CameraEvidenceIdentifier"},
}};

}

const char *vegetationCalibrationMeasurementKindName(
	VegetationCalibrationMeasurementKind kind)
{
	for (const MeasurementKindName &entry : measurement_kind_names) {
		if (entry.kind == kind) return entry.name;
	}
	return "Unknown";
}

std::optional<VegetationCalibrationMeasurementKind>
vegetationCalibrationMeasurementKindFromName(const std::string &name)
{
	for (const MeasurementKindName &entry : measurement_kind_names) {
		if (name == entry.name) return entry.kind;
	}
	return std::nullopt;
}

VegetationCalibrationDomain vegetationCalibrationMeasurementDomain(
	VegetationCalibrationMeasurementKind kind)
{
	switch (kind) {
	case MeasurementKind::ScientificName:
	case MeasurementKind::CultivarName:
	case MeasurementKind::SpecimenIdentifier:
		return VegetationCalibrationDomain::TaxonomicIdentity;
	case MeasurementKind::ShootTopologyArtifactIdentifier:
	case MeasurementKind::MaximumBranchOrder:
		return VegetationCalibrationDomain::ShootTopology;
	case MeasurementKind::RootGraphIdentifier:
	case MeasurementKind::MaximumRootDepthMetres:
	case MeasurementKind::MaximumRootRadialSpreadMetres:
	case MeasurementKind::MaximumRootOrder:
		return VegetationCalibrationDomain::RootArchitecture;
	case MeasurementKind::LeafAreaIndex:
	case MeasurementKind::CrownGapFraction:
	case MeasurementKind::MeanLeafInclinationDegrees:
		return VegetationCalibrationDomain::CanopyOptics;
	case MeasurementKind::SeasonalScheduleArtifactIdentifier:
		return VegetationCalibrationDomain::Phenology;
	case MeasurementKind::MassDensityKilogramsPerCubicMetre:
	case MeasurementKind::ElasticModulusPascals:
	case MeasurementKind::DampingRatio:
	case MeasurementKind::DragCoefficient:
		return VegetationCalibrationDomain::Biomechanics;
	case MeasurementKind::EnvironmentalResponseArtifactIdentifier:
		return VegetationCalibrationDomain::EnvironmentalResponse;
	case MeasurementKind::OntologyIdentifier:
	case MeasurementKind::OntologyReleaseIdentifier:
	case MeasurementKind::AnatomicalEntityIdentifiers:
	case MeasurementKind::DevelopmentStageIdentifiers:
		return VegetationCalibrationDomain::SemanticAnnotation;
	case MeasurementKind::SpecificLeafAreaSquareMetresPerKilogram:
	case MeasurementKind::LeafDryMatterContentKilogramsPerKilogram:
	case MeasurementKind::LeafNitrogenContentKilogramsPerKilogram:
	case MeasurementKind::MaximumMatureHeightMetres:
		return VegetationCalibrationDomain::FunctionalTraits;
	case MeasurementKind::MaximumLeafSpecificConductanceMillimolesPerSquareMetrePerSecondPerMegapascal:
	case MeasurementKind::HydraulicCapacitanceKilogramsPerMegapascal:
	case MeasurementKind::XylemWaterPotentialAtFiftyPercentConductivityLossMegapascals:
	case MeasurementKind::StomatalWaterPotentialAtFiftyPercentConductanceLossMegapascals:
	case MeasurementKind::LeafToSapwoodAreaRatio:
		return VegetationCalibrationDomain::Hydraulics;
	case MeasurementKind::ReferenceHeightMetres:
	case MeasurementKind::ReferenceHorizontalRadiusMetres:
	case MeasurementKind::ReferenceSupportingAxisDiameterMetres:
	case MeasurementKind::SizeRelationshipModelIdentifier:
		return VegetationCalibrationDomain::SizeAllometry;
	case MeasurementKind::MinimumRootableVolumeCubicMetres:
	case MeasurementKind::MinimumRootableDepthMetres:
	case MeasurementKind::MaximumBulkDensityKilogramsPerCubicMetre:
	case MeasurementKind::MinimumAirFilledPorosityFraction:
	case MeasurementKind::MinimumAvailableWaterCapacityFraction:
		return VegetationCalibrationDomain::SubstrateSuitability;
	case MeasurementKind::GrammarEvidenceIdentifier:
	case MeasurementKind::CameraEvidenceIdentifier:
		return VegetationCalibrationDomain::RepresentationFidelity;
	}
	return VegetationCalibrationDomain::TaxonomicIdentity;
}

const char *vegetationCalibrationMeasurementValueKindName(
	VegetationCalibrationMeasurementValueKind kind)
{
	switch (kind) {
	case VegetationCalibrationMeasurementValueKind::Decimal: return "Decimal";
	case VegetationCalibrationMeasurementValueKind::Count: return "Count";
	case VegetationCalibrationMeasurementValueKind::Text: return "Text";
	case VegetationCalibrationMeasurementValueKind::IdentifierList:
		return "IdentifierList";
	}
	return "Unknown";
}

std::optional<VegetationCalibrationMeasurementValueKind>
vegetationCalibrationMeasurementValueKindFromName(const std::string &name)
{
	if (name == "Decimal") {
		return VegetationCalibrationMeasurementValueKind::Decimal;
	}
	if (name == "Count") {
		return VegetationCalibrationMeasurementValueKind::Count;
	}
	if (name == "Text") {
		return VegetationCalibrationMeasurementValueKind::Text;
	}
	if (name == "IdentifierList") {
		return VegetationCalibrationMeasurementValueKind::IdentifierList;
	}
	return std::nullopt;
}

VegetationCalibrationMeasurementValueKind
VegetationCalibrationMeasurementValue::kind() const
{
	if (std::holds_alternative<double>(storage_)) {
		return VegetationCalibrationMeasurementValueKind::Decimal;
	}
	if (std::holds_alternative<std::size_t>(storage_)) {
		return VegetationCalibrationMeasurementValueKind::Count;
	}
	if (std::holds_alternative<std::string>(storage_)) {
		return VegetationCalibrationMeasurementValueKind::Text;
	}
	return VegetationCalibrationMeasurementValueKind::IdentifierList;
}

std::optional<double> VegetationCalibrationMeasurementValue::decimalValue() const
{
	if (const double *value = std::get_if<double>(&storage_)) return *value;
	return std::nullopt;
}

std::optional<std::size_t>
VegetationCalibrationMeasurementValue::countValue() const
{
	if (const std::size_t *value = std::get_if<std::size_t>(&storage_)) {
		return *value;
	}
	return std::nullopt;
}

const std::string *VegetationCalibrationMeasurementValue::textValue() const
{
	return std::get_if<std::string>(&storage_);
}

const std::vector<std::string> *
VegetationCalibrationMeasurementValue::identifierListValue() const
{
	return std::get_if<std::vector<std::string>>(&storage_);
}
