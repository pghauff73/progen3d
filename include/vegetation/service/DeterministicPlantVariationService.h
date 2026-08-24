#pragma once

#include <cstdint>
#include <string>

class DeterministicPlantVariationService
{
public:
	float sampleUnit(std::uint64_t seed, const std::string &scope) const;
	float sampleSigned(std::uint64_t seed, const std::string &scope) const;
	std::uint64_t hash(std::uint64_t seed, const std::string &scope) const;
};

