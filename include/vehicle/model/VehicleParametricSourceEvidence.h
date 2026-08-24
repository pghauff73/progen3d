#pragma once

#include "vehicle/parametric/model/GeneratedVehicleRealization.h"

#include <cstdint>
#include <string>
#include <utility>

class VehicleParametricSourceEvidence
{
public:
	VehicleParametricSourceEvidence(
		std::string identifier,
		std::string coordinate_frame_identifier,
		std::uint64_t source_manifest_hash,
		std::uint64_t generation_policy_hash,
		std::uint64_t generated_geometry_hash,
		GeneratedObservationSet observations,
		GeneratedArtifactManifest artifact_manifest)
		: identifier_(std::move(identifier)),
		  coordinate_frame_identifier_(std::move(coordinate_frame_identifier)),
		  source_manifest_hash_(source_manifest_hash),
		  generation_policy_hash_(generation_policy_hash),
		  generated_geometry_hash_(generated_geometry_hash),
		  observations_(std::move(observations)),
		  artifact_manifest_(std::move(artifact_manifest))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &coordinateFrameIdentifier() const
	{
		return coordinate_frame_identifier_;
	}
	std::uint64_t sourceManifestHash() const { return source_manifest_hash_; }
	std::uint64_t generationPolicyHash() const { return generation_policy_hash_; }
	std::uint64_t generatedGeometryHash() const { return generated_geometry_hash_; }
	const GeneratedObservationSet &observations() const { return observations_; }
	const GeneratedArtifactManifest &artifactManifest() const
	{
		return artifact_manifest_;
	}

private:
	std::string identifier_;
	std::string coordinate_frame_identifier_;
	std::uint64_t source_manifest_hash_ = 0u;
	std::uint64_t generation_policy_hash_ = 0u;
	std::uint64_t generated_geometry_hash_ = 0u;
	GeneratedObservationSet observations_{{}};
	GeneratedArtifactManifest artifact_manifest_{{}};
};
