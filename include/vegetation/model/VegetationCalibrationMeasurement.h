#pragma once

#include "vegetation/model/VegetationCalibrationDomain.h"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

enum class VegetationCalibrationMeasurementKind
{
	ScientificName,
	CultivarName,
	SpecimenIdentifier,
	ShootTopologyArtifactIdentifier,
	MaximumBranchOrder,
	RootGraphIdentifier,
	MaximumRootDepthMetres,
	MaximumRootRadialSpreadMetres,
	MaximumRootOrder,
	LeafAreaIndex,
	CrownGapFraction,
	MeanLeafInclinationDegrees,
	SeasonalScheduleArtifactIdentifier,
	MassDensityKilogramsPerCubicMetre,
	ElasticModulusPascals,
	DampingRatio,
	DragCoefficient,
	EnvironmentalResponseArtifactIdentifier,
	OntologyIdentifier,
	OntologyReleaseIdentifier,
	AnatomicalEntityIdentifiers,
	DevelopmentStageIdentifiers,
	SpecificLeafAreaSquareMetresPerKilogram,
	LeafDryMatterContentKilogramsPerKilogram,
	LeafNitrogenContentKilogramsPerKilogram,
	MaximumMatureHeightMetres,
	MaximumLeafSpecificConductanceMillimolesPerSquareMetrePerSecondPerMegapascal,
	HydraulicCapacitanceKilogramsPerMegapascal,
	XylemWaterPotentialAtFiftyPercentConductivityLossMegapascals,
	StomatalWaterPotentialAtFiftyPercentConductanceLossMegapascals,
	LeafToSapwoodAreaRatio,
	ReferenceHeightMetres,
	ReferenceHorizontalRadiusMetres,
	ReferenceSupportingAxisDiameterMetres,
	SizeRelationshipModelIdentifier,
	MinimumRootableVolumeCubicMetres,
	MinimumRootableDepthMetres,
	MaximumBulkDensityKilogramsPerCubicMetre,
	MinimumAirFilledPorosityFraction,
	MinimumAvailableWaterCapacityFraction,
	GrammarEvidenceIdentifier,
	CameraEvidenceIdentifier
};

enum class VegetationCalibrationMeasurementValueKind
{
	Decimal,
	Count,
	Text,
	IdentifierList
};

const char *vegetationCalibrationMeasurementKindName(
	VegetationCalibrationMeasurementKind kind);
std::optional<VegetationCalibrationMeasurementKind>
vegetationCalibrationMeasurementKindFromName(const std::string &name);
VegetationCalibrationDomain vegetationCalibrationMeasurementDomain(
	VegetationCalibrationMeasurementKind kind);
const char *vegetationCalibrationMeasurementValueKindName(
	VegetationCalibrationMeasurementValueKind kind);
std::optional<VegetationCalibrationMeasurementValueKind>
vegetationCalibrationMeasurementValueKindFromName(const std::string &name);

class VegetationCalibrationMeasurementValue
{
public:
	using Storage = std::variant<
		double,
		std::size_t,
		std::string,
		std::vector<std::string>>;

	explicit VegetationCalibrationMeasurementValue(double decimal_value)
		: storage_(decimal_value)
	{
	}
	explicit VegetationCalibrationMeasurementValue(std::size_t count_value)
		: storage_(count_value)
	{
	}
	explicit VegetationCalibrationMeasurementValue(std::string text_value)
		: storage_(std::move(text_value))
	{
	}
	explicit VegetationCalibrationMeasurementValue(
		std::vector<std::string> identifier_values)
		: storage_(std::move(identifier_values))
	{
	}

	VegetationCalibrationMeasurementValueKind kind() const;
	std::optional<double> decimalValue() const;
	std::optional<std::size_t> countValue() const;
	const std::string *textValue() const;
	const std::vector<std::string> *identifierListValue() const;

private:
	Storage storage_;
};

class VegetationCalibrationMeasurement
{
public:
	VegetationCalibrationMeasurement(
		VegetationCalibrationMeasurementKind kind,
		std::string declared_domain_name,
		VegetationCalibrationMeasurementValue value,
		std::string unit,
		std::string observation_scope,
		std::string uncertainty_statement,
		std::vector<std::string> evidence_source_identifiers)
		: kind_(kind),
		  declared_domain_name_(std::move(declared_domain_name)),
		  value_(std::move(value)),
		  unit_(std::move(unit)),
		  observation_scope_(std::move(observation_scope)),
		  uncertainty_statement_(std::move(uncertainty_statement)),
		  evidence_source_identifiers_(
			  std::move(evidence_source_identifiers))
	{
	}

	VegetationCalibrationMeasurementKind kind() const { return kind_; }
	const std::string &declaredDomainName() const
	{
		return declared_domain_name_;
	}
	const VegetationCalibrationMeasurementValue &value() const
	{
		return value_;
	}
	const std::string &unit() const { return unit_; }
	const std::string &observationScope() const
	{
		return observation_scope_;
	}
	const std::string &uncertaintyStatement() const
	{
		return uncertainty_statement_;
	}
	const std::vector<std::string> &evidenceSourceIdentifiers() const
	{
		return evidence_source_identifiers_;
	}

private:
	VegetationCalibrationMeasurementKind kind_ =
		VegetationCalibrationMeasurementKind::ScientificName;
	std::string declared_domain_name_;
	VegetationCalibrationMeasurementValue value_{std::string()};
	std::string unit_;
	std::string observation_scope_;
	std::string uncertainty_statement_;
	std::vector<std::string> evidence_source_identifiers_;
};
