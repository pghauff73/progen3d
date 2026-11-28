#pragma once

#include "editor/model/PreviewVisibilityBounds.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

class PreviewOcclusionDepth {
public:
	PreviewOcclusionDepth(int width, int height)
		: viewport_(width, height)
	{
		for (auto &level : levels_) {
			level.fill(1.0);
		}
	}

	// Each box is backed by six complete, opaque faces in the submitted geometry.
	// A tile enters the depth map only when one face covers its entire padded area.
	void addOpaqueBox(const PreviewVisibilityBounds &box,
	                  const PreviewVisibilityBoundsProjection &projection)
	{
		std::array<PreviewScreenBounds, 8> corners;
		for (std::size_t index = 0; index < corners.size(); ++index) {
			const glm::vec3 position(
				index & 1u ? box.maximum.x : box.minimum.x,
				index & 2u ? box.maximum.y : box.minimum.y,
				index & 4u ? box.maximum.z : box.minimum.z);
			const auto projected = projection.projectPoint(position);
			// Leave clipped faces to the GPU so coverage never depends on
			// the precision of hardware-generated clip vertices.
			if (!projected || projected->farthest_depth >= 1.0 ||
			    glm::any(glm::lessThan(projected->minimum, glm::dvec2(0.0))) ||
			    glm::any(glm::greaterThan(projected->maximum, viewport_))) {
				return;
			}
			corners[index] = *projected;
		}

		constexpr std::array<std::array<std::size_t, 4>, 6> faces{{
			{{0, 2, 3, 1}}, {{4, 5, 7, 6}},
			{{0, 1, 5, 4}}, {{2, 6, 7, 3}},
			{{0, 4, 6, 2}}, {{1, 3, 7, 5}}
		}};
		for (const auto &face : faces) {
			addFace({corners[face[0]], corners[face[1]], corners[face[2]], corners[face[3]]});
		}
	}

	void finish()
	{
		if (!has_coverage_) return;
		for (int level = 1; level < level_count; ++level) {
			const int size = grid_size >> level;
			const int child_size = size * 2;
			for (int y = 0; y < size; ++y) {
				for (int x = 0; x < size; ++x) {
					const int child = 2 * y * child_size + 2 * x;
					const auto &children = levels_[level - 1];
					levels_[level][y * size + x] = std::max({
						children[child], children[child + 1],
						children[child + child_size], children[child + child_size + 1]
					});
				}
			}
		}
	}

	bool hides(const PreviewScreenBounds &range) const
	{
		if (!has_coverage_ || range.nearest_depth <= 0.0) return false;
		const glm::ivec2 minimum = tileFor(range.minimum);
		const glm::ivec2 maximum = tileFor(range.maximum);
		// A coarser cell may extend beyond the range. Its farthest depth can
		// make the test inconclusive, but cannot make it falsely occluded.
		int level = 0;
		while (level + 1 < level_count &&
		       (((maximum.x >> level) - (minimum.x >> level)) > 3 ||
		        ((maximum.y >> level) - (minimum.y >> level)) > 3)) {
			++level;
		}
		const int size = grid_size >> level;
		for (int y = minimum.y >> level; y <= (maximum.y >> level); ++y) {
			for (int x = minimum.x >> level; x <= (maximum.x >> level); ++x) {
				// Allow both transform/coverage tolerances and the GPU's depth quantization.
				if (levels_[level][y * size + x] + 1.0e-5 >= range.nearest_depth) return false;
			}
		}
		return true;
	}

private:
	static constexpr int grid_size = 32;
	static constexpr int level_count = 6;

	glm::ivec2 tileFor(glm::dvec2 position) const
	{
		return glm::ivec2(glm::clamp(position * double(grid_size) / viewport_,
		                            glm::dvec2(0.0), glm::dvec2(grid_size - 1)));
	}

	void addFace(const std::array<PreviewScreenBounds, 4> &corners)
	{
		std::array<glm::dvec2, 4> vertices;
		glm::dvec2 minimum(viewport_);
		glm::dvec2 maximum(0.0);
		glm::dvec2 center(0.0);
		double depth = 0.0;
		double edge_padding = 1.0;
		for (std::size_t corner = 0; corner < vertices.size(); ++corner) {
			const auto &bounds = corners[corner];
			vertices[corner] = (bounds.minimum + bounds.maximum) * 0.5;
			minimum = glm::min(minimum, vertices[corner]);
			maximum = glm::max(maximum, vertices[corner]);
			center += vertices[corner] * 0.25;
			depth = std::max(depth, bounds.farthest_depth);
			edge_padding = std::max(edge_padding, 1.0 + glm::length(bounds.maximum - bounds.minimum) * 0.5);
		}
		const glm::dvec2 tile_size = viewport_ / double(grid_size);
		if (glm::any(glm::lessThan(maximum - minimum, tile_size))) return;

		std::array<glm::dvec2, 4> inward_normals;
		for (std::size_t edge = 0; edge < vertices.size(); ++edge) {
			const glm::dvec2 direction = vertices[(edge + 1) % vertices.size()] - vertices[edge];
			const double length = glm::length(direction);
			if (length <= 2.0 * edge_padding) return;
			glm::dvec2 normal(-direction.y / length, direction.x / length);
			const double center_distance = glm::dot(normal, center - vertices[edge]);
			if (std::abs(center_distance) <= edge_padding) return;
			inward_normals[edge] = center_distance > 0.0 ? normal : -normal;
		}

		const glm::ivec2 first = tileFor(minimum);
		const glm::ivec2 last = tileFor(maximum);
		for (int y = first.y; y <= last.y; ++y) {
			for (int x = first.x; x <= last.x; ++x) {
				const glm::dvec2 lower = glm::dvec2(x, y) * tile_size;
				bool covered = true;
				for (std::size_t edge = 0; edge < vertices.size(); ++edge) {
					const glm::dvec2 normal = inward_normals[edge];
					const glm::dvec2 test = lower + glm::dvec2(
						normal.x < 0.0 ? tile_size.x : 0.0,
						normal.y < 0.0 ? tile_size.y : 0.0);
					if (glm::dot(normal, test - vertices[edge]) <= edge_padding) {
						covered = false;
						break;
					}
				}
				if (covered) {
					auto &tile = levels_[0][y * grid_size + x];
					tile = std::min(tile, depth);
					has_coverage_ = true;
				}
			}
		}
	}

	glm::dvec2 viewport_;
	std::array<std::array<double, grid_size * grid_size>, level_count> levels_;
	bool has_coverage_ = false;
};
