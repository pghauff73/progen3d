#pragma once

#include "vegetation/model/VegetationTriangleRegionRole.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class VegetationTriangleAllocation
{
public:
	VegetationTriangleAllocation(
		std::string region_identifier,
		VegetationTriangleRegionRole role,
		std::size_t source_triangle_count,
		std::size_t allocated_triangle_count,
		std::size_t meshlet_count,
		double importance_score,
		double organ_linear_area_preservation_scale)
		: region_identifier_(std::move(region_identifier)),
		  role_(role),
		  source_triangle_count_(source_triangle_count),
		  allocated_triangle_count_(allocated_triangle_count),
		  meshlet_count_(meshlet_count),
		  importance_score_(importance_score),
		  organ_linear_area_preservation_scale_(
			  organ_linear_area_preservation_scale)
	{
	}

	const std::string &regionIdentifier() const { return region_identifier_; }
	VegetationTriangleRegionRole role() const { return role_; }
	std::size_t sourceTriangleCount() const { return source_triangle_count_; }
	std::size_t allocatedTriangleCount() const
	{
		return allocated_triangle_count_;
	}
	std::size_t meshletCount() const { return meshlet_count_; }
	double importanceScore() const { return importance_score_; }
	double organLinearAreaPreservationScale() const
	{
		return organ_linear_area_preservation_scale_;
	}

private:
	std::string region_identifier_;
	VegetationTriangleRegionRole role_ =
		VegetationTriangleRegionRole::FoliageInterior;
	std::size_t source_triangle_count_ = 0u;
	std::size_t allocated_triangle_count_ = 0u;
	std::size_t meshlet_count_ = 0u;
	double importance_score_ = 0.0;
	double organ_linear_area_preservation_scale_ = 1.0;
};

class VegetationTriangleDistributionSnapshot
{
public:
	VegetationTriangleDistributionSnapshot(
		std::vector<VegetationTriangleAllocation> allocations,
		std::size_t source_triangle_count,
		std::size_t allocated_triangle_count,
		std::uint64_t evidence_hash)
		: allocations_(std::move(allocations)),
		  source_triangle_count_(source_triangle_count),
		  allocated_triangle_count_(allocated_triangle_count),
		  evidence_hash_(evidence_hash)
	{
	}

	const std::vector<VegetationTriangleAllocation> &allocations() const
	{
		return allocations_;
	}
	std::size_t sourceTriangleCount() const { return source_triangle_count_; }
	std::size_t allocatedTriangleCount() const
	{
		return allocated_triangle_count_;
	}
	std::uint64_t evidenceHash() const { return evidence_hash_; }

private:
	std::vector<VegetationTriangleAllocation> allocations_;
	std::size_t source_triangle_count_ = 0u;
	std::size_t allocated_triangle_count_ = 0u;
	std::uint64_t evidence_hash_ = 0u;
};

class VegetationTriangleDistributionResult
{
public:
	static VegetationTriangleDistributionResult succeeded(
		VegetationTriangleDistributionSnapshot snapshot)
	{
		return VegetationTriangleDistributionResult(std::move(snapshot), {});
	}

	static VegetationTriangleDistributionResult failed(std::string diagnostic)
	{
		return VegetationTriangleDistributionResult(
			std::nullopt, std::move(diagnostic));
	}

	bool succeeded() const { return snapshot_.has_value(); }
	const std::optional<VegetationTriangleDistributionSnapshot> &snapshot() const
	{
		return snapshot_;
	}
	const std::string &diagnostic() const { return diagnostic_; }

private:
	VegetationTriangleDistributionResult(
		std::optional<VegetationTriangleDistributionSnapshot> snapshot,
		std::string diagnostic)
		: snapshot_(std::move(snapshot)), diagnostic_(std::move(diagnostic))
	{
	}

	std::optional<VegetationTriangleDistributionSnapshot> snapshot_;
	std::string diagnostic_;
};
