#include "vegetation/model/VegetationCalibrationEvidenceBundle.h"

const char *vegetationCalibrationSubjectScopeKindName(
	VegetationCalibrationSubjectScopeKind kind)
{
	switch (kind) {
	case VegetationCalibrationSubjectScopeKind::ArchitecturalArchetype:
		return "ArchitecturalArchetype";
	case VegetationCalibrationSubjectScopeKind::BotanicalTaxon:
		return "BotanicalTaxon";
	case VegetationCalibrationSubjectScopeKind::Cultivar:
		return "Cultivar";
	case VegetationCalibrationSubjectScopeKind::MeasuredSpecimen:
		return "MeasuredSpecimen";
	}
	return "Unknown";
}

std::optional<VegetationCalibrationSubjectScopeKind>
vegetationCalibrationSubjectScopeKindFromName(const std::string &name)
{
	if (name == "ArchitecturalArchetype") {
		return VegetationCalibrationSubjectScopeKind::ArchitecturalArchetype;
	}
	if (name == "BotanicalTaxon") {
		return VegetationCalibrationSubjectScopeKind::BotanicalTaxon;
	}
	if (name == "Cultivar") {
		return VegetationCalibrationSubjectScopeKind::Cultivar;
	}
	if (name == "MeasuredSpecimen") {
		return VegetationCalibrationSubjectScopeKind::MeasuredSpecimen;
	}
	return std::nullopt;
}
