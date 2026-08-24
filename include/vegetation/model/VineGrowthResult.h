#pragma once

#include "vegetation/model/BranchGraph.h"
#include "vegetation/model/SurfaceAttachmentPoint.h"
#include "vegetation/model/VineGrowthResolutionEvidence.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

class VinePath
{
public:
	VinePath(
		BranchGraph graph,
		std::vector<SurfaceAttachmentPoint> attachment_points,
		VinePathState state)
		: graph_(std::move(graph)),
		  attachment_points_(std::move(attachment_points)),
		  state_(state)
	{
	}

	const BranchGraph &graph() const { return graph_; }
	const std::vector<SurfaceAttachmentPoint> &attachmentPoints() const
	{
		return attachment_points_;
	}
	VinePathState state() const { return state_; }

private:
	BranchGraph graph_;
	std::vector<SurfaceAttachmentPoint> attachment_points_;
	VinePathState state_ = VinePathState::Free;
};

class VineGrowthSnapshot
{
public:
	VineGrowthSnapshot(
		VinePath path,
		VineGrowthResolutionEvidence evidence)
		: path_(std::move(path)), evidence_(std::move(evidence))
	{
	}

	const VinePath &path() const { return path_; }
	const VineGrowthResolutionEvidence &evidence() const { return evidence_; }

private:
	VinePath path_;
	VineGrowthResolutionEvidence evidence_;
};

class VineGrowthResult
{
public:
	static VineGrowthResult succeeded(VineGrowthSnapshot snapshot)
	{
		return VineGrowthResult(std::move(snapshot), {});
	}

	static VineGrowthResult failed(std::string diagnostic)
	{
		return VineGrowthResult(std::nullopt, std::move(diagnostic));
	}

	bool succeeded() const { return snapshot_.has_value(); }
	const std::optional<VineGrowthSnapshot> &snapshot() const
	{
		return snapshot_;
	}
	const std::string &diagnostic() const { return diagnostic_; }

private:
	VineGrowthResult(
		std::optional<VineGrowthSnapshot> snapshot,
		std::string diagnostic)
		: snapshot_(std::move(snapshot)), diagnostic_(std::move(diagnostic))
	{
	}

	std::optional<VineGrowthSnapshot> snapshot_;
	std::string diagnostic_;
};
