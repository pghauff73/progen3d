#include "vegetation/service/VegetationCalibrationEvidenceBundleValidationService.h"

#include "vegetation/service/VegetationCalibrationPayloadHashService.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <optional>
#include <set>
#include <string>

namespace {

using MeasurementKind = VegetationCalibrationMeasurementKind;
using ValueKind = VegetationCalibrationMeasurementValueKind;
using ValidationCode =
	VegetationCalibrationEvidenceBundleValidationCode;

ValueKind expected_value_kind(MeasurementKind kind)
{
	switch (kind) {
	case MeasurementKind::MaximumBranchOrder:
	case MeasurementKind::MaximumRootOrder:
		return ValueKind::Count;
	case MeasurementKind::AnatomicalEntityIdentifiers:
	case MeasurementKind::DevelopmentStageIdentifiers:
		return ValueKind::IdentifierList;
	case MeasurementKind::ScientificName:
	case MeasurementKind::CultivarName:
	case MeasurementKind::SpecimenIdentifier:
	case MeasurementKind::ShootTopologyArtifactIdentifier:
	case MeasurementKind::RootGraphIdentifier:
	case MeasurementKind::SeasonalScheduleArtifactIdentifier:
	case MeasurementKind::EnvironmentalResponseArtifactIdentifier:
	case MeasurementKind::OntologyIdentifier:
	case MeasurementKind::OntologyReleaseIdentifier:
	case MeasurementKind::SizeRelationshipModelIdentifier:
	case MeasurementKind::GrammarEvidenceIdentifier:
	case MeasurementKind::CameraEvidenceIdentifier:
		return ValueKind::Text;
	case MeasurementKind::MaximumRootDepthMetres:
	case MeasurementKind::MaximumRootRadialSpreadMetres:
	case MeasurementKind::LeafAreaIndex:
	case MeasurementKind::CrownGapFraction:
	case MeasurementKind::MeanLeafInclinationDegrees:
	case MeasurementKind::MassDensityKilogramsPerCubicMetre:
	case MeasurementKind::ElasticModulusPascals:
	case MeasurementKind::DampingRatio:
	case MeasurementKind::DragCoefficient:
	case MeasurementKind::SpecificLeafAreaSquareMetresPerKilogram:
	case MeasurementKind::LeafDryMatterContentKilogramsPerKilogram:
	case MeasurementKind::LeafNitrogenContentKilogramsPerKilogram:
	case MeasurementKind::MaximumMatureHeightMetres:
	case MeasurementKind::MaximumLeafSpecificConductanceMillimolesPerSquareMetrePerSecondPerMegapascal:
	case MeasurementKind::HydraulicCapacitanceKilogramsPerMegapascal:
	case MeasurementKind::XylemWaterPotentialAtFiftyPercentConductivityLossMegapascals:
	case MeasurementKind::StomatalWaterPotentialAtFiftyPercentConductanceLossMegapascals:
	case MeasurementKind::LeafToSapwoodAreaRatio:
	case MeasurementKind::ReferenceHeightMetres:
	case MeasurementKind::ReferenceHorizontalRadiusMetres:
	case MeasurementKind::ReferenceSupportingAxisDiameterMetres:
	case MeasurementKind::MinimumRootableVolumeCubicMetres:
	case MeasurementKind::MinimumRootableDepthMetres:
	case MeasurementKind::MaximumBulkDensityKilogramsPerCubicMetre:
	case MeasurementKind::MinimumAirFilledPorosityFraction:
	case MeasurementKind::MinimumAvailableWaterCapacityFraction:
		return ValueKind::Decimal;
	}
	return ValueKind::Text;
}

const char *expected_unit(MeasurementKind kind)
{
	switch (kind) {
	case MeasurementKind::ScientificName:
	case MeasurementKind::CultivarName:
		return "text";
	case MeasurementKind::SpecimenIdentifier:
	case MeasurementKind::ShootTopologyArtifactIdentifier:
	case MeasurementKind::RootGraphIdentifier:
	case MeasurementKind::SeasonalScheduleArtifactIdentifier:
	case MeasurementKind::EnvironmentalResponseArtifactIdentifier:
	case MeasurementKind::OntologyIdentifier:
	case MeasurementKind::OntologyReleaseIdentifier:
	case MeasurementKind::SizeRelationshipModelIdentifier:
	case MeasurementKind::GrammarEvidenceIdentifier:
	case MeasurementKind::CameraEvidenceIdentifier:
		return "identifier";
	case MeasurementKind::AnatomicalEntityIdentifiers:
	case MeasurementKind::DevelopmentStageIdentifiers:
		return "identifier-list";
	case MeasurementKind::MaximumBranchOrder:
	case MeasurementKind::MaximumRootOrder:
		return "count";
	case MeasurementKind::MaximumRootDepthMetres:
	case MeasurementKind::MaximumRootRadialSpreadMetres:
	case MeasurementKind::MaximumMatureHeightMetres:
	case MeasurementKind::ReferenceHeightMetres:
	case MeasurementKind::ReferenceHorizontalRadiusMetres:
	case MeasurementKind::ReferenceSupportingAxisDiameterMetres:
	case MeasurementKind::MinimumRootableDepthMetres:
		return "m";
	case MeasurementKind::MinimumRootableVolumeCubicMetres:
		return "m3";
	case MeasurementKind::LeafAreaIndex:
	case MeasurementKind::DampingRatio:
	case MeasurementKind::DragCoefficient:
	case MeasurementKind::LeafToSapwoodAreaRatio:
		return "dimensionless";
	case MeasurementKind::CrownGapFraction:
	case MeasurementKind::LeafDryMatterContentKilogramsPerKilogram:
	case MeasurementKind::LeafNitrogenContentKilogramsPerKilogram:
	case MeasurementKind::MinimumAirFilledPorosityFraction:
	case MeasurementKind::MinimumAvailableWaterCapacityFraction:
		return "fraction";
	case MeasurementKind::MeanLeafInclinationDegrees:
		return "degree";
	case MeasurementKind::MassDensityKilogramsPerCubicMetre:
	case MeasurementKind::MaximumBulkDensityKilogramsPerCubicMetre:
		return "kg/m3";
	case MeasurementKind::ElasticModulusPascals:
		return "Pa";
	case MeasurementKind::SpecificLeafAreaSquareMetresPerKilogram:
		return "m2/kg";
	case MeasurementKind::MaximumLeafSpecificConductanceMillimolesPerSquareMetrePerSecondPerMegapascal:
		return "mmol/m2/s/MPa";
	case MeasurementKind::HydraulicCapacitanceKilogramsPerMegapascal:
		return "kg/MPa";
	case MeasurementKind::XylemWaterPotentialAtFiftyPercentConductivityLossMegapascals:
	case MeasurementKind::StomatalWaterPotentialAtFiftyPercentConductanceLossMegapascals:
		return "MPa";
	}
	return "unsupported";
}

bool is_sha256(const std::string &value)
{
	return value.size() == 64u && std::all_of(
		value.begin(), value.end(), [](unsigned char character) {
			return std::isxdigit(character) != 0;
		});
}

bool contains_duplicates(const std::vector<std::string> &values)
{
	return std::set<std::string>(values.begin(), values.end()).size() !=
	       values.size();
}

bool valid_decimal_range(
	MeasurementKind kind,
	double value)
{
	if (!std::isfinite(value)) return false;
	switch (kind) {
	case MeasurementKind::CrownGapFraction:
	case MeasurementKind::LeafDryMatterContentKilogramsPerKilogram:
	case MeasurementKind::LeafNitrogenContentKilogramsPerKilogram:
	case MeasurementKind::MinimumAirFilledPorosityFraction:
	case MeasurementKind::MinimumAvailableWaterCapacityFraction:
		return value > 0.0 && value <= 1.0;
	case MeasurementKind::MeanLeafInclinationDegrees:
		return value >= 0.0 && value <= 90.0;
	case MeasurementKind::DampingRatio:
		return value >= 0.0;
	case MeasurementKind::XylemWaterPotentialAtFiftyPercentConductivityLossMegapascals:
	case MeasurementKind::StomatalWaterPotentialAtFiftyPercentConductanceLossMegapascals:
		return value < 0.0;
	default:
		return value > 0.0;
	}
}

const VegetationCalibrationMeasurement *measurement_for(
	const VegetationCalibrationEvidenceBundle &bundle,
	MeasurementKind kind)
{
	const auto found = std::find_if(
		bundle.measurements().begin(), bundle.measurements().end(),
		[kind](const VegetationCalibrationMeasurement &measurement) {
			return measurement.kind() == kind;
		});
	return found == bundle.measurements().end() ? nullptr : &*found;
}

std::string measurement_text(
	const VegetationCalibrationEvidenceBundle &bundle,
	MeasurementKind kind)
{
	const VegetationCalibrationMeasurement *measurement =
		measurement_for(bundle, kind);
	if (measurement == nullptr || measurement->value().textValue() == nullptr) {
		return std::string();
	}
	return *measurement->value().textValue();
}

void validate_subject_scope(
	const BuildingVegetationObjectModel &object_model,
	const VegetationCalibrationEvidenceBundle &bundle,
	VegetationCalibrationEvidenceBundleValidationReport &report)
{
	const VegetationCalibrationSubjectScope &scope = bundle.subjectScope();
	if (scope.subjectIdentifier().empty()) {
		report.addIssue(
			ValidationCode::InvalidSubjectScope,
			"Calibration evidence subject scope requires an identifier.");
	}
	if (scope.architecture() != object_model.plantArchitecture()) {
		report.addIssue(
			ValidationCode::ArchitectureMismatch,
			"Calibration evidence architecture differs from the target vegetation object.");
	}
	switch (scope.kind()) {
	case VegetationCalibrationSubjectScopeKind::ArchitecturalArchetype:
		if (!scope.scientificName().empty() || !scope.cultivarName().empty() ||
		    !scope.specimenIdentifier().empty()) {
			report.addIssue(
				ValidationCode::InvalidSubjectScope,
				"Architectural-archetype scope may not claim taxon, cultivar, or specimen identity.");
		}
		break;
	case VegetationCalibrationSubjectScopeKind::BotanicalTaxon:
		if (scope.scientificName().empty() || !scope.cultivarName().empty() ||
		    !scope.specimenIdentifier().empty()) {
			report.addIssue(
				ValidationCode::InvalidSubjectScope,
				"Botanical-taxon scope requires only a scientific name.");
		}
		break;
	case VegetationCalibrationSubjectScopeKind::Cultivar:
		if (scope.scientificName().empty() || scope.cultivarName().empty() ||
		    !scope.specimenIdentifier().empty()) {
			report.addIssue(
				ValidationCode::InvalidSubjectScope,
				"Cultivar scope requires scientific and cultivar names without a specimen identifier.");
		}
		break;
	case VegetationCalibrationSubjectScopeKind::MeasuredSpecimen:
		if (scope.scientificName().empty() ||
		    scope.specimenIdentifier().empty()) {
			report.addIssue(
				ValidationCode::InvalidSubjectScope,
				"Measured-specimen scope requires scientific and specimen identifiers.");
		}
		break;
	}

	const PlantIdentityProfile &identity =
		object_model.biologicalProfile().identity();
	if ((!identity.scientificName().empty() &&
	     identity.scientificName() != scope.scientificName()) ||
	    (!identity.cultivarName().empty() &&
	     identity.cultivarName() != scope.cultivarName()) ||
	    (!identity.specimenIdentifier().empty() &&
	     identity.specimenIdentifier() != scope.specimenIdentifier())) {
		report.addIssue(
			ValidationCode::ContradictoryIdentity,
			"Calibration evidence subject identity contradicts the target profile.");
	}

	for (const auto &identity_measurement : {
	     std::pair<MeasurementKind, std::string>(
		     MeasurementKind::ScientificName, scope.scientificName()),
	     std::pair<MeasurementKind, std::string>(
		     MeasurementKind::CultivarName, scope.cultivarName()),
	     std::pair<MeasurementKind, std::string>(
		     MeasurementKind::SpecimenIdentifier,
		     scope.specimenIdentifier())}) {
		const std::string value =
			measurement_text(bundle, identity_measurement.first);
		if (!value.empty() && value != identity_measurement.second) {
			report.addIssue(
				ValidationCode::ContradictoryIdentityMeasurement,
				"Identity measurement contradicts the bundle subject scope.");
		}
	}
}

}

