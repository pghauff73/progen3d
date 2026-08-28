#include "vehicle/service/VehicleMvp26ValidationService.h"

#include "vehicle/service/VehicleMvp25DeterministicHashService.h"
#include "vehicle/service/VehicleMvp25ValidationService.h"

#include <cmath>

VehicleValidationReport VehicleMvp26ValidationService::validate(
	const VehicleMvp26Architecture &architecture) const
{
	VehicleValidationReport report;
	const VehicleValidationReport authority_report =
		VehicleMvp25ValidationService().validate(
			architecture.acceptedMvp25Architecture());
	report.append(authority_report);
	if (architecture.acceptedMvp25Hash() == 0u ||
	    VehicleMvp25DeterministicHashService().calculate(
		    architecture.acceptedMvp25Architecture()) !=
		    architecture.acceptedMvp25Hash()) {
		report.addIssue({
			VehicleDiagnosticCode::InvalidMvp26Architecture,
			"MVP2.6 must preserve the exact accepted MVP2.5 architecture hash.",
			{architecture.identifier(),
			 architecture.acceptedMvp25Architecture().identifier()}});
	}
	const VehiclePackageEvidence &package = architecture.variant().packageCandidate();
	if (architecture.variant().identifier().empty() ||
	    package.overallLength() <= 0.0f || package.overallWidth() <= 0.0f ||
	    package.overallHeight() <= 0.0f || package.wheelbase() <= 0.0f ||
	    std::fabs(package.overallLength() -
		(package.frontOverhang() + package.wheelbase() + package.rearOverhang())) >
		1.0e-4f) {
		report.addIssue({
			VehicleDiagnosticCode::InvalidParametricVariant,
			"MVP2.6 variant package is missing or violates its package equation.",
			{architecture.identifier(), architecture.variant().identifier()}});
	}
	const VehicleParametricShapePrior &shape_prior =
		architecture.parametricShapePrior();
	if (shape_prior.sourceVariantIdentifier() != architecture.variant().identifier() ||
	    shape_prior.characterCurves().curves().size() != 8u ||
	    shape_prior.bodySections().sections().empty() ||
	    shape_prior.styleTerms().empty()) {
		report.addIssue({
			VehicleDiagnosticCode::InvalidParametricVariant,
			"Parametric shape prior must retain curves, sections, style terms, and source variant identity.",
			{architecture.identifier(), shape_prior.identifier()}});
	}
	const VehicleParametricSourceEvidence &source = architecture.sourceEvidence();
	if (source.coordinateFrameIdentifier() != "MCP_OMv1.VehicleCoordinateFrame" ||
	    source.sourceManifestHash() == 0u || source.generationPolicyHash() == 0u ||
	    source.generatedGeometryHash() == 0u ||
	    source.observations().observations().size() != 5u) {
		report.addIssue({
			VehicleDiagnosticCode::InvalidParametricSourceEvidence,
			"Parametric source evidence must retain frame, source, policy, geometry, and five-view provenance.",
			{architecture.identifier(), source.identifier()}});
	}
	if (!architecture.fitReport().isValid()) {
		report.addIssue({
			VehicleDiagnosticCode::InvalidParametricFitReport,
			"MVP2.6 parametric fit contains blocking package or silhouette diagnostics.",
			{architecture.identifier(), architecture.fitReport().identifier()}});
	}
	return report;
}
