#pragma once

#include "vegetation/model/CompoundLeafPlacement.h"
#include "vegetation/model/OrganAttachment.h"
#include "vegetation/model/PlantCompoundLeafSpecification.h"
#include "vegetation/model/PlantPetioleSpecification.h"

#include <optional>
#include <string>
#include <vector>

class CompoundLeafPlacementService
{
public:
	std::optional<CompoundLeafPlacement> place(
		const std::vector<OrganAttachment> &organ_attachments,
		const PlantPetioleSpecification &petiole,
		const PlantCompoundLeafSpecification &compound_leaf,
		std::string *diagnostic) const;
};

