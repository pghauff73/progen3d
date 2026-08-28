#include "vegetation/service/VegetationTriangleDistributionService.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

struct WorkingTriangleRegion
{
	const VegetationTriangleRegion *region = nullptr;
	double importance_score = 0.0;
	std::size_t allocated_triangle_count = 0u;
};

bool finite_unit_interval(double value)
{
	return std::isfinite(value) && value >= 0.0 && value <= 1.0;
}

bool checked_add(std::size_t value, std::size_t addition, std::size_t *result)
{
	if (value > std::numeric_limits<std::size_t>::max() - addition) return false;
	*result = value + addition;
	return true;
}

double importance_score(
	const VegetationTriangleImportance &importance,
	const VegetationTriangleImportanceWeights &weights)
{
	const double weight_sum =
		weights.silhouetteWeight() +
		weights.projectedAreaWeight() +
		weights.curvatureWeight() +
		weights.topologyWeight() +
		weights.motionWeight() +
		weights.materialBoundaryWeight() +
		weights.visibilityWeight() +
		weights.biologicalAreaWeight();
	if (weight_sum <= 0.0) return 0.0;
	return (
		weights.silhouetteWeight() * importance.silhouetteContribution() +
		weights.projectedAreaWeight() * importance.projectedAreaContribution() +
		weights.curvatureWeight() * importance.curvatureContribution() +
		weights.topologyWeight() * importance.topologyContribution() +
		weights.motionWeight() * importance.motionContribution() +
		weights.materialBoundaryWeight() *
			importance.materialBoundaryContribution() +
		weights.visibilityWeight() * importance.visibilityContribution() +
		weights.biologicalAreaWeight() *
			importance.biologicalAreaContribution()) /
		weight_sum;
}

bool valid_importance(const VegetationTriangleImportance &importance)
{
	return finite_unit_interval(importance.silhouetteContribution()) &&
	       finite_unit_interval(importance.projectedAreaContribution()) &&
	       finite_unit_interval(importance.curvatureContribution()) &&
	       finite_unit_interval(importance.topologyContribution()) &&
	       finite_unit_interval(importance.motionContribution()) &&
	       finite_unit_interval(importance.materialBoundaryContribution()) &&
	       finite_unit_interval(importance.visibilityContribution()) &&
	       finite_unit_interval(importance.biologicalAreaContribution());
}

bool valid_weights(const VegetationTriangleImportanceWeights &weights)
{
	const double values[] = {
		weights.silhouetteWeight(), weights.projectedAreaWeight(),
		weights.curvatureWeight(), weights.topologyWeight(),
		weights.motionWeight(), weights.materialBoundaryWeight(),
		weights.visibilityWeight(), weights.biologicalAreaWeight()};
	double sum = 0.0;
	for (double value : values) {
		if (!std::isfinite(value) || value < 0.0) return false;
		sum += value;
	}
	return std::isfinite(sum) && sum > 0.0;
}

std::uint64_t fnv_offset_basis()
{
	return 1469598103934665603ull;
}

void hash_byte(std::uint64_t *hash, unsigned char value)
{
	*hash ^= static_cast<std::uint64_t>(value);
	*hash *= 1099511628211ull;
}

void hash_string(std::uint64_t *hash, const std::string &value)
{
	for (unsigned char character : value) hash_byte(hash, character);
	hash_byte(hash, 0xffu);
}

void hash_unsigned(std::uint64_t *hash, std::uint64_t value)
{
	for (unsigned int byte_index = 0u; byte_index < 8u; ++byte_index) {
		hash_byte(
			hash,
			static_cast<unsigned char>((value >> (byte_index * 8u)) & 0xffu));
	}
}

void hash_double(std::uint64_t *hash, double value)
{
	std::uint64_t bits = 0u;
	static_assert(sizeof(bits) == sizeof(value), "Unexpected double width");
	std::memcpy(&bits, &value, sizeof(bits));
	hash_unsigned(hash, bits);
}

