#pragma once

#include "vehicle/mcsmv2/model/VehicleSurfaceDomainLoop.h"

#include <string>
#include <utility>
#include <vector>

enum class VehicleSurfaceDomainKind
{
	Panel,
	Aperture
};

class VehicleSurfaceDomain
{
public:
	VehicleSurfaceDomain(
		std::string identifier,
		VehicleSurfaceDomainKind kind,
		bool closure,
		std::string parent_panel_identifier,
		std::vector<VehicleSurfaceDomainLoop> loops)
		: identifier_(std::move(identifier)),
		  kind_(kind),
		  closure_(closure),
		  parent_panel_identifier_(std::move(parent_panel_identifier)),
		  loops_(std::move(loops))
	{
	}

	const std::string &identifier() const { return identifier_; }
	VehicleSurfaceDomainKind kind() const { return kind_; }
	bool isClosure() const { return closure_; }
	const std::string &parentPanelIdentifier() const
	{
		return parent_panel_identifier_;
	}
	const std::vector<VehicleSurfaceDomainLoop> &loops() const { return loops_; }

private:
	std::string identifier_;
	VehicleSurfaceDomainKind kind_ = VehicleSurfaceDomainKind::Panel;
	bool closure_ = false;
	std::string parent_panel_identifier_;
	std::vector<VehicleSurfaceDomainLoop> loops_;
};
