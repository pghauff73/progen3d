#include "geometry/service/VariableSectionSweepSpecificationValidator.h"

#include <glm/geometric.hpp>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <utility>

namespace {

constexpr float kSweepTolerance = 1.0e-6f;

bool finite(const glm::vec2 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y);
}

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

float canonical_float(float value)
{
	return value == 0.0f ? 0.0f : value;
}

std::string float_bits(float value)
{
	value = canonical_float(value);
	std::uint32_t bits = 0u;
	std::memcpy(&bits, &value, sizeof(bits));
	std::ostringstream text;
	text << std::hex << std::setw(8) << std::setfill('0') << bits;
	return text.str();
}

bool profiles_correspond(const Profile2D &first, const Profile2D &second)
{
	if (first.outerLoop().points().size() != second.outerLoop().points().size() ||
	    first.innerLoops().size() != second.innerLoops().size()) {
		return false;
	}
	for (std::size_t loop_index = 0;
	     loop_index < first.innerLoops().size();
	     ++loop_index) {
		if (first.innerLoops()[loop_index].points().size() !=
		    second.innerLoops()[loop_index].points().size()) {
			return false;
		}
	}
	return true;
}

const char *frame_policy_text(SweepFramePolicy policy)
{
	return policy == SweepFramePolicy::ReferenceAligned
		? "reference-aligned"
		: "rotation-minimizing";
}

void append_loop_key(std::ostringstream *key, const ProfileLoop2D &loop)
{
	*key << loop.points().size() << ":";
	for (const glm::vec2 &point : loop.points()) {
		*key << float_bits(point.x) << "," << float_bits(point.y) << ";";
	}
}

void append_profile_key(std::ostringstream *key, const Profile2D &profile)
{
	append_loop_key(key, profile.outerLoop());
	*key << "holes=" << profile.innerLoops().size() << ":";
	for (const ProfileLoop2D &loop : profile.innerLoops()) {
		append_loop_key(key, loop);
	}
}

}

