#pragma once

#include "vegetation/model/BranchGraph.h"
#include "vegetation/model/PlantDevelopmentState.h"
#include "vegetation/model/PlantGrowthSpecification.h"

#include <utility>

class PlantGrowthRequest
{
public:
	PlantGrowthRequest(
		BranchGraph source_graph,
		float evaluation_time,
		PlantDevelopmentState development_state,
		PlantGrowthSpecification specification)
		: source_graph_(std::move(source_graph)),
		  evaluation_time_(evaluation_time),
		  development_state_(development_state),
		  specification_(std::move(specification))
	{
	}

	const BranchGraph &sourceGraph() const { return source_graph_; }
	float evaluationTime() const { return evaluation_time_; }
	PlantDevelopmentState developmentState() const
	{
		return development_state_;
	}
	const PlantGrowthSpecification &specification() const
	{
		return specification_;
	}

private:
	BranchGraph source_graph_;
	float evaluation_time_ = 0.0f;
	PlantDevelopmentState development_state_ = PlantDevelopmentState::Seed;
	PlantGrowthSpecification specification_{
		0.0f,
		1.0f,
		PlantGrowthChannelSpecification(0.05f, GrowthCurve::SmoothStep),
		PlantGrowthChannelSpecification(0.08f, GrowthCurve::SmoothStep),
		PlantGrowthChannelSpecification(0.02f, GrowthCurve::EaseOut)};
};
