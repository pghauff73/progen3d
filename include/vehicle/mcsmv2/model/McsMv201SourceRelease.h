#pragma once

#include <cstddef>
#include <string>
#include <utility>

class McsMv201SourceRelease
{
public:
	McsMv201SourceRelease(
		std::string version,
		std::string model_schema,
		std::string manifest_schema,
		std::string release_manifest_sha256,
		std::size_t signed_artifact_count)
		: version_(std::move(version)),
		  model_schema_(std::move(model_schema)),
		  manifest_schema_(std::move(manifest_schema)),
		  release_manifest_sha256_(std::move(release_manifest_sha256)),
		  signed_artifact_count_(signed_artifact_count)
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

private:
	std::string version_;
	std::string model_schema_;
	std::string manifest_schema_;
	std::string release_manifest_sha256_;
	std::size_t signed_artifact_count_ = 0u;
};
