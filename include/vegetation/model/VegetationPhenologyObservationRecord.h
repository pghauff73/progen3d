#pragma once

#include <string>
#include <utility>

class VegetationPhenologyObservationRecord
{
public:
	VegetationPhenologyObservationRecord(
		std::string observation_date,
		std::string development_stage_identifier)
		: observation_date_(std::move(observation_date)),
		  development_stage_identifier_(
			  std::move(development_stage_identifier))
	{
	}

	const std::string &observationDate() const { return observation_date_; }
	const std::string &developmentStageIdentifier() const
	{
		return development_stage_identifier_;
	}

private:
	std::string observation_date_;
	std::string development_stage_identifier_;
};