VegetationCalibrationEvidenceBundleValidationReport
VegetationCalibrationEvidenceBundleValidationService::validate(
	const BuildingVegetationObjectModel &object_model,
	const VegetationCalibrationEvidenceBundle &bundle) const
{
	VegetationCalibrationEvidenceBundleValidationReport report;
	if (bundle.schemaVersion() !=
	    "ProGen3D-VegetationCalibrationEvidenceBundle-v1") {
		report.addIssue(
			ValidationCode::UnsupportedSchemaVersion,
			"Unsupported vegetation calibration evidence schema version.");
	}
	if (bundle.bundleIdentifier().empty()) {
		report.addIssue(
			ValidationCode::EmptyBundleIdentifier,
			"Calibration evidence bundle requires an identifier.");
	}
	validate_subject_scope(object_model, bundle, report);
	if (bundle.coordinateSystem().empty()) {
		report.addIssue(
			ValidationCode::EmptyCoordinateSystem,
			"Calibration evidence bundle requires a coordinate system.");
	}
	if (bundle.unitSystem() != "SI") {
		report.addIssue(
			ValidationCode::UnsupportedUnitSystem,
			"Calibration evidence bundle unit system must be SI.");
	}
	if (bundle.acquisitionMethod().empty()) {
		report.addIssue(
			ValidationCode::EmptyAcquisitionMethod,
			"Calibration evidence bundle requires an acquisition method.");
	}
	if (bundle.uncertaintyStatement().empty()) {
		report.addIssue(
			ValidationCode::EmptyUncertaintyStatement,
			"Calibration evidence bundle requires an uncertainty statement.");
	}

	std::set<std::string> source_identifiers;
	if (bundle.sourceReferences().empty()) {
		report.addIssue(
			ValidationCode::MissingSourceReference,
			"Calibration evidence bundle requires at least one source reference.");
	}
	for (const VegetationCalibrationSourceReference &source :
	     bundle.sourceReferences()) {
		if (source.sourceIdentifier().empty() || source.citation().empty() ||
		    source.locator().empty()) {
			report.addIssue(
				ValidationCode::MissingSourceReference,
				"Calibration source reference requires identifier, citation, and locator.");
		}
		if (!source_identifiers.insert(source.sourceIdentifier()).second) {
			report.addIssue(
				ValidationCode::DuplicateSourceReference,
				"Calibration source identifier appears more than once.");
		}
		if (source.sha256().empty()) {
			report.addIssue(
				ValidationCode::MissingEvidenceHash,
				"Calibration source reference requires a SHA-256 hash.");
		} else if (!is_sha256(source.sha256())) {
			report.addIssue(
				ValidationCode::InvalidEvidenceHash,
				"Calibration source reference SHA-256 is malformed.");
		}
	}

	if (bundle.payloadSha256().empty()) {
		report.addIssue(
			ValidationCode::MissingEvidenceHash,
			"Calibration evidence bundle requires a payload SHA-256.");
	} else if (!is_sha256(bundle.payloadSha256())) {
		report.addIssue(
			ValidationCode::InvalidEvidenceHash,
			"Calibration evidence payload SHA-256 is malformed.");
	} else if (VegetationCalibrationPayloadHashService()
		           .calculateCanonicalPayloadSha256(bundle) !=
	           bundle.payloadSha256()) {
		report.addIssue(
			ValidationCode::PayloadHashMismatch,
			"Calibration evidence payload SHA-256 does not match canonical content.");
	}

	std::set<MeasurementKind> measurement_kinds;
	for (const VegetationCalibrationMeasurement &measurement :
	     bundle.measurements()) {
		const std::optional<VegetationCalibrationDomain> declared_domain =
			vegetationCalibrationDomainFromName(measurement.declaredDomainName());
		if (!declared_domain.has_value()) {
			report.addIssue(
				ValidationCode::UnknownMeasurementDomain,
				"Calibration measurement declares an unknown domain.");
		} else if (*declared_domain !=
		           vegetationCalibrationMeasurementDomain(measurement.kind())) {
			report.addIssue(
				ValidationCode::MeasurementDomainMismatch,
				"Calibration measurement kind does not belong to its declared domain.");
		}
		if (!measurement_kinds.insert(measurement.kind()).second) {
			report.addIssue(
				ValidationCode::DuplicateMeasurement,
				"Calibration measurement kind appears more than once.");
		}
		if (measurement.unit() != expected_unit(measurement.kind())) {
			report.addIssue(
				ValidationCode::UnsupportedMeasurementUnit,
				"Calibration measurement uses an unsupported unit.");
		}
		if (measurement.value().kind() != expected_value_kind(measurement.kind())) {
			report.addIssue(
				ValidationCode::MeasurementValueKindMismatch,
				"Calibration measurement value kind does not match its measurement kind.");
		} else if (measurement.value().kind() == ValueKind::Decimal) {
			const double value = measurement.value().decimalValue().value();
			if (!std::isfinite(value)) {
				report.addIssue(
					ValidationCode::NonFiniteMeasurementValue,
					"Calibration measurement contains a non-finite decimal value.");
			} else if (!valid_decimal_range(measurement.kind(), value)) {
				report.addIssue(
					ValidationCode::InvalidMeasurementValueRange,
					"Calibration measurement decimal value is outside its valid range.");
			}
		} else if (measurement.value().kind() == ValueKind::Count &&
		           measurement.value().countValue().value() == 0u) {
			report.addIssue(
				ValidationCode::InvalidMeasurementValueRange,
				"Calibration measurement count must be positive.");
		} else if (measurement.value().kind() == ValueKind::Text &&
		           measurement.value().textValue()->empty()) {
			report.addIssue(
				ValidationCode::InvalidMeasurementValueRange,
				"Calibration measurement text value must not be empty.");
		} else if (measurement.value().kind() == ValueKind::IdentifierList &&
		           (measurement.value().identifierListValue()->empty() ||
		            contains_duplicates(
			            *measurement.value().identifierListValue()))) {
			report.addIssue(
				ValidationCode::InvalidMeasurementValueRange,
				"Calibration identifier-list measurement must be non-empty and unique.");
		}
		if (measurement.observationScope().empty()) {
			report.addIssue(
				ValidationCode::EmptyObservationScope,
				"Calibration measurement requires an observation scope.");
		}
		if (measurement.uncertaintyStatement().empty()) {
			report.addIssue(
				ValidationCode::EmptyMeasurementUncertainty,
				"Calibration measurement requires an uncertainty statement.");
		}
		if (measurement.evidenceSourceIdentifiers().empty()) {
			report.addIssue(
				ValidationCode::MissingMeasurementEvidenceReference,
				"Calibration measurement requires at least one evidence source.");
		}
		for (const std::string &source_identifier :
		     measurement.evidenceSourceIdentifiers()) {
			if (source_identifiers.find(source_identifier) ==
			    source_identifiers.end()) {
				report.addIssue(
					ValidationCode::UnknownMeasurementEvidenceReference,
					"Calibration measurement references an unknown evidence source.");
			}
		}
	}
	return report;
}
