#pragma once

#include "geometry/model/GeometryDetailLevel.h"
#include "vehicle/model/ModernVehicleAssembly.h"
#include "vehicle/parametric/model/ParametricModelGenerationPolicy.h"

#include <optional>
#include <string>

class ModernCarMvp26FamilyBuilder
{
public:
	std::optional<ModernVehicleAssembly> buildReference(
		GeometryDetailLevel detail_level,
		const ParametricModelGenerationPolicy &generation_policy,
		std::string *diagnostic = nullptr) const;
	std::optional<ModernVehicleAssembly> buildTrack(
		GeometryDetailLevel detail_level,
		const ParametricModelGenerationPolicy &generation_policy,
		std::string *diagnostic = nullptr) const;
	std::optional<ModernVehicleAssembly> buildAero(
		GeometryDetailLevel detail_level,
		const ParametricModelGenerationPolicy &generation_policy,
		std::string *diagnostic = nullptr) const;
	std::optional<ModernVehicleAssembly> buildCrossover(
		GeometryDetailLevel detail_level,
		const ParametricModelGenerationPolicy &generation_policy,
		std::string *diagnostic = nullptr) const;

private:
	std::optional<ModernVehicleAssembly> buildVariant(
		const std::string &variant_identifier,
		GeometryDetailLevel detail_level,
		const ParametricModelGenerationPolicy &generation_policy,
		std::string *diagnostic) const;
};
