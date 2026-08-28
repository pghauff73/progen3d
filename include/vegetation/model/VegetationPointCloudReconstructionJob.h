#pragma once

#include "vegetation/model/VegetationPointCloudReconstructionTarget.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

class VegetationPointCloudReconstructionJob
{
public:
	VegetationPointCloudReconstructionJob(
		std::string schema_version,
		std::string job_identifier,
		VegetationPointCloudReconstructionTarget target,
		std::string dataset_identifier,
		std::string source_payload_sha256,
		std::string algorithm_identifier,
		std::size_t maximum_output_primitives,
		std::uint64_t deterministic_seed)
		: schema_version_(std::move(schema_version)),
		  job_identifier_(std::move(job_identifier)),
		  target_(target),
		  dataset_identifier_(std::move(dataset_identifier)),
		  source_payload_sha256_(std::move(source_payload_sha256)),
		  algorithm_identifier_(std::move(algorithm_identifier)),
		  maximum_output_primitives_(maximum_output_primitives),
		  deterministic_seed_(deterministic_seed)
	{
	}

	const std::string &schemaVersion() const { return schema_version_; }
	const std::string &jobIdentifier() const { return job_identifier_; }
	VegetationPointCloudReconstructionTarget target() const { return target_; }
	const std::string &datasetIdentifier() const { return dataset_identifier_; }
	const std::string &sourcePayloadSha256() const
	{
		return source_payload_sha256_;
	}
	const std::string &algorithmIdentifier() const
	{
		return algorithm_identifier_;
	}
	std::size_t maximumOutputPrimitives() const
	{
		return maximum_output_primitives_;
	}
	std::uint64_t deterministicSeed() const { return deterministic_seed_; }

private:
	std::string schema_version_;
	std::string job_identifier_;
	VegetationPointCloudReconstructionTarget target_ =
		VegetationPointCloudReconstructionTarget::ShootArchitecture;
	std::string dataset_identifier_;
	std::string source_payload_sha256_;
	std::string algorithm_identifier_;
	std::size_t maximum_output_primitives_ = 0u;
	std::uint64_t deterministic_seed_ = 0u;
};
