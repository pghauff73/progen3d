#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "vehicle/mcsmv2/model/McsMv22NativePartitionGeometry.h"

#include <string>
#include <utility>

class McsMv22NativeKinematicEvidenceGeometry
{
public:
	McsMv22NativeKinematicEvidenceGeometry(
		std::string variant_identifier,
		GeneratedPrimitiveMesh source_final_body_mesh,
		McsMv22NativePartitionGeometry progen3d_partition_geometry)
		: variant_identifier_(std::move(variant_identifier)),
		  source_final_body_mesh_(std::move(source_final_body_mesh)),
		  progen3d_partition_geometry_(std::move(progen3d_partition_geometry))
	{
	}

	const std::string &variantIdentifier() const { return variant_identifier_; }
	const GeneratedPrimitiveMesh &sourceFinalBodyMesh() const
	{
		return source_final_body_mesh_;
	}
	const McsMv22NativePartitionGeometry &progen3dPartitionGeometry() const
	{
		return progen3d_partition_geometry_;
	}

private:
	std::string variant_identifier_;
	GeneratedPrimitiveMesh source_final_body_mesh_{std::make_shared<Mesh>(), {}};
	McsMv22NativePartitionGeometry progen3d_partition_geometry_{
		"", GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {}), {}, {}};
};
