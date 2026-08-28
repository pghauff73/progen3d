#pragma once

#include "vehicle/parametric/model/GeneratedVehicleRealization.h"
#include "vehicle/parametric/model/ModernCarParametricObjectModel.h"

class ModernCarParametricValidationService
{
public:
	ParametricVehicleValidationReport validateObjectModel(
		const ModernCarParametricObjectModel &object_model) const;
	ParametricVehicleValidationReport validateVariant(
		const ModernCarVariantDefinition &variant) const;
	ParametricVehicleValidationReport validateGenerationPolicy(
		const ParametricModelGenerationPolicy &policy) const;
	ParametricVehicleValidationReport validateRealization(
		const ModernCarVariantDefinition &variant,
		const GeneratedVehicleRealization &realization) const;
};
