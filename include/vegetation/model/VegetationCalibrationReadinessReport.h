#pragma once

#include "vegetation/model/VegetationBiologicalProfileValidationReport.h"
#include "vegetation/model/VegetationCalibrationDomain.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

class VegetationCalibrationReadinessObservation
{
public:
	VegetationCalibrationReadinessObservation(
		VegetationCalibrationDomain domain,
		bool ready,
		std::vector<std::string> unmet_requirements)
		: domain_(domain),
		  ready_(ready),
		  unmet_requirements_(std::move(unmet_requirements))
	{
	}

	VegetationCalibrationDomain domain() const { return domain_; }
	bool ready() const { return ready_; }
	const std::vector<std::string> &unmetRequirements() const
	{
		return unmet_requirements_;
	}

private:
	VegetationCalibrationDomain domain_ =
		VegetationCalibrationDomain::TaxonomicIdentity;
	bool ready_ = false;
	std::vector<std::string> unmet_requirements_;
};

class VegetationCalibrationReadinessReport
{
public:
	VegetationCalibrationReadinessReport(
		VegetationBiologicalProfileValidationReport validation_report,
		std::vector<VegetationCalibrationReadinessObservation> observations)
		: validation_report_(std::move(validation_report)),
		  observations_(std::move(observations))
	{
	}

	const VegetationBiologicalProfileValidationReport &validationReport() const
	{
		return validation_report_;
	}
	const std::vector<VegetationCalibrationReadinessObservation> &observations() const
	{
		return observations_;
	}
	bool readyFor(VegetationCalibrationDomain domain) const
	{
		const auto found = std::find_if(
			observations_.begin(), observations_.end(),
			[domain](const VegetationCalibrationReadinessObservation &observation) {
				return observation.domain() == domain;
			});
		return found != observations_.end() && found->ready();
	}
	bool allRequestedDomainsReady() const
	{
		return validation_report_.passed() && !observations_.empty() &&
		       std::all_of(
			       observations_.begin(), observations_.end(),
			       [](const VegetationCalibrationReadinessObservation &observation) {
				       return observation.ready();
			       });
	}

private:
	VegetationBiologicalProfileValidationReport validation_report_;
	std::vector<VegetationCalibrationReadinessObservation> observations_;
};
