#pragma once

#include "geometry/model/ProceduralShapeCatalogEntry.h"

#include <string>
#include <vector>

class ProceduralShapeCatalogRepository
{
public:
	const std::vector<ProceduralShapeCatalogEntry> &entries() const;
	const ProceduralShapeCatalogEntry *find(const std::string &identifier) const;
	std::vector<std::string> completionNames() const;
	std::vector<std::string> optionSignatures(const std::string &identifier) const;
	std::vector<std::string> optionCompletions(const std::string &identifier) const;
	std::string tooltipFor(const std::string &identifier) const;
};
