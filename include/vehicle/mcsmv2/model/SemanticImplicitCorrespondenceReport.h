#pragma once

#include "vehicle/mcsmv2/model/SurfaceCorrespondenceDirectionReport.h"

#include <string>
#include <utility>
#include <vector>

class SemanticImplicitRegionCorrespondenceReport
{
public:
	SemanticImplicitRegionCorrespondenceReport(
		std::string region_identifier,
		SurfaceCorrespondenceDirectionReport semantic_to_scaffold,
		SurfaceCorrespondenceDirectionReport scaffold_to_semantic)
		: region_identifier_(std::move(region_identifier)),
		  semantic_to_scaffold_(std::move(semantic_to_scaffold)),
		  scaffold_to_semantic_(std::move(scaffold_to_semantic))
	{
	}

	const std::string &regionIdentifier() const { return region_identifier_; }
	const SurfaceCorrespondenceDirectionReport &semanticToScaffold() const
	{
		return semantic_to_scaffold_;
	}
	const SurfaceCorrespondenceDirectionReport &scaffoldToSemantic() const
	{
		return scaffold_to_semantic_;
	}

private:
	std::string region_identifier_;
	SurfaceCorrespondenceDirectionReport semantic_to_scaffold_{0u, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, ""};
	SurfaceCorrespondenceDirectionReport scaffold_to_semantic_{0u, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, ""};
};

class SemanticImplicitCorrespondenceReport
{
public:
	SemanticImplicitCorrespondenceReport(
		SurfaceCorrespondenceDirectionReport semantic_to_scaffold,
		SurfaceCorrespondenceDirectionReport scaffold_to_semantic,
		double percentile95_distance_tolerance,
		double maximum_distance_tolerance,
		double percentile95_normal_angle_tolerance_degrees,
		std::vector<SemanticImplicitRegionCorrespondenceReport> region_reports,
		bool passed)
		: semantic_to_scaffold_(std::move(semantic_to_scaffold)),
		  scaffold_to_semantic_(std::move(scaffold_to_semantic)),
		  percentile95_distance_tolerance_(percentile95_distance_tolerance),
		  maximum_distance_tolerance_(maximum_distance_tolerance),
		  percentile95_normal_angle_tolerance_degrees_(
			  percentile95_normal_angle_tolerance_degrees),
		  region_reports_(std::move(region_reports)),
		  passed_(passed)
	{
	}

	const SurfaceCorrespondenceDirectionReport &semanticToScaffold() const
	{
		return semantic_to_scaffold_;
	}
	const SurfaceCorrespondenceDirectionReport &scaffoldToSemantic() const
	{
		return scaffold_to_semantic_;
	}
	double percentile95DistanceTolerance() const
	{
		return percentile95_distance_tolerance_;
	}
	double maximumDistanceTolerance() const { return maximum_distance_tolerance_; }
	double percentile95NormalAngleToleranceDegrees() const
	{
		return percentile95_normal_angle_tolerance_degrees_;
	}
	const std::vector<SemanticImplicitRegionCorrespondenceReport> &regionReports() const
	{
		return region_reports_;
	}
	bool passed() const { return passed_; }

private:
	SurfaceCorrespondenceDirectionReport semantic_to_scaffold_{0u, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, ""};
	SurfaceCorrespondenceDirectionReport scaffold_to_semantic_{0u, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, ""};
	double percentile95_distance_tolerance_ = 0.0;
	double maximum_distance_tolerance_ = 0.0;
	double percentile95_normal_angle_tolerance_degrees_ = 0.0;
	std::vector<SemanticImplicitRegionCorrespondenceReport> region_reports_;
	bool passed_ = false;
};
