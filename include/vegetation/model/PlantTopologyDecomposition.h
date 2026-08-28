#pragma once

#include "vegetation/model/PlantInternode.h"
#include "vegetation/model/PlantStem.h"

#include <string>
#include <utility>
#include <vector>

class PlantTopologyDecomposition
{
public:
	static PlantTopologyDecomposition succeeded(
		std::vector<PlantInternode> internodes,
		std::vector<PlantStem> stems)
	{
		return PlantTopologyDecomposition(
			std::move(internodes), std::move(stems), std::string());
	}

	static PlantTopologyDecomposition failed(std::string diagnostic)
	{
		return PlantTopologyDecomposition({}, {}, std::move(diagnostic));
	}

	bool succeeded() const { return diagnostic_.empty(); }
	const std::string &diagnostic() const { return diagnostic_; }
	const std::vector<PlantInternode> &internodes() const { return internodes_; }
	const std::vector<PlantStem> &stems() const { return stems_; }

private:
	PlantTopologyDecomposition(
		std::vector<PlantInternode> internodes,
		std::vector<PlantStem> stems,
		std::string diagnostic)
		: internodes_(std::move(internodes)),
		  stems_(std::move(stems)),
		  diagnostic_(std::move(diagnostic))
	{
	}

	std::vector<PlantInternode> internodes_;
	std::vector<PlantStem> stems_;
	std::string diagnostic_;
};
