#pragma once

#include "vehicle/model/VehicleMvp26Architecture.h"
#include "vehicle/parametric/model/GeneratedVehicleRealization.h"
#include "vehicle/parametric/model/ModernCarVariantDefinition.h"
#include "vehicle/parametric/model/ParametricVehicleValidationReport.h"

#include <cstdint>
#include <optional>

class Mvp26ParametricVehicleIntegrationResult
{
public:
	Mvp26ParametricVehicleIntegrationResult(
		std::optional<VehicleMvp26Architecture> architecture,
		ParametricVehicleValidationReport validation_report)
		: architecture_(std::move(architecture)),
		  validation_report_(std::move(validation_report))
	{
	}

	const std::optional<VehicleMvp26Architecture> &architecture() const
	{
		return architecture_;
	}
	const ParametricVehicleValidationReport &validationReport() const
	{
		return validation_report_;
	}
	bool succeeded() const
	{
		return architecture_.has_value() && validation_report_.isValid();
	}

private:
	std::optional<VehicleMvp26Architecture> architecture_;
	ParametricVehicleValidationReport validation_report_;
};

class Mvp26ParametricVehicleIntegrationService
{
public:
	Mvp26ParametricVehicleIntegrationResult integrate(
		const VehicleMvp25Architecture &accepted_mvp25_architecture,
		std::uint64_t accepted_mvp25_hash,
		const ModernCarVariantDefinition &variant,
		const GeneratedVehicleRealization &realization) const;
};
