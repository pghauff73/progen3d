#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

class VehicleSurfaceOwnership
{
public:
	VehicleSurfaceOwnership(
		std::vector<std::string> face_owner_identifiers,
		std::map<std::string, std::size_t> owner_face_counts,
		std::size_t unowned_face_count,
		std::size_t multiply_owned_face_count,
		double coverage_fraction,
		std::uint64_t deterministic_hash)
		: face_owner_identifiers_(std::move(face_owner_identifiers)),
		  owner_face_counts_(std::move(owner_face_counts)),
		  unowned_face_count_(unowned_face_count),
		  multiply_owned_face_count_(multiply_owned_face_count),
		  coverage_fraction_(coverage_fraction),
		  deterministic_hash_(deterministic_hash)
	{
	}

	const std::vector<std::string> &faceOwnerIdentifiers() const
	{
		return face_owner_identifiers_;
	}
	const std::map<std::string, std::size_t> &ownerFaceCounts() const
	{
		return owner_face_counts_;
	}
	std::size_t unownedFaceCount() const { return unowned_face_count_; }
	std::size_t multiplyOwnedFaceCount() const { return multiply_owned_face_count_; }
	double coverageFraction() const { return coverage_fraction_; }
	std::uint64_t deterministicHash() const { return deterministic_hash_; }
	bool passed() const
	{
		return unowned_face_count_ == 0u && multiply_owned_face_count_ == 0u &&
		       coverage_fraction_ >= 1.0;
	}

private:
	std::vector<std::string> face_owner_identifiers_;
	std::map<std::string, std::size_t> owner_face_counts_;
	std::size_t unowned_face_count_ = 0u;
	std::size_t multiply_owned_face_count_ = 0u;
	double coverage_fraction_ = 0.0;
	std::uint64_t deterministic_hash_ = 0u;
};
