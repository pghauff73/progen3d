#pragma once

#include <glm/glm.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>

struct PreviewVisibilityBounds {
	glm::vec3 minimum{0.0f};
	glm::vec3 maximum{0.0f};
	bool valid = false;
};

struct PreviewScreenBounds {
	// Coordinates are viewport pixels measured from the bottom-left corner.
	glm::dvec2 minimum{0.0};
	glm::dvec2 maximum{0.0};
	double nearest_depth = 0.0;
	double farthest_depth = 1.0;
};

class PreviewVisibilityBoundsProjection {
public:
	struct Result {
		bool outside_frustum = false;
		bool depth_test_safe = false;
		std::optional<PreviewScreenBounds> screen_bounds;
	};

	PreviewVisibilityBoundsProjection(const glm::mat4 &projection,
	                                  const glm::mat4 &view,
	                                  const glm::mat4 &model,
	                                  int width,
	                                  int height)
		: clip_from_world_(glm::dmat4(projection) * glm::dmat4(view) * glm::dmat4(model)),
		  width_(width), height_(height)
	{
		const glm::dmat4 model_magnitude = absoluteMatrix(model);
		const glm::dmat4 view_magnitude = absoluteMatrix(view) * model_magnitude;
		const glm::dmat4 clip_magnitude = absoluteMatrix(projection) * view_magnitude;
		for (int column = 0; column < 4; ++column) {
			arithmetic_scale_[column] = glm::max(
				glm::max(model_magnitude[column], view_magnitude[column]), clip_magnitude[column]);
		}
	}

	static Result project(const PreviewVisibilityBounds &bounds,
	                      const glm::mat4 &clip_from_world,
	                      int width,
	                      int height)
	{
		const glm::mat4 identity(1.0f);
		return PreviewVisibilityBoundsProjection(clip_from_world, identity, identity, width, height).project(bounds);
	}

	Result project(const PreviewVisibilityBounds &bounds) const
	{
		if (!bounds.valid || width_ <= 0 || height_ <= 0 ||
		    glm::any(glm::greaterThan(bounds.minimum, bounds.maximum))) {
			return {};
		}

		const glm::dvec3 minimum(bounds.minimum);
		const glm::dvec3 maximum(bounds.maximum);
		const glm::dvec4 input_magnitude(glm::max(glm::abs(minimum), glm::abs(maximum)), 1.0);
		const auto clip_error = clipError(input_magnitude);
		if (!clip_error) {
			return {};
		}

		std::array<ClipInterval, 8> corners;
		glm::bvec3 outside_negative(true);
		glm::bvec3 outside_positive(true);
		bool beyond_near_plane = true;

		for (std::size_t index = 0; index < corners.size(); ++index) {
			const glm::dvec4 position(
				(index & 1u) ? maximum.x : minimum.x,
				(index & 2u) ? maximum.y : minimum.y,
				(index & 4u) ? maximum.z : minimum.z,
				1.0);
			const glm::dvec4 clip = clip_from_world_ * position;
			if (!finite(clip)) {
				return {};
			}

			const ClipInterval corner{clip - *clip_error, clip + *clip_error};
			corners[index] = corner;
			for (int axis = 0; axis < 3; ++axis) {
				outside_negative[axis] = outside_negative[axis] && corner.maximum[axis] < -corner.maximum.w;
				outside_positive[axis] = outside_positive[axis] && corner.minimum[axis] > corner.maximum.w;
			}
			beyond_near_plane = beyond_near_plane && pastNearPlane(corner);
		}

		if (glm::any(outside_negative) || glm::any(outside_positive)) {
			return {true, false, std::nullopt};
		}
		if (!beyond_near_plane) {
			return {};
		}

		const auto screen_bounds = projectToViewport(corners, true);
		return {false, screen_bounds.has_value(), screen_bounds};
	}

	// Project a single occluder vertex, retaining off-screen coordinates so
	// clipping against the viewport does not change the triangle's edges.
	std::optional<PreviewScreenBounds> projectPoint(const glm::vec3 &point) const
	{
		if (width_ <= 0 || height_ <= 0) {
			return std::nullopt;
		}
		const glm::dvec4 position(glm::dvec3(point), 1.0);
		const auto clip_error = clipError(glm::abs(position));
		if (!clip_error) {
			return std::nullopt;
		}
		const glm::dvec4 clip = clip_from_world_ * position;
		const ClipInterval interval{clip - *clip_error, clip + *clip_error};
		if (!finite(clip) || !pastNearPlane(interval)) {
			return std::nullopt;
		}
		return projectToViewport(std::array<ClipInterval, 1>{interval}, false);
	}

