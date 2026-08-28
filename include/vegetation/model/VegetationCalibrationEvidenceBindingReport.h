#pragma once

#include "vegetation/model/VegetationBiologicalProfile.h"
#include "vegetation/model/VegetationCalibrationDomain.h"
#include "vegetation/model/VegetationCalibrationEvidenceBundleValidationReport.h"
#include "vegetation/model/VegetationCalibrationMeasurement.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class VegetationCalibrationEvidenceBindingDisposition
{
	Accepted,
	Deferred,
	Rejected
};

class VegetationCalibrationEvidenceBindingObservation
{
public:
	VegetationCalibrationEvidenceBindingObservation(
		VegetationCalibrationMeasurementKind measurement_kind,
		VegetationCalibrationEvidenceBindingDisposition disposition,
		std::string explanation)
		: measurement_kind_(measurement_kind),
		  disposition_(disposition),
		  explanation_(std::move(explanation))
	{
	}

	VegetationCalibrationMeasurementKind measurementKind() const
	{
		return measurement_kind_;
	}
	VegetationCalibrationEvidenceBindingDisposition disposition() const
	{
		return disposition_;
	}
	const std::string &explanation() const { return explanation_; }

private:
	VegetationCalibrationMeasurementKind measurement_kind_ =
		VegetationCalibrationMeasurementKind::ScientificName;
	VegetationCalibrationEvidenceBindingDisposition disposition_ =
		VegetationCalibrationEvidenceBindingDisposition::Deferred;
	std::string explanation_;
};

class VegetationCalibrationEvidenceBindingReport
{
public:
	VegetationCalibrationEvidenceBindingReport(
		VegetationCalibrationEvidenceBundleValidationReport validation_report,
		std::vector<VegetationCalibrationEvidenceBindingObservation> observations,
		std::vector<VegetationCalibrationDomain> ready_domains_before_binding,
		std::vector<VegetationCalibrationDomain> ready_domains_after_binding,
		std::optional<VegetationBiologicalProfile> bound_profile)
		: validation_report_(std::move(validation_report)),
		  observations_(std::move(observations)),
		  ready_domains_before_binding_(
			  std::move(ready_domains_before_binding)),
		  ready_domains_after_binding_(
			  std::move(ready_domains_after_binding)),
		  bound_profile_(std::move(bound_profile))
	{
	}

	bool succeeded() const
	{
		return validation_report_.passed() && bound_profile_.has_value();
	}
	const VegetationCalibrationEvidenceBundleValidationReport &validationReport()
		const
	{
		return validation_report_;
	}
	const std::vector<VegetationCalibrationEvidenceBindingObservation> &
	observations() const
	{
		return observations_;
	}
	const std::vector<VegetationCalibrationDomain> &readyDomainsBeforeBinding()
		const
	{
		return ready_domains_before_binding_;
	}
	const std::vector<VegetationCalibrationDomain> &readyDomainsAfterBinding()
		const
	{
		return ready_domains_after_binding_;
	}
	const std::optional<VegetationBiologicalProfile> &boundProfile() const
	{
		return bound_profile_;
	}

private:
	VegetationCalibrationEvidenceBundleValidationReport validation_report_;
	std::vector<VegetationCalibrationEvidenceBindingObservation> observations_;
	std::vector<VegetationCalibrationDomain> ready_domains_before_binding_;
	std::vector<VegetationCalibrationDomain> ready_domains_after_binding_;
	std::optional<VegetationBiologicalProfile> bound_profile_;
};
