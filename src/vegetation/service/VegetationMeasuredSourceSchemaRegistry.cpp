#include "vegetation/service/VegetationMeasuredSourceSchemaRegistry.h"

#include <string>
#include <utility>
#include <vector>

VegetationMeasuredSourceSchemaRegistry::VegetationMeasuredSourceSchemaRegistry()
	: registrations_({
		  VegetationMeasuredSourceSchemaRegistration(
			  "text/vnd.treeqsm.cylinder-table",
			  "TreeQSM-save_model_text-1.1.0",
			  "VegetationTreeQsmCylinderTableDecoder",
			  "application/vnd.progen3d.qsm+json",
			  "ProGen3D-QuantitativeStructureModel-v1"),
		  VegetationMeasuredSourceSchemaRegistration(
			  "application/rsml+xml",
			  "RSML-1",
			  "VegetationRootSystemMarkupLanguageDecoder",
			  "application/vnd.progen3d.root-architecture+json",
			  "ProGen3D-RootArchitectureGraph-v1"),
		  VegetationMeasuredSourceSchemaRegistration(
			  "text/vnd.progen3d.canopy-observation-table",
			  "ProGen3D-CanopyObservationTable-v1",
			  "VegetationCanopyObservationTableDecoder",
			  "application/vnd.progen3d.canopy-observation+json",
			  "ProGen3D-CanopyObservation-v1"),
		  VegetationMeasuredSourceSchemaRegistration(
			  "text/vnd.progen3d.phenology-series-table",
			  "ProGen3D-PhenologyObservationTable-v1",
			  "VegetationPhenologyObservationTableDecoder",
			  "application/vnd.progen3d.phenology-series+json",
			  "ProGen3D-PhenologyObservationSeries-v1"),
		  VegetationMeasuredSourceSchemaRegistration(
			  "text/vnd.progen3d.biomechanical-test-table",
			  "ProGen3D-BiomechanicalMaterialTestTable-v1",
			  "VegetationBiomechanicalMaterialTestTableDecoder",
			  "application/vnd.progen3d.biomechanical-test+json",
			  "ProGen3D-BiomechanicalMaterialTest-v1"),
	  })
{
}

VegetationMeasuredSourceSchemaRegistry::VegetationMeasuredSourceSchemaRegistry(
	std::vector<VegetationMeasuredSourceSchemaRegistration> registrations)
	: registrations_(std::move(registrations))
{
}

std::vector<VegetationMeasuredSourceSchemaRegistration>
VegetationMeasuredSourceSchemaRegistry::findRegistrations(
	const std::string &source_media_type,
	const std::string &source_schema_version) const
{
	std::vector<VegetationMeasuredSourceSchemaRegistration> matches;
	for (const VegetationMeasuredSourceSchemaRegistration &registration :
	     registrations_) {
		if (registration.sourceMediaType() == source_media_type &&
		    registration.sourceSchemaVersion() == source_schema_version) {
			matches.push_back(registration);
		}
	}
	return matches;
}

std::optional<VegetationMeasuredSourceSchemaRegistration>
VegetationMeasuredSourceSchemaRegistry::resolveUnique(
	const std::string &source_media_type,
	const std::string &source_schema_version) const
{
	const auto matches =
		findRegistrations(source_media_type, source_schema_version);
	if (matches.size() != 1u) return std::nullopt;
	return matches.front();
}
