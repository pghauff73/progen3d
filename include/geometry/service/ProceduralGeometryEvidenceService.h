#pragma once

#include "geometry/model/PrimitiveCollisionPolicy.h"
#include "geometry/model/PrimitiveVolumeEvidence.h"

class MeshTopologyReport;
class ShapeSpecification;

class ProceduralGeometryEvidenceService
{
public:
	PrimitiveVolumeEvidence createVolumeEvidence(
		const ShapeSpecification &specification,
		const MeshTopologyReport &topology) const;

	PrimitiveCollisionPolicy chooseCollisionPolicy(
		const ShapeSpecification &specification,
		bool watertight) const;
};
