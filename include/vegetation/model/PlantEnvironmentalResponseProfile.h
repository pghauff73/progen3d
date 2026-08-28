#pragma once

#include <string>
#include <utility>
#include <vector>

enum class PlantEnvironmentalDriver
{
	Light,
	Water,
	Temperature,
	Wind,
	Soil,
	AvailableSpace,
	HostSupport
};

class PlantEnvironmentalResponseProfile
{
public:
	PlantEnvironmentalResponseProfile(
		std::vector<PlantEnvironmentalDriver> required_drivers,
		std::vector<std::string> response_evidence_identifiers = {})
		: required_drivers_(std::move(required_drivers)),
		  response_evidence_identifiers_(
			  std::move(response_evidence_identifiers))
	{
	}

	static PlantEnvironmentalResponseProfile uncalibratedRequiredDrivers()
	{
		return PlantEnvironmentalResponseProfile({
			PlantEnvironmentalDriver::Light,
			PlantEnvironmentalDriver::Water,
			PlantEnvironmentalDriver::Temperature,
			PlantEnvironmentalDriver::Wind,
			PlantEnvironmentalDriver::Soil,
			PlantEnvironmentalDriver::AvailableSpace,
			PlantEnvironmentalDriver::HostSupport,
		});
	}

	const std::vector<PlantEnvironmentalDriver> &requiredDrivers() const
	{
		return required_drivers_;
	}
	const std::vector<std::string> &responseEvidenceIdentifiers() const
	{
		return response_evidence_identifiers_;
	}
	bool hasCalibratedResponses() const
	{
		return !response_evidence_identifiers_.empty();
	}

private:
	std::vector<PlantEnvironmentalDriver> required_drivers_;
	std::vector<std::string> response_evidence_identifiers_;
};
