#include "vegetation/service/DeterministicPlantVariationService.h"

#include <limits>

std::uint64_t DeterministicPlantVariationService::hash(
	std::uint64_t seed,
	const std::string &scope) const
{
	std::uint64_t value = 1469598103934665603ull ^ seed;
	for (unsigned char character : scope) {
		value ^= character;
		value *= 1099511628211ull;
	}
	value ^= value >> 30u;
	value *= 0xbf58476d1ce4e5b9ull;
	value ^= value >> 27u;
	value *= 0x94d049bb133111ebull;
	value ^= value >> 31u;
	return value;
}

float DeterministicPlantVariationService::sampleUnit(
	std::uint64_t seed,
	const std::string &scope) const
{
	const std::uint64_t value = hash(seed, scope);
	const double normalized = static_cast<double>(value >> 11u) /
	                          static_cast<double>(1ull << 53u);
	return static_cast<float>(normalized);
}

float DeterministicPlantVariationService::sampleSigned(
	std::uint64_t seed,
	const std::string &scope) const
{
	return sampleUnit(seed, scope) * 2.0f - 1.0f;
}

