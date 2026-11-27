#include "ShotCameraRail.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace {

constexpr int arc_intervals = 128;
constexpr double minimum_vector_length = 1.0e-10;

glm::dvec3 unit_vector(const glm::dvec3 &vector, const char *message)
{
	const double length = glm::length(vector);
	if (!std::isfinite(length) || length <= minimum_vector_length) {
		throw std::runtime_error(message);
	}
	return vector / length;
}

glm::dvec3 curve_position(const ShotRailCurve &curve, double u)
{
	const glm::dvec3 p01 = glm::mix(curve[0], curve[1], u);
	const glm::dvec3 p12 = glm::mix(curve[1], curve[2], u);
	const glm::dvec3 p23 = glm::mix(curve[2], curve[3], u);
	return glm::mix(glm::mix(p01, p12, u), glm::mix(p12, p23, u), u);
}

glm::dvec3 curve_derivative(const ShotRailCurve &curve, double u)
{
	const double v = 1.0 - u;
	return 3.0 * (v * v * (curve[1] - curve[0]) +
	              2.0 * v * u * (curve[2] - curve[1]) +
	              u * u * (curve[3] - curve[2]));
}

glm::dvec3 curve_tangent(const ShotRailCurve &curve, double u)
{
	return unit_vector(curve_derivative(curve, u),
	                   "Camera rail has a stationary or undefined curve tangent.");
}

double arc_length(const ShotRailCurve &curve, double first, double last)
{
	// Five-point Gauss-Legendre quadrature on one small curve interval.
	constexpr double nodes[] = {0.0, -0.5384693101056831, 0.5384693101056831,
	                            -0.9061798459386640, 0.9061798459386640};
	constexpr double weights[] = {0.5688888888888889, 0.4786286704993665, 0.4786286704993665,
	                              0.2369268850561891, 0.2369268850561891};
	const double center = (first + last) * 0.5;
	const double half_width = (last - first) * 0.5;
	double length = 0.0;
	for (int index = 0; index < 5; ++index) {
		length += weights[index] * glm::length(curve_derivative(curve, center + half_width * nodes[index]));
	}
	return half_width * length;
}

double solve_arc_parameter(const ShotRailCurve &curve, double first, double last,
                           double interval_length, double desired_length)
{
	if (desired_length <= 0.0) return first;
	if (desired_length >= interval_length) return last;
	double lower = first;
	double upper = last;
	double estimate = glm::mix(first, last, desired_length / interval_length);
	for (int iteration = 0; iteration < 12; ++iteration) {
		const double difference = arc_length(curve, first, estimate) - desired_length;
		if (std::abs(difference) <= interval_length * 1.0e-10) break;
		if (difference > 0.0) upper = estimate;
		else lower = estimate;
		const double derivative = glm::length(curve_derivative(curve, estimate));
		const double candidate = estimate - difference / derivative;
		estimate = candidate > lower && candidate < upper ? candidate : (lower + upper) * 0.5;
	}
	return estimate;
}

glm::dvec3 transport_up(const glm::dvec3 &up, const glm::dvec3 &previous_tangent,
                       const glm::dvec3 &tangent)
{
	const double alignment = glm::dot(previous_tangent, tangent);
	if (alignment <= -0.999999) {
		throw std::runtime_error("Camera rail tangent reverses within a curve interval.");
	}
	const glm::dvec3 axis = glm::cross(previous_tangent, tangent);
	const glm::dvec3 rotated = up + glm::cross(axis, up) +
	                          glm::cross(axis, glm::cross(axis, up)) / (1.0 + alignment);
	return unit_vector(rotated - tangent * glm::dot(rotated, tangent), "Camera rail transport failed.");
}

glm::dmat3 rigid_anchor_axes(const glm::mat4 &frame)
{
	const glm::dvec3 right = unit_vector(glm::dvec3(frame[0]), "Camera rail anchor has a degenerate X axis.");
	const glm::dvec3 raw_up(frame[1]);
	const glm::dvec3 up = unit_vector(raw_up - right * glm::dot(raw_up, right),
	                                  "Camera rail anchor has a degenerate Y axis.");
	return {right, up, glm::cross(right, up)};
}

std::vector<double> progress_slopes(const std::vector<ShotKeyframe<double>> &keys)
{
	std::vector<double> slopes(keys.size(), 0.0);
	for (std::size_t index = 1; index < keys.size(); ++index) {
		if (keys[index].value < keys[index - 1].value) {
			throw std::runtime_error("Camera rail progress must be nondecreasing.");
		}
	}
	// Monotone cubic Hermite slopes. Stops at the endpoints and at explicit holds.
	for (std::size_t index = 1; index + 1 < keys.size(); ++index) {
		const double dt_before = keys[index].time_seconds - keys[index - 1].time_seconds;
		const double dt_after = keys[index + 1].time_seconds - keys[index].time_seconds;
		const double before = (keys[index].value - keys[index - 1].value) / dt_before;
		const double after = (keys[index + 1].value - keys[index].value) / dt_after;
		if (before == 0.0 || after == 0.0) continue;
		const double after_weight = (1.0 + dt_before / (dt_before + dt_after)) / 3.0;
		const double before_weight = 1.0 - after_weight;
		slopes[index] = 1.0 / (before_weight / before + after_weight / after);
	}
	return slopes;
}

}

