#pragma once

#include <string>
#include <utility>

class VehiclePanelAdjacencyRelationship
{
public:
	VehiclePanelAdjacencyRelationship(
		std::string first_panel_identifier,
		std::string second_panel_identifier,
		std::string relationship)
		: first_panel_identifier_(std::move(first_panel_identifier)),
		  second_panel_identifier_(std::move(second_panel_identifier)),
		  relationship_(std::move(relationship))
	{
	}

	const std::string &firstPanelIdentifier() const
	{
		return first_panel_identifier_;
	}
	const std::string &secondPanelIdentifier() const
	{
		return second_panel_identifier_;
	}
	const std::string &relationship() const { return relationship_; }

private:
	std::string first_panel_identifier_;
	std::string second_panel_identifier_;
	std::string relationship_;
};