	static bool missesSingleSampleGrid(const PreviewScreenBounds &bounds)
	{
		return std::ceil(bounds.minimum.x - 0.5) > std::floor(bounds.maximum.x - 0.5) ||
		       std::ceil(bounds.minimum.y - 0.5) > std::floor(bounds.maximum.y - 0.5);
	}

private:
	struct ClipInterval {
		glm::dvec4 minimum;
		glm::dvec4 maximum;
	};

	static glm::dmat4 absoluteMatrix(const glm::mat4 &matrix)
	{
		glm::dmat4 absolute(0.0);
		for (int column = 0; column < 4; ++column) {
			absolute[column] = glm::abs(glm::dvec4(matrix[column]));
		}
		return absolute;
	}

	static bool finite(const glm::dvec4 &value)
	{
		return std::isfinite(value.x) && std::isfinite(value.y) &&
		       std::isfinite(value.z) && std::isfinite(value.w);
	}

	static bool pastNearPlane(const ClipInterval &corner)
	{
		return corner.minimum.w > 0.0 && corner.minimum.z > -corner.minimum.w;
	}

	std::optional<glm::dvec4> clipError(const glm::dvec4 &input_magnitude) const
	{
		const glm::dvec4 arithmetic_magnitude = arithmetic_scale_ * input_magnitude;
		if (!finite(input_magnitude) || !finite(arithmetic_magnitude) ||
		    glm::any(glm::greaterThan(arithmetic_magnitude,
		                             glm::dvec4(std::numeric_limits<float>::max() / 4.0)))) {
			return std::nullopt;
		}
		// The shader uses float model/view/projection arithmetic. Absolute
		// matrix products keep this error band valid even under cancellation.
		return (arithmetic_magnitude + glm::dvec4(1.0)) * (64.0 * std::numeric_limits<float>::epsilon());
	}

	template <std::size_t CornerCount>
	std::optional<PreviewScreenBounds> projectToViewport(
		const std::array<ClipInterval, CornerCount> &corners, bool clamp_to_viewport) const
	{
		const double infinite = std::numeric_limits<double>::infinity();
		glm::dvec3 normalized_minimum(infinite);
		glm::dvec3 normalized_maximum(-infinite);
		for (const ClipInterval &corner : corners) {
			const glm::dvec3 clip_minimum(corner.minimum);
			const glm::dvec3 clip_maximum(corner.maximum);
			normalized_minimum = glm::min(normalized_minimum, glm::min(
				clip_minimum / corner.minimum.w, clip_minimum / corner.maximum.w));
			normalized_maximum = glm::max(normalized_maximum, glm::max(
				clip_maximum / corner.minimum.w, clip_maximum / corner.maximum.w));
		}

		if (!finite(glm::dvec4(normalized_minimum, 1.0)) || !finite(glm::dvec4(normalized_maximum, 1.0))) {
			return std::nullopt;
		}

		const glm::dvec2 viewport(width_, height_);
		// Pad one subpixel at OpenGL's minimum precision plus float viewport math.
		const glm::dvec2 normalized_magnitude = glm::max(
			glm::abs(glm::dvec2(normalized_minimum)), glm::abs(glm::dvec2(normalized_maximum)));
		const glm::dvec2 raster_padding = glm::dvec2(1.0 / 16.0) + (normalized_magnitude + 1.0) *
			viewport * (8.0 * std::numeric_limits<float>::epsilon());
		glm::dvec2 window_minimum = (glm::dvec2(normalized_minimum) * 0.5 + 0.5) * viewport - raster_padding;
		glm::dvec2 window_maximum = (glm::dvec2(normalized_maximum) * 0.5 + 0.5) * viewport + raster_padding;
		if (clamp_to_viewport) {
			window_minimum = glm::clamp(window_minimum, glm::dvec2(0.0), viewport);
			window_maximum = glm::clamp(window_maximum, glm::dvec2(0.0), viewport);
		}
		// Include float depth interpolation and one unit of 24-bit depth quantization.
		constexpr double depth_padding = 8.0 * std::numeric_limits<float>::epsilon() + 1.0 / 16777215.0;
		return PreviewScreenBounds{
			window_minimum,
			window_maximum,
			glm::clamp(normalized_minimum.z * 0.5 + 0.5 - depth_padding, 0.0, 1.0),
			glm::clamp(normalized_maximum.z * 0.5 + 0.5 + depth_padding, 0.0, 1.0)};
	}

	glm::dmat4 clip_from_world_;
	glm::dmat4 arithmetic_scale_{0.0};
	int width_;
	int height_;
};