std::shared_ptr<const VariableSectionSweepShapeSpecification>
VariableSectionSweepSpecificationValidator::validate(
	VariableSectionSweepShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	if (candidate.path_points.size() < 2u ||
	    candidate.path_points.size() > complexity_limits_.maximumPathPoints()) {
		if (diagnostic != nullptr) {
			*diagnostic = "VariableSectionSweep requires between two and the configured maximum number of path points.";
		}
		return {};
	}
	float path_length = 0.0f;
	for (std::size_t point_index = 0;
	     point_index < candidate.path_points.size();
	     ++point_index) {
		if (!finite(candidate.path_points[point_index])) {
			if (diagnostic != nullptr) {
				*diagnostic = "VariableSectionSweep path points must be finite.";
			}
			return {};
		}
		if (point_index > 0u) {
			const float segment_length = glm::length(
				candidate.path_points[point_index] -
				candidate.path_points[point_index - 1u]);
			if (segment_length <= kSweepTolerance) {
				if (diagnostic != nullptr) {
					*diagnostic = "VariableSectionSweep path cannot contain zero-length segments.";
				}
				return {};
			}
			path_length += segment_length;
		}
	}
	if (path_length <= kSweepTolerance || !finite(candidate.up_hint) ||
	    glm::dot(candidate.up_hint, candidate.up_hint) <=
		kSweepTolerance * kSweepTolerance) {
		if (diagnostic != nullptr) {
			*diagnostic = "VariableSectionSweep requires a non-degenerate path and finite non-zero up hint.";
		}
		return {};
	}
	if (candidate.stations.size() < 2u ||
	    candidate.stations.size() > complexity_limits_.maximumLoftSections()) {
		if (diagnostic != nullptr) {
			*diagnostic = "VariableSectionSweep requires between two and the configured maximum number of stations.";
		}
		return {};
	}

	std::vector<VariableSectionSweepStationSpecification> stations;
	stations.reserve(candidate.stations.size());
	for (std::size_t station_index = 0;
	     station_index < candidate.stations.size();
	     ++station_index) {
		VariableSectionSweepStationCandidate &station_candidate =
			candidate.stations[station_index];
		if (!std::isfinite(station_candidate.normalized_path_position) ||
		    station_candidate.normalized_path_position < 0.0f ||
		    station_candidate.normalized_path_position > 1.0f ||
		    !finite(station_candidate.center) || !finite(station_candidate.scale) ||
		    station_candidate.scale.x <= 0.0f || station_candidate.scale.y <= 0.0f ||
		    !std::isfinite(station_candidate.rotation_degrees)) {
			if (diagnostic != nullptr) {
				*diagnostic = "VariableSectionSweep station parameters must be finite, positions must lie in [0,1], and scales must be positive.";
			}
			return {};
		}
		if (station_index > 0u &&
		    station_candidate.normalized_path_position <=
			stations.back().normalizedPathPosition()) {
			if (diagnostic != nullptr) {
				*diagnostic = "VariableSectionSweep station positions must be strictly increasing.";
			}
			return {};
		}
		auto profile = Profile2DValidator(complexity_limits_).validate(
			std::move(station_candidate.profile), diagnostic);
		if (!profile) return {};
		if (!stations.empty() &&
		    !profiles_correspond(stations.front().profile(), *profile)) {
			if (diagnostic != nullptr) {
				*diagnostic = "VariableSectionSweep v1 requires corresponding loop and vertex counts across stations.";
			}
			return {};
		}
		stations.emplace_back(
			station_candidate.normalized_path_position,
			*profile,
			station_candidate.center,
			station_candidate.scale,
			station_candidate.rotation_degrees);
	}
	if (std::fabs(stations.front().normalizedPathPosition()) > kSweepTolerance ||
	    std::fabs(stations.back().normalizedPathPosition() - 1.0f) > kSweepTolerance) {
		if (diagnostic != nullptr) {
			*diagnostic = "VariableSectionSweep v1 requires first and last stations at normalized path positions 0 and 1.";
		}
		return {};
	}

	std::vector<Profile2D> nominal_section_profiles;
	nominal_section_profiles.reserve(candidate.nominal_section_profiles.size());
	for (Profile2DCandidate &profile_candidate : candidate.nominal_section_profiles) {
		auto profile = Profile2DValidator(complexity_limits_).validate(
			std::move(profile_candidate), diagnostic);
		if (!profile) return {};
		nominal_section_profiles.push_back(*profile);
	}

	std::ostringstream key;
	key << "VariableSectionSweep:v1:cap=" << candidate.cap_policy.canonicalText()
	    << ":frame=" << frame_policy_text(candidate.frame_policy) << ":path=";
	for (const glm::vec3 &point : candidate.path_points) {
		key << float_bits(point.x) << "," << float_bits(point.y) << ","
		    << float_bits(point.z) << ";";
	}
	key << ":up=" << float_bits(candidate.up_hint.x) << ","
	    << float_bits(candidate.up_hint.y) << ","
	    << float_bits(candidate.up_hint.z);
	for (const VariableSectionSweepStationSpecification &station : stations) {
		key << ":station=" << float_bits(station.normalizedPathPosition()) << ","
		    << float_bits(station.center().x) << ","
		    << float_bits(station.center().y) << ","
		    << float_bits(station.scale().x) << ","
		    << float_bits(station.scale().y) << ","
		    << float_bits(station.rotationDegrees()) << ":";
		append_profile_key(&key, station.profile());
	}
	for (const Profile2D &profile : nominal_section_profiles) {
		key << ":nominal-section:";
		append_profile_key(&key, profile);
	}
	const std::string canonical_text =
		"VariableSectionSweep(pathPoints(" +
		std::to_string(candidate.path_points.size()) + ") stations(" +
		std::to_string(stations.size()) + ") nominalSections(" +
		std::to_string(nominal_section_profiles.size()) +
		") frame(" + std::string(frame_policy_text(candidate.frame_policy)) + ") cap(" +
		candidate.cap_policy.canonicalText() + "))";
	return std::make_shared<const VariableSectionSweepShapeSpecification>(
		std::move(candidate.path_points),
		std::move(stations),
		std::move(nominal_section_profiles),
		candidate.up_hint,
		candidate.frame_policy,
		candidate.cap_policy,
		ShapeSpecificationKey(key.str()),
		canonical_text,
		candidate.detail_level);
}
