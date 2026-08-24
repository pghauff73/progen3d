#include "geometry/service/AxialProfileSpecificationValidator.h"

#include "geometry/service/PolygonContainmentAnalyzer.h"
#include "geometry/service/SimplePolygonValidator.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <unordered_map>

namespace {

constexpr std::size_t kMaximumProfiles = 128;
constexpr std::size_t kMaximumVerticesPerProfile = 256;
constexpr std::size_t kMaximumDeclaredVertices = 32768;
constexpr std::size_t kMaximumLevels = 512;
constexpr std::size_t kMaximumSteps = 256;
constexpr std::size_t kMaximumGeneratedVertices = 131072;
constexpr std::size_t kMaximumGeneratedTriangles = 262144;
constexpr float kLevelTolerance = 1.0e-6f;

float canonical_float(float value)
{
	return value == 0.0f ? 0.0f : value;
}

float canonical_rotation(float degrees)
{
	float result = std::fmod(degrees, 360.0f);
	if (result < 0.0f) result += 360.0f;
	return canonical_float(result);
}

std::uint32_t float_bits(float value)
{
	const float canonical = canonical_float(value);
	std::uint32_t bits = 0;
	std::memcpy(&bits, &canonical, sizeof(bits));
	return bits;
}

bool finite_vector(const glm::vec2 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y);
}

bool nearly_equal(float first, float second)
{
	return std::fabs(first - second) <= kLevelTolerance;
}

bool nearly_equal(const glm::vec2 &first, const glm::vec2 &second)
{
	return nearly_equal(first.x, second.x) &&
	       nearly_equal(first.y, second.y);
}

std::vector<glm::vec2> transform_profile(
	const AxialProfilePolygon &profile,
	const AxialProfileLevelCandidate &level)
{
	const float radians = glm::radians(level.rotation_degrees);
	const float cosine = std::cos(radians);
	const float sine = std::sin(radians);
	std::vector<glm::vec2> transformed;
	transformed.reserve(profile.vertices().size());
	for (const glm::vec2 &vertex : profile.vertices()) {
		const glm::vec2 scaled(vertex.x * level.scale.x,
		                       vertex.y * level.scale.y);
		transformed.emplace_back(
			level.center.x + cosine * scaled.x - sine * scaled.y,
			level.center.y + sine * scaled.x + cosine * scaled.y);
	}
	return transformed;
}

const char *transition_name(AxialTransitionKind transition)
{
	switch (transition) {
	case AxialTransitionKind::Initial: return "at";
	case AxialTransitionKind::Hold: return "hold";
	case AxialTransitionKind::Linear: return "linear";
	case AxialTransitionKind::Step: return "step";
	}
	return "unknown";
}

}

