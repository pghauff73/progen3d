#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class PlantRootRepresentationKind
{
	SafetyEnvelope,
	ExplicitRootGraph
};

class PlantRootArchitectureProfile
{
public:
	PlantRootArchitectureProfile(
		PlantRootRepresentationKind representation,
		std::optional<double> maximum_depth_metres,
		std::optional<double> maximum_radial_spread_metres,
		std::optional<std::size_t> maximum_root_order,
		std::vector<std::string> evidence_identifiers = {},
		std::string root_graph_identifier = std::string())
		: representation_(representation),
		  maximum_depth_metres_(maximum_depth_metres),
		  maximum_radial_spread_metres_(maximum_radial_spread_metres),
		  maximum_root_order_(maximum_root_order),
		  evidence_identifiers_(std::move(evidence_identifiers)),
		  root_graph_identifier_(std::move(root_graph_identifier))
	{
	}

	static PlantRootArchitectureProfile uncalibratedSafetyEnvelope()
	{
		return PlantRootArchitectureProfile(
			PlantRootRepresentationKind::SafetyEnvelope,
			std::nullopt,
			std::nullopt,
			std::nullopt);
	}

	PlantRootRepresentationKind representation() const { return representation_; }
	std::optional<double> maximumDepthMetres() const
	{
		return maximum_depth_metres_;
	}
	std::optional<double> maximumRadialSpreadMetres() const
	{
		return maximum_radial_spread_metres_;
	}
	std::optional<std::size_t> maximumRootOrder() const
	{
		return maximum_root_order_;
	}
	const std::vector<std::string> &evidenceIdentifiers() const
	{
		return evidence_identifiers_;
	}
	const std::string &rootGraphIdentifier() const
	{
		return root_graph_identifier_;
	}
	bool hasCalibratedRootArchitecture() const
	{
		return representation_ == PlantRootRepresentationKind::ExplicitRootGraph &&
		       maximum_depth_metres_.has_value() &&
		       maximum_depth_metres_.value() > 0.0 &&
		       maximum_radial_spread_metres_.has_value() &&
		       maximum_radial_spread_metres_.value() > 0.0 &&
		       maximum_root_order_.has_value() &&
		       maximum_root_order_.value() > 0u &&
		       !root_graph_identifier_.empty() &&
		       !evidence_identifiers_.empty();
	}

private:
	PlantRootRepresentationKind representation_ =
		PlantRootRepresentationKind::SafetyEnvelope;
	std::optional<double> maximum_depth_metres_;
	std::optional<double> maximum_radial_spread_metres_;
	std::optional<std::size_t> maximum_root_order_;
	std::vector<std::string> evidence_identifiers_;
	std::string root_graph_identifier_;
};
