#include "vegetation/service/CrownAttractionPointSamplingService.h"

#include "vegetation/service/CrownVolumeContainmentService.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace {

double halton(std::uint64_t index, std::uint64_t base)
{
	double fraction = 1.0;
	double result = 0.0;
	while (index > 0u) {
		fraction /= static_cast<double>(base);
		result += fraction * static_cast<double>(index % base);
		index /= base;
	}
	return result;
}

std::uint64_t mixed_seed(std::uint64_t seed)
{
	seed ^= seed >> 30u;
	seed *= 0xbf58476d1ce4e5b9ull;
	seed ^= seed >> 27u;
	seed *= 0x94d049bb133111ebull;
	seed ^= seed >> 31u;
	return seed;
}

} // namespace

CrownAttractionPointSamplingResult
CrownAttractionPointSamplingService::sample(
	const CrownVolumeSpecification &volume,
	std::size_t point_count,
	std::uint64_t deterministic_seed) const
{
	const CrownVolumeContainmentService containment_service;
	std::string diagnostic;
	if (!containment_service.validate(volume, &diagnostic)) {
		return CrownAttractionPointSamplingResult::failed(diagnostic);
	}
	if (point_count == 0u) {
		return CrownAttractionPointSamplingResult::failed(
			"Crown attraction sampling requires at least one point.");
	}
	const AxisAlignedBounds bounds = containment_service.bounds(volume);
	if (!bounds.valid) {
		return CrownAttractionPointSamplingResult::failed(
			"Crown attraction sampling could not derive valid bounds.");
	}
	std::vector<glm::vec3> points;
	points.reserve(point_count);
	const std::uint64_t start_index =
		1u + mixed_seed(deterministic_seed) % 104729u;
	const std::size_t maximum_attempts = point_count * 128u + 1024u;
	for (std::size_t attempt = 0;
	     attempt < maximum_attempts && points.size() < point_count;
	     ++attempt) {
		const std::uint64_t sequence_index =
			start_index + static_cast<std::uint64_t>(attempt);
		const glm::vec3 unit(
			static_cast<float>(halton(sequence_index, 2u)),
			static_cast<float>(halton(sequence_index, 3u)),
			static_cast<float>(halton(sequence_index, 5u)));
		const glm::vec3 candidate =
			bounds.min + unit * (bounds.max - bounds.min);
		if (containment_service.contains(volume, candidate)) {
			points.push_back(candidate);
		}
	}
	if (points.size() != point_count) {
		return CrownAttractionPointSamplingResult::failed(
			"Crown attraction sampling exhausted its deterministic attempt ceiling.");
	}
	return CrownAttractionPointSamplingResult::succeeded(std::move(points));
}