std::uint64_t evidence_hash(
	const VegetationTriangleDistributionPolicy &policy,
	const std::vector<VegetationTriangleAllocation> &allocations)
{
	std::uint64_t hash = fnv_offset_basis();
	hash_unsigned(&hash, static_cast<std::uint64_t>(policy.targetTriangleCount()));
	hash_unsigned(
		&hash,
		static_cast<std::uint64_t>(policy.maximumMeshletTriangleCount()));
	hash_byte(&hash, policy.preservesOrganArea() ? 1u : 0u);
	for (const VegetationTriangleAllocation &allocation : allocations) {
		hash_string(&hash, allocation.regionIdentifier());
		hash_unsigned(
			&hash, static_cast<std::uint64_t>(allocation.role()));
		hash_unsigned(
			&hash,
			static_cast<std::uint64_t>(allocation.sourceTriangleCount()));
		hash_unsigned(
			&hash,
			static_cast<std::uint64_t>(allocation.allocatedTriangleCount()));
		hash_unsigned(
			&hash, static_cast<std::uint64_t>(allocation.meshletCount()));
		hash_double(&hash, allocation.importanceScore());
		hash_double(&hash, allocation.organLinearAreaPreservationScale());
	}
	return hash;
}

} // namespace

VegetationTriangleDistributionResult
VegetationTriangleDistributionService::distribute(
	const VegetationTriangleDistributionRequest &request) const
{
	const VegetationTriangleDistributionPolicy &policy = request.policy();
	if (policy.targetTriangleCount() == 0u) {
		return VegetationTriangleDistributionResult::failed(
			"The vegetation triangle target must be greater than zero.");
	}
	if (policy.maximumMeshletTriangleCount() == 0u) {
		return VegetationTriangleDistributionResult::failed(
			"The maximum meshlet triangle count must be greater than zero.");
	}
	if (!valid_weights(policy.importanceWeights())) {
		return VegetationTriangleDistributionResult::failed(
			"Vegetation triangle importance weights must be finite, non-negative, and non-zero in total.");
	}
	if (request.regions().empty()) {
		return VegetationTriangleDistributionResult::failed(
			"Vegetation triangle distribution requires at least one semantic region.");
	}

	std::unordered_set<std::string> region_identifiers;
	std::size_t source_triangle_count = 0u;
	std::size_t minimum_triangle_count = 0u;
	std::vector<WorkingTriangleRegion> working_regions;
	working_regions.reserve(request.regions().size());
	for (const VegetationTriangleRegion &region : request.regions()) {
		if (region.regionIdentifier().empty() ||
		    !region_identifiers.insert(region.regionIdentifier()).second) {
			return VegetationTriangleDistributionResult::failed(
				"Vegetation triangle region identifiers must be non-empty and unique.");
		}
		if (region.sourceTriangleCount() == 0u ||
		    region.minimumTriangleCount() > region.sourceTriangleCount()) {
			return VegetationTriangleDistributionResult::failed(
				"Each vegetation triangle region must have source geometry and a feasible minimum count.");
		}
		if (!std::isfinite(region.representedSurfaceArea()) ||
		    region.representedSurfaceArea() < 0.0 ||
		    !valid_importance(region.importance())) {
			return VegetationTriangleDistributionResult::failed(
				"Vegetation triangle region area and importance values are invalid.");
		}
		if (!checked_add(source_triangle_count, region.sourceTriangleCount(),
		                 &source_triangle_count) ||
		    !checked_add(minimum_triangle_count, region.minimumTriangleCount(),
		                 &minimum_triangle_count)) {
			return VegetationTriangleDistributionResult::failed(
				"Vegetation triangle counts exceed supported integer capacity.");
		}
		working_regions.push_back(WorkingTriangleRegion{
			&region,
			importance_score(region.importance(), policy.importanceWeights()),
			region.minimumTriangleCount()});
	}
	if (policy.targetTriangleCount() < minimum_triangle_count) {
		return VegetationTriangleDistributionResult::failed(
			"The vegetation triangle target is below the sum of semantic minimums.");
	}
	if (policy.targetTriangleCount() > source_triangle_count) {
		return VegetationTriangleDistributionResult::failed(
			"The vegetation triangle target exceeds available source geometry.");
	}

	std::size_t remaining_triangle_count =
		policy.targetTriangleCount() - minimum_triangle_count;
	while (remaining_triangle_count > 0u) {
		double active_score_sum = 0.0;
		std::size_t active_capacity_sum = 0u;
		for (const WorkingTriangleRegion &working : working_regions) {
			const std::size_t capacity =
				working.region->sourceTriangleCount() -
				working.allocated_triangle_count;
			if (capacity == 0u) continue;
			active_score_sum += working.importance_score;
			active_capacity_sum += capacity;
		}
		if (active_capacity_sum < remaining_triangle_count) {
			return VegetationTriangleDistributionResult::failed(
				"Vegetation triangle allocation exhausted semantic region capacity.");
		}

		struct FractionalAllocation
		{
			std::size_t region_index = 0u;
			double remainder = 0.0;
		};
		std::vector<FractionalAllocation> fractional_allocations;
		std::size_t applied_triangle_count = 0u;
		for (std::size_t region_index = 0u;
		     region_index < working_regions.size(); ++region_index) {
			WorkingTriangleRegion &working = working_regions[region_index];
			const std::size_t capacity =
				working.region->sourceTriangleCount() -
				working.allocated_triangle_count;
			if (capacity == 0u) continue;
			const double distribution_value =
				active_score_sum > 0.0
					? working.importance_score / active_score_sum
					: static_cast<double>(capacity) /
						  static_cast<double>(active_capacity_sum);
			const double exact_share =
				static_cast<double>(remaining_triangle_count) *
				distribution_value;
			const std::size_t floor_share = std::min(
				capacity,
				static_cast<std::size_t>(std::floor(exact_share)));
			working.allocated_triangle_count += floor_share;
			applied_triangle_count += floor_share;
			if (floor_share < capacity) {
				fractional_allocations.push_back(
					{region_index, exact_share - std::floor(exact_share)});
			}
		}
		remaining_triangle_count -= applied_triangle_count;
		if (remaining_triangle_count == 0u) break;

		std::sort(
			fractional_allocations.begin(), fractional_allocations.end(),
			[&](const FractionalAllocation &first,
			    const FractionalAllocation &second) {
				if (first.remainder != second.remainder) {
					return first.remainder > second.remainder;
				}
				return working_regions[first.region_index]
					       .region->regionIdentifier() <
				       working_regions[second.region_index]
					       .region->regionIdentifier();
			});
		std::size_t rounded_triangle_count = 0u;
		for (const FractionalAllocation &fractional : fractional_allocations) {
			if (remaining_triangle_count == 0u) break;
			WorkingTriangleRegion &working =
				working_regions[fractional.region_index];
			if (working.allocated_triangle_count >=
			    working.region->sourceTriangleCount()) {
				continue;
			}
			++working.allocated_triangle_count;
			--remaining_triangle_count;
			++rounded_triangle_count;
		}
		if (applied_triangle_count == 0u && rounded_triangle_count == 0u) {
			return VegetationTriangleDistributionResult::failed(
				"Vegetation triangle allocation made no deterministic progress.");
		}
	}

	std::sort(
		working_regions.begin(), working_regions.end(),
		[](const WorkingTriangleRegion &first,
		   const WorkingTriangleRegion &second) {
			return first.region->regionIdentifier() <
			       second.region->regionIdentifier();
		});
	std::vector<VegetationTriangleAllocation> allocations;
	allocations.reserve(working_regions.size());
	for (const WorkingTriangleRegion &working : working_regions) {
		const std::size_t meshlet_count =
			(working.allocated_triangle_count +
			 policy.maximumMeshletTriangleCount() - 1u) /
			policy.maximumMeshletTriangleCount();
		double area_preservation_scale = 1.0;
		if (policy.preservesOrganArea() &&
		    vegetationTriangleRegionPreservesOrganArea(working.region->role()) &&
		    working.allocated_triangle_count <
			    working.region->sourceTriangleCount()) {
			area_preservation_scale = std::sqrt(
				static_cast<double>(working.region->sourceTriangleCount()) /
				static_cast<double>(working.allocated_triangle_count));
		}
		allocations.emplace_back(
			working.region->regionIdentifier(), working.region->role(),
			working.region->sourceTriangleCount(),
			working.allocated_triangle_count, meshlet_count,
			working.importance_score, area_preservation_scale);
	}
	return VegetationTriangleDistributionResult::succeeded(
		VegetationTriangleDistributionSnapshot(
			allocations, source_triangle_count,
			policy.targetTriangleCount(),
			evidence_hash(policy, allocations)));
}