std::shared_ptr<const AxialProfileShapeSpecification>
AxialProfileSpecificationValidator::validate(
	AxialProfileSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	if (candidate.axis != AxialProfileAxis::Y) {
		if (diagnostic != nullptr) {
			*diagnostic = "AxialProfilev1 supports only axis(y).";
		}
		return {};
	}
	if (candidate.profiles.empty() || candidate.profiles.size() > kMaximumProfiles) {
		if (diagnostic != nullptr) {
			*diagnostic = "AxialProfile requires 1 to 128 named profiles.";
		}
		return {};
	}
	if (candidate.levels.size() < 2 || candidate.levels.size() > kMaximumLevels) {
		if (diagnostic != nullptr) {
			*diagnostic = "AxialProfile requires 2 to 512 axial states.";
		}
		return {};
	}

	SimplePolygonValidator polygon_validator;
	std::vector<AxialProfilePolygon> profiles;
	profiles.reserve(candidate.profiles.size());
	std::unordered_map<std::string, std::size_t> profile_indices;
	std::size_t declared_vertex_count = 0;
	std::size_t expected_vertex_count = 0;
	for (AxialProfilePolygonCandidate &profile_candidate : candidate.profiles) {
		if (profile_candidate.name.empty()) {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfile profile names cannot be empty.";
			}
			return {};
		}
		if (profile_indices.count(profile_candidate.name) != 0) {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfile profile '" + profile_candidate.name +
				              "' is defined more than once.";
			}
			return {};
		}
		std::vector<glm::vec2> normalized_vertices;
		std::string polygon_diagnostic;
		if (!polygon_validator.normalize(profile_candidate.vertices,
		                                &normalized_vertices,
		                                &polygon_diagnostic)) {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfile profile '" + profile_candidate.name +
				              "': " + polygon_diagnostic;
			}
			return {};
		}
		if (normalized_vertices.size() > kMaximumVerticesPerProfile) {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfile profiles may contain at most 256 vertices.";
			}
			return {};
		}
		if (expected_vertex_count == 0) {
			expected_vertex_count = normalized_vertices.size();
		} else if (normalized_vertices.size() != expected_vertex_count) {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfilev1 requires equal vertex counts for all profiles.";
			}
			return {};
		}
		declared_vertex_count += normalized_vertices.size();
		if (declared_vertex_count > kMaximumDeclaredVertices) {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfile exceeds the 32768 declared-vertex limit.";
			}
			return {};
		}
		profile_indices.emplace(profile_candidate.name, profiles.size());
		profiles.emplace_back(profile_candidate.name, std::move(normalized_vertices));
	}

	if (candidate.levels.front().transition != AxialTransitionKind::Initial) {
		if (diagnostic != nullptr) {
			*diagnostic = "AxialProfile must begin with one at() state.";
		}
		return {};
	}

	std::vector<AxialProfileLevel> levels;
	levels.reserve(candidate.levels.size());
	std::size_t step_count = 0;
	float minimum_position = candidate.levels.front().axial_position;
	float maximum_position = minimum_position;
	for (std::size_t level_index = 0;
	     level_index < candidate.levels.size();
	     ++level_index) {
		AxialProfileLevelCandidate &level_candidate = candidate.levels[level_index];
		if (level_index > 0 &&
		    level_candidate.transition == AxialTransitionKind::Initial) {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfile may contain only one initial at() state.";
			}
			return {};
		}
		if (!std::isfinite(level_candidate.axial_position) ||
		    !finite_vector(level_candidate.center) ||
		    !finite_vector(level_candidate.scale) ||
		    !std::isfinite(level_candidate.rotation_degrees)) {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfile level values must be finite.";
			}
			return {};
		}
		if (level_candidate.scale.x * level_candidate.scale.y <= 0.0f) {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfile scale must be non-zero and orientation-preserving.";
			}
			return {};
		}
		const auto profile = profile_indices.find(level_candidate.profile_name);
		if (profile == profile_indices.end()) {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfile profile '" + level_candidate.profile_name +
				              "' is not defined.";
			}
			return {};
		}
		level_candidate.axial_position = canonical_float(level_candidate.axial_position);
		level_candidate.center.x = canonical_float(level_candidate.center.x);
		level_candidate.center.y = canonical_float(level_candidate.center.y);
		level_candidate.scale.x = canonical_float(level_candidate.scale.x);
		level_candidate.scale.y = canonical_float(level_candidate.scale.y);
		level_candidate.rotation_degrees =
			canonical_rotation(level_candidate.rotation_degrees);

		if (level_index > 0) {
			const AxialProfileLevelCandidate &previous = candidate.levels[level_index - 1];
			if (level_candidate.transition == AxialTransitionKind::Step) {
				++step_count;
				if (step_count > kMaximumSteps ||
				    !nearly_equal(level_candidate.axial_position,
				                  previous.axial_position)) {
					if (diagnostic != nullptr) {
						*diagnostic = step_count > kMaximumSteps
							? "AxialProfile exceeds the 256-step limit."
							: "AxialProfile step() must occur at the current axial position.";
					}
					return {};
				}
				const auto relationship = PolygonContainmentAnalyzer().analyze(
					transform_profile(profiles[profile_indices.at(previous.profile_name)], previous),
					transform_profile(profiles[profile->second], level_candidate));
				if (relationship != PolygonContainmentRelationship::FirstContainsSecond &&
				    relationship != PolygonContainmentRelationship::SecondContainsFirst) {
					if (diagnostic != nullptr) {
						*diagnostic =
							"AxialProfile step profiles intersect and cannot be resolved by v1.";
					}
					return {};
				}
			} else if (!(level_candidate.axial_position >
			             previous.axial_position + kLevelTolerance)) {
				if (diagnostic != nullptr) {
					*diagnostic = "AxialProfile axial levels must be strictly increasing.";
				}
				return {};
			}
			if (level_candidate.transition == AxialTransitionKind::Hold &&
			    (level_candidate.profile_name != previous.profile_name ||
			     !nearly_equal(level_candidate.center, previous.center) ||
			     !nearly_equal(level_candidate.scale, previous.scale) ||
			     !nearly_equal(level_candidate.rotation_degrees,
			                   previous.rotation_degrees))) {
				if (diagnostic != nullptr) {
					*diagnostic = "AxialProfile hold() must preserve the current section exactly.";
				}
				return {};
			}
		}

		minimum_position = std::min(minimum_position, level_candidate.axial_position);
		maximum_position = std::max(maximum_position, level_candidate.axial_position);
		levels.emplace_back(level_candidate.axial_position,
		                    profile->second,
		                    level_candidate.center,
		                    level_candidate.scale,
		                    level_candidate.rotation_degrees,
		                    level_candidate.transition);
	}
	if (!(maximum_position > minimum_position + kLevelTolerance)) {
		if (diagnostic != nullptr) {
			*diagnostic = "AxialProfile must span a non-zero axial distance.";
		}
		return {};

	}

	const std::size_t transition_count = levels.size() - 1;
	const std::size_t cap_count =
		(candidate.cap_policy.capsBottom() ? 1u : 0u) +
		(candidate.cap_policy.capsTop() ? 1u : 0u);
	const std::size_t estimated_triangles =
		transition_count * expected_vertex_count * 2u +
		cap_count * (expected_vertex_count - 2u);
	const std::size_t estimated_vertices = estimated_triangles * 3u;
	if (estimated_triangles > kMaximumGeneratedTriangles ||
	    estimated_vertices > kMaximumGeneratedVertices) {
		if (diagnostic != nullptr) {
			*diagnostic = "AxialProfile exceeds the generated mesh safety ceiling.";
		}
		return {};
	}

	std::ostringstream key;
	key << "axial-profile-v1|" << static_cast<int>(candidate.axis) << "|";
	for (const AxialProfilePolygon &profile : profiles) {
		key << "p:" << profile.vertices().size();
		for (const glm::vec2 &vertex : profile.vertices()) {
			key << ":" << float_bits(vertex.x) << ":" << float_bits(vertex.y);
		}
		key << "|";
	}
	for (const AxialProfileLevel &level : levels) {
		key << "l:" << static_cast<int>(level.transition())
		    << ":" << float_bits(level.axialPosition())
		    << ":" << level.profileIndex()
		    << ":" << float_bits(level.center().x)
		    << ":" << float_bits(level.center().y)
		    << ":" << float_bits(level.scale().x)
		    << ":" << float_bits(level.scale().y)
		    << ":" << float_bits(level.rotationDegrees()) << "|";
	}
	key << "caps:" << candidate.cap_policy.capsBottom()
	    << ":" << candidate.cap_policy.capsTop();

	std::ostringstream text;
	text << "AxialProfile(axis(y)";
	for (const AxialProfilePolygon &profile : profiles) {
		text << " profile(" << profile.name() << " polygon(";
		for (std::size_t index = 0; index < profile.vertices().size(); ++index) {
			if (index > 0) text << " ";
			text << profile.vertices()[index].x << " "
			     << profile.vertices()[index].y;
		}
		text << "))";
	}
	for (const AxialProfileLevel &level : levels) {
		text << " " << transition_name(level.transition()) << "("
		     << level.axialPosition() << " "
		     << profiles[level.profileIndex()].name()
		     << " center(" << level.center().x << " " << level.center().y << ")"
		     << " scale(" << level.scale().x << " " << level.scale().y << ")"
		     << " rotate(" << level.rotationDegrees() << "))";
	}
	text << " cap(";
	if (candidate.cap_policy.closesBothEnds()) text << "all";
	else if (candidate.cap_policy.capsBottom()) text << "bottom";
	else if (candidate.cap_policy.capsTop()) text << "top";
	else text << "none";
	text << "))";

	return std::make_shared<AxialProfileShapeSpecification>(
		candidate.axis,
		std::move(profiles),
		std::move(levels),
		candidate.cap_policy,
		ShapeSpecificationKey(key.str()),
		text.str());
}
