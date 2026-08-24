#pragma once

#include "vehicle/parametric/model/GeneratedVehicleRealization.h"
#include "vehicle/parametric/model/ModernCarFamilyDefinition.h"
#include "vehicle/parametric/model/ModernCarVariantDefinition.h"
#include "vehicle/parametric/model/ParametricModelGenerationPolicy.h"
#include "vehicle/parametric/model/ParametricModelSourceManifest.h"

#include <cstdint>

class ModernCarParametricDeterministicHashService
{
public:
	std::uint64_t calculateFamily(
		const ModernCarFamilyDefinition &family) const;
	std::uint64_t calculateVariant(
		const ModernCarVariantDefinition &variant) const;
	std::uint64_t calculateGenerationPolicy(
		const ParametricModelGenerationPolicy &policy) const;
	std::uint64_t calculateSourceManifest(
		const ParametricModelSourceManifest &source_manifest) const;
	std::uint64_t calculateBodyMesh(const GeneratedBodyMesh &body_mesh) const;
	std::uint64_t calculateRealization(
		const GeneratedVehicleRealization &realization) const;
};