ShotCameraRail::ShotCameraRail(std::string id, std::vector<ShotRailCurve> curves,
                               std::vector<ShotKeyframe<double>> progress,
                               std::optional<ShotAttachment> anchor, glm::dvec3 up)
	: id_(std::move(id)), progress_(std::move(progress)), progress_slopes_(progress_slopes(progress_)),
	  anchor_(std::move(anchor))
{
	if (curves.empty() || progress_.empty()) {
		throw std::runtime_error("Camera rail needs at least one curve and one progress key.");
	}
	for (std::size_t index = 1; index < progress_.size(); ++index) {
		const double span = progress_[index].time_seconds - progress_[index - 1].time_seconds;
		if (!std::isfinite(1.0 / span) || !std::isfinite(progress_slopes_[index] * span) ||
		    !std::isfinite(progress_slopes_[index - 1] * span)) {
			throw std::runtime_error("Camera rail progress keyframe spacing is outside the motion range.");
		}
	}

	segments_.reserve(curves.size());
	glm::dvec3 tangent = curve_tangent(curves.front(), 0.0);
	up = unit_vector(up - tangent * glm::dot(up, tangent),
	                 "Camera rail up must be independent of its initial tangent.");
	for (auto &curve : curves) {
		if (!segments_.empty()) {
			if (glm::length(curve[0] - segments_.back().curve[3]) > 1.0e-7) {
				throw std::runtime_error("Adjacent camera rail curves must share endpoints.");
			}
			curve[0] = segments_.back().curve[3];
			if (glm::length(curve_tangent(curve, 0.0) - tangent) > 1.0e-5) {
				throw std::runtime_error("Adjacent camera rail curves must share the same tangent direction.");
			}
			curve[1] = curve[0] + tangent * glm::length(curve[1] - curve[0]);
		}

		Segment segment{curve, {}};
		segment.samples.reserve(arc_intervals + 1);
		segment.samples.push_back({0.0, length_, tangent, up});
		for (int index = 1; index <= arc_intervals; ++index) {
			const double parameter = static_cast<double>(index) / arc_intervals;
			const auto next_tangent = curve_tangent(curve, parameter);
			up = transport_up(up, tangent, next_tangent);
			tangent = next_tangent;
			const double next_distance = length_ + arc_length(curve, segment.samples.back().parameter, parameter);
			if (!std::isfinite(next_distance) || next_distance <= length_) {
				throw std::runtime_error("Camera rail arc length must be finite and increasing.");
			}
			length_ = next_distance;
			segment.samples.push_back({parameter, length_, tangent, up});
		}
		segments_.push_back(std::move(segment));
	}
}

std::pair<double, double> ShotCameraRail::sampleProgress(double time_seconds) const
{
	const auto next = std::upper_bound(progress_.begin(), progress_.end(), time_seconds,
		[](double time, const auto &key) { return time < key.time_seconds; });
	if (next == progress_.begin()) return {progress_.front().value, 0.0};
	if (next == progress_.end()) return {progress_.back().value, 0.0};
	const std::size_t index = static_cast<std::size_t>(next - progress_.begin());
	const auto &first = progress_[index - 1];
	const auto &last = *next;
	const double span = last.time_seconds - first.time_seconds;
	const double u = (time_seconds - first.time_seconds) / span;
	const double squared = u * u;
	const double cubed = squared * u;
	const double start_slope = progress_slopes_[index - 1];
	const double end_slope = progress_slopes_[index];
	const double progress = first.value + (last.value - first.value) * (3.0 * squared - 2.0 * cubed) +
	                        span * ((cubed - 2.0 * squared + u) * start_slope + (cubed - squared) * end_slope);
	const double speed = (last.value - first.value) / span * (6.0 * u - 6.0 * squared) +
	                     (3.0 * squared - 4.0 * u + 1.0) * start_slope + (3.0 * squared - 2.0 * u) * end_slope;
	return {std::clamp(progress, first.value, last.value), std::max(0.0, speed)};
}

ShotRailPose ShotCameraRail::sample(const SceneGenerationContext &scene, double time_seconds) const
{
	const auto [progress, rate] = sampleProgress(time_seconds);
	const double distance = progress * length_;
	const double speed = rate * length_;
	if (!std::isfinite(speed)) throw std::runtime_error("Camera rail speed exceeds the supported range.");
	const auto next_segment = std::upper_bound(segments_.begin(), segments_.end(), distance,
		[](double position, const Segment &segment) { return position < segment.samples.back().distance; });
	const auto &segment = next_segment == segments_.end() ? segments_.back() : *next_segment;
	const auto next_sample = std::upper_bound(segment.samples.begin(), segment.samples.end(), distance,
		[](double position, const ArcSample &sample) { return position < sample.distance; });
	const auto interval_end = next_sample == segment.samples.end() ? std::prev(next_sample) : next_sample;
	const auto &first = *std::prev(interval_end);
	const auto &last = *interval_end;
	const double parameter = solve_arc_parameter(segment.curve, first.parameter, last.parameter,
	                                            last.distance - first.distance, distance - first.distance);
	const glm::dvec3 tangent = curve_tangent(segment.curve, parameter);
	glm::dvec3 position = curve_position(segment.curve, parameter);
	glm::dvec3 up = transport_up(first.up, first.tangent, tangent);
	if (anchor_) {
		const auto frame = resolve_scene_attachment_frame(scene, *anchor_);
		const auto axes = rigid_anchor_axes(frame);
		position = glm::dvec3(frame[3]) + axes * position;
		up = axes * up;
	}
	return {position, up, {id_, distance, speed}};
}
