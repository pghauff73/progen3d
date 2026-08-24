#pragma once

#include "electrical/model/ElectricalControlGraph.h"
#include "lighting/model/PreviewLightCollection.h"

class LightingStateEvaluator
{
public:
	void apply(const ElectricalControlGraph &control_graph,
	           PreviewLightCollection *lights) const;
};
