#pragma once

#include "vegetation/model/PlantDevelopmentState.h"

#include <string>
#include <utility>
#include <vector>

class PlantPhenologyProfile
{
public:
	PlantPhenologyProfile(
		PlantDevelopmentState reference_state,
		std::vector<PlantDevelopmentState> supported_states,
		std::vector<std::string> schedule_evidence_identifiers = {})
		: reference_state_(reference_state),
		  supported_states_(std::move(supported_states)),
		  schedule_evidence_identifiers_(
			  std::move(schedule_evidence_identifiers))
	{
	}

	static PlantPhenologyProfile uncalibratedMatureLeafOnProfile()
	{
		return PlantPhenologyProfile(
			PlantDevelopmentState::Mature,
			{
				PlantDevelopmentState::Juvenile,
				PlantDevelopmentState::Mature,
				PlantDevelopmentState::Flowering,
				PlantDevelopmentState::Fruiting,
				PlantDevelopmentState::Senescent,
				PlantDevelopmentState::Dormant,
			});
	}

	PlantDevelopmentState referenceState() const { return reference_state_; }
	const std::vector<PlantDevelopmentState> &supportedStates() const
	{
		return supported_states_;
	}
	const std::vector<std::string> &scheduleEvidenceIdentifiers() const
	{
		return schedule_evidence_identifiers_;
	}
	bool hasCalibratedSeasonalSchedule() const
	{
		return !schedule_evidence_identifiers_.empty();
	}

private:
	PlantDevelopmentState reference_state_ = PlantDevelopmentState::Mature;
	std::vector<PlantDevelopmentState> supported_states_;
	std::vector<std::string> schedule_evidence_identifiers_;
};
