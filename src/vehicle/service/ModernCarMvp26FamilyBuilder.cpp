#include "vehicle/service/ModernCarMvp26FamilyBuilder.h"

#include "vehicle/parametric/service/Mvp26ParametricVehicleIntegrationService.h"
#include "vehicle/parametric/service/ModernCarBodySectionExtractionService.h"
#include "vehicle/parametric/service/ModernCarCharacterCurveExtractionService.h"
#include "vehicle/parametric/service/ModernCarIsoSurfaceGenerationService.h"
#include "vehicle/parametric/service/ModernCarObservationGenerationService.h"
#include "vehicle/parametric/service/ModernCarParametricCatalogFactory.h"
#include "vehicle/parametric/service/ModernCarParametricDeterministicHashService.h"
#include "vehicle/parametric/service/ModernCarParametricValidationService.h"
#include "vehicle/service/RedAwdHatchbackMvp25Builder.h"
#include "vehicle/service/VehicleMvp26DeterministicHashService.h"
#include "vehicle/service/VehicleMvp26ValidationService.h"

#include <memory>
#include <string>

namespace {

void set_first_parametric_diagnostic(
	const ParametricVehicleValidationReport &report,
	std::string *diagnostic)
{
	if (diagnostic == nullptr || report.issues().empty()) return;
	const ParametricVehicleValidationIssue &issue = report.issues().front();
	*diagnostic = std::string(parametricVehicleValidationCodeName(issue.code())) +
		": " + issue.message();
}

} // namespace

std::optional<ModernVehicleAssembly> ModernCarMvp26FamilyBuilder::buildReference(
	GeometryDetailLevel detail_level,
	const ParametricModelGenerationPolicy &generation_policy,
	std::string *diagnostic) const
{
	return buildVariant("reference", detail_level, generation_policy, diagnostic);
}

std::optional<ModernVehicleAssembly> ModernCarMvp26FamilyBuilder::buildTrack(
	GeometryDetailLevel detail_level,
	const ParametricModelGenerationPolicy &generation_policy,
	std::string *diagnostic) const
{
	return buildVariant("track", detail_level, generation_policy, diagnostic);
}

std::optional<ModernVehicleAssembly> ModernCarMvp26FamilyBuilder::buildAero(
	GeometryDetailLevel detail_level,
	const ParametricModelGenerationPolicy &generation_policy,
	std::string *diagnostic) const
{
	return buildVariant("aero", detail_level, generation_policy, diagnostic);
}

std::optional<ModernVehicleAssembly> ModernCarMvp26FamilyBuilder::buildCrossover(
	GeometryDetailLevel detail_level,
	const ParametricModelGenerationPolicy &generation_policy,
	std::string *diagnostic) const
{
	return buildVariant("crossover", detail_level, generation_policy, diagnostic);
}

std::optional<ModernVehicleAssembly> ModernCarMvp26FamilyBuilder::buildVariant(
	const std::string &variant_identifier,
	GeometryDetailLevel detail_level,
	const ParametricModelGenerationPolicy &generation_policy,
	std::string *diagnostic) const
{
	std::optional<ModernVehicleAssembly> accepted_vehicle =
		RedAwdHatchbackMvp25Builder().build(detail_level, diagnostic);
	if (!accepted_vehicle.has_value() || !accepted_vehicle->mvp25Architecture()) {
		return std::nullopt;
	}

	ModernCarParametricCatalogFactory catalog_factory;
	const ModernCarFamilyDefinition family = catalog_factory.createFamilyDefinition();
	const ModernCarVariantDefinition *variant = family.findVariant(variant_identifier);
	if (variant == nullptr) {
		if (diagnostic != nullptr) {
			*diagnostic = "InvalidParametricVariant: unknown MCP_OMv1 variant " +
				variant_identifier;
		}
		return std::nullopt;
	}
	ModernCarParametricValidationService parametric_validation_service;
	ParametricVehicleValidationReport parametric_report =
		parametric_validation_service.validateVariant(*variant);
	parametric_report.append(
		parametric_validation_service.validateGenerationPolicy(generation_policy));
	if (!parametric_report.isValid()) {
		set_first_parametric_diagnostic(parametric_report, diagnostic);
		return std::nullopt;
	}

	GeneratedBodyMeshResult body_result =
		ModernCarIsoSurfaceGenerationService().generate(*variant, generation_policy);
	if (!body_result.succeeded()) {
		set_first_parametric_diagnostic(body_result.validationReport(), diagnostic);
		return std::nullopt;
	}
	const GeneratedBodyMesh &body_mesh = *body_result.bodyMesh();
	GeneratedCharacterCurveSet curves =
		ModernCarCharacterCurveExtractionService().extract(*variant);
	GeneratedBodySectionSet sections =
		ModernCarBodySectionExtractionService().extract(*variant, body_mesh);
	GeneratedObservationSet observations =
		ModernCarObservationGenerationService().generate(*variant, body_mesh);
	ModernCarParametricDeterministicHashService hash_service;
	const std::uint64_t source_hash = hash_service.calculateSourceManifest(
		catalog_factory.createSourceManifest());
	const std::uint64_t generation_policy_hash =
		hash_service.calculateGenerationPolicy(generation_policy);
	const std::uint64_t geometry_hash = hash_service.calculateBodyMesh(body_mesh);
	GeneratedVehicleRealization realization(
		"MCP_OMv1." + variant_identifier + ".GeneratedVehicleRealization",
		variant_identifier, source_hash, generation_policy_hash, geometry_hash,
		body_mesh, std::move(curves), std::move(sections), std::move(observations),
		GeneratedArtifactManifest({}));
	parametric_report =
		parametric_validation_service.validateRealization(*variant, realization);
	if (!parametric_report.isValid()) {
		set_first_parametric_diagnostic(parametric_report, diagnostic);
		return std::nullopt;
	}

	Mvp26ParametricVehicleIntegrationResult integration =
		Mvp26ParametricVehicleIntegrationService().integrate(
			*accepted_vehicle->mvp25Architecture(),
			accepted_vehicle->mvp25ArchitectureHash(), *variant, realization);
	if (!integration.succeeded()) {
		set_first_parametric_diagnostic(integration.validationReport(), diagnostic);
		return std::nullopt;
	}
	std::shared_ptr<const VehicleMvp26Architecture> architecture =
		std::make_shared<const VehicleMvp26Architecture>(*integration.architecture());
	const VehicleValidationReport mvp26_validation =
		VehicleMvp26ValidationService().validate(*architecture);
	if (!mvp26_validation.isValid()) {
		if (diagnostic != nullptr) {
			*diagnostic = std::string(vehicleDiagnosticCodeName(
				mvp26_validation.issues().front().code())) + ": " +
				mvp26_validation.issues().front().message();
		}
		return std::nullopt;
	}
	const std::uint64_t mvp26_hash =
		VehicleMvp26DeterministicHashService().calculate(*architecture);
	return accepted_vehicle->withMvp26Architecture(
		std::move(architecture), mvp26_hash, mvp26_validation);
}
