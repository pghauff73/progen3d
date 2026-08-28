#pragma once

#include "chair/model/ChairObjectModel.h"

#include <vector>

class ChairCatalog
{
public:
	ChairCatalog();
	const std::vector<ChairDesignDefinition> &designs() const { return designs_; }
	const ChairDesignDefinition *findByIdentifier(const std::string &identifier) const;
	std::vector<const ChairDesignDefinition *> designsForContext(ChairUseContext context) const;
private:
	std::vector<ChairDesignDefinition> designs_;
};
