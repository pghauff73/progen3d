#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <string>
#include <utility>

class McsMv220SourceRelease
{
public:
	McsMv220SourceRelease(
		std::string version,
		std::string model_schema,
		std::string manifest_schema,
		std::string release_manifest_sha256,
		std::size_t signed_artifact_count,
		std::size_t signed_total_size_bytes,
		std::string reference_frame_schema,
		glm::dmat4 to_progen3d_matrix,
		std::string assurance_boundary)
		: version_(std::move(version)),
		  model_schema_(std::move(model_schema)),
		  manifest_schema_(std::move(manifest_schema)),
		  release_manifest_sha256_(std::move(release_manifest_sha256)),
		  signed_artifact_count_(signed_artifact_count),
		  signed_total_size_bytes_(signed_total_size_bytes),
		  reference_frame_schema_(std::move(reference_frame_schema)),
		  to_progen3d_matrix_(to_progen3d_matrix),
		  assurance_boundary_(std::move(assurance_boundary))
	{
	}

	const std::string &version() const { return version_; }
	const std::string &modelSchema() const { return model_schema_; }
	const std::string &manifestSchema() const { return manifest_schema_; }
	const std::string &releaseManifestSha256() const
	{
		return release_manifest_sha256_;
	}
	std::size_t signedArtifactCount() const { return signed_artifact_count_; }
	std::size_t signedTotalSizeBytes() const { return signed_total_size_bytes_; }
	const std::string &referenceFrameSchema() const
	{
		return reference_frame_schema_;
	}
	const glm::dmat4 &toProgen3dMatrix() const { return to_progen3d_matrix_; }
	const std::string &assuranceBoundary() const { return assurance_boundary_; }

private:
	std::string version_;
	std::string model_schema_;
	std::string manifest_schema_;
	std::string release_manifest_sha256_;
	std::size_t signed_artifact_count_ = 0u;
	std::size_t signed_total_size_bytes_ = 0u;
	std::string reference_frame_schema_;
	glm::dmat4 to_progen3d_matrix_{1.0};
	std::string assurance_boundary_;
};
