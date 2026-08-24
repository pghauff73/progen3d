#include "geometry/service/ThreeViewProjectionService.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace {

constexpr float minimum_projection_extent = 1.0e-6f;

glm::vec2 project_vertex(const glm::vec3 &vertex, OrthographicProjectionView view)
{
	switch (view) {
	case OrthographicProjectionView::Side:
		return {-vertex.z, vertex.y};
	case OrthographicProjectionView::Front:
		return {vertex.x, vertex.y};
	case OrthographicProjectionView::Top:
		return {-vertex.z, vertex.x};
	}
	return {vertex.x, vertex.y};
}

float signed_edge(const glm::vec2 &first,
	              const glm::vec2 &second,
	              const glm::vec2 &point)
{
	return (point.x - first.x) * (second.y - first.y) -
	       (point.y - first.y) * (second.x - first.x);
}

class OrthographicSilhouetteRasterizer
{
public:
	OrthographicSilhouetteRasterizer(
		const Mesh &mesh,
		const std::vector<glm::vec3> &world_vertices,
		OrthographicProjectionView view,
		const ThreeViewProjectionConfiguration &configuration)
		: mesh_(mesh),
		  view_(view),
		  silhouette_(configuration.resolution(), configuration.resolution())
	{
		projected_vertices_.reserve(world_vertices.size());
		for (const glm::vec3 &world_vertex : world_vertices) {
			projected_vertices_.push_back(project_vertex(world_vertex, view_));
		}
		calculateProjectionTransform(configuration);
	}

	BinarySilhouette rasterize()
	{
		for (const glm::ivec3 &face : mesh_.faces) rasterizeTriangle(face);
		return std::move(silhouette_);
	}

private:
	void calculateProjectionTransform(
		const ThreeViewProjectionConfiguration &configuration)
	{
		glm::vec2 minimum(std::numeric_limits<float>::max());
		glm::vec2 maximum(std::numeric_limits<float>::lowest());
		if (configuration.envelope()) {
			const glm::vec3 lower = configuration.envelope()->minimum();
			const glm::vec3 upper = configuration.envelope()->maximum();
			for (int x_index = 0; x_index < 2; ++x_index) {
				for (int y_index = 0; y_index < 2; ++y_index) {
					for (int z_index = 0; z_index < 2; ++z_index) {
						const glm::vec2 projected = project_vertex(
							glm::vec3(
								x_index == 0 ? lower.x : upper.x,
								y_index == 0 ? lower.y : upper.y,
								z_index == 0 ? lower.z : upper.z),
							view_);
						minimum = glm::min(minimum, projected);
						maximum = glm::max(maximum, projected);
					}
				}
			}
		}
		else {
			for (const glm::vec2 &vertex : projected_vertices_) {
				minimum = glm::min(minimum, vertex);
				maximum = glm::max(maximum, vertex);
			}
		}
		const glm::vec2 extent = glm::max(
			maximum - minimum,
			glm::vec2(minimum_projection_extent));
		const float image_extent = static_cast<float>(silhouette_.width() - 1u);
		const float padding = std::clamp(configuration.paddingRatio(), 0.0f, 0.45f) * image_extent;
		const float usable_extent = std::max(1.0f, image_extent - 2.0f * padding);
		scale_ = std::min(usable_extent / extent.x, usable_extent / extent.y);
		const glm::vec2 projected_size = extent * scale_;
		offset_.x = (image_extent - projected_size.x) * 0.5f - minimum.x * scale_;
		offset_.y = (image_extent - projected_size.y) * 0.5f + maximum.y * scale_;
	}

	glm::vec2 imagePoint(const glm::vec2 &projected_point) const
	{
		return {
			projected_point.x * scale_ + offset_.x,
			offset_.y - projected_point.y * scale_};
	}

	void rasterizeTriangle(const glm::ivec3 &face)
	{
		const std::array<int, 3> indices = {face.x, face.y, face.z};
		std::array<glm::vec2, 3> points;
		for (std::size_t index = 0u; index < indices.size(); ++index) {
			if (indices[index] < 0 ||
			    static_cast<std::size_t>(indices[index]) >= projected_vertices_.size()) {
				return;
			}
			points[index] = imagePoint(projected_vertices_[static_cast<std::size_t>(indices[index])]);
		}
		const float area = signed_edge(points[0], points[1], points[2]);
		if (std::fabs(area) <= minimum_projection_extent) return;

		const float minimum_x = std::min({points[0].x, points[1].x, points[2].x});
		const float maximum_x = std::max({points[0].x, points[1].x, points[2].x});
		const float minimum_y = std::min({points[0].y, points[1].y, points[2].y});
		const float maximum_y = std::max({points[0].y, points[1].y, points[2].y});
		const int first_x = std::max(0, static_cast<int>(std::floor(minimum_x)));
		const int last_x = std::min(
			static_cast<int>(silhouette_.width()) - 1,
			static_cast<int>(std::ceil(maximum_x)));
		const int first_y = std::max(0, static_cast<int>(std::floor(minimum_y)));
		const int last_y = std::min(
			static_cast<int>(silhouette_.height()) - 1,
			static_cast<int>(std::ceil(maximum_y)));

		for (int y = first_y; y <= last_y; ++y) {
			for (int x = first_x; x <= last_x; ++x) {
				const glm::vec2 sample(
					static_cast<float>(x) + 0.5f,
					static_cast<float>(y) + 0.5f);
				const float first_edge = signed_edge(points[0], points[1], sample);
				const float second_edge = signed_edge(points[1], points[2], sample);
				const float third_edge = signed_edge(points[2], points[0], sample);
				const bool has_negative = first_edge < 0.0f || second_edge < 0.0f ||
				                          third_edge < 0.0f;
				const bool has_positive = first_edge > 0.0f || second_edge > 0.0f ||
				                          third_edge > 0.0f;
				if (!(has_negative && has_positive)) {
					silhouette_.occupy(
						static_cast<std::size_t>(x),
						static_cast<std::size_t>(y));
				}
			}
		}
	}

	const Mesh &mesh_;
	OrthographicProjectionView view_ = OrthographicProjectionView::Side;
	std::vector<glm::vec2> projected_vertices_;
	BinarySilhouette silhouette_;
	float scale_ = 1.0f;
	glm::vec2 offset_{0.0f};
};

BinarySilhouette rasterize_view(
	const Mesh &mesh,
	const std::vector<glm::vec3> &world_vertices,
	OrthographicProjectionView view,
	const ThreeViewProjectionConfiguration &configuration)
{
	return OrthographicSilhouetteRasterizer(
		mesh, world_vertices, view, configuration).rasterize();
}

} // namespace

ThreeViewProjection ThreeViewProjectionService::project(
	const Mesh &mesh,
	const glm::mat4 &world_transform,
	const ThreeViewProjectionConfiguration &configuration) const
{
	std::vector<glm::vec3> world_vertices;
	world_vertices.reserve(mesh.vertices.size());
	for (const glm::vec3 &vertex : mesh.vertices) {
		world_vertices.emplace_back(world_transform * glm::vec4(vertex, 1.0f));
	}
	return ThreeViewProjection(
		rasterize_view(mesh, world_vertices, OrthographicProjectionView::Side, configuration),
		rasterize_view(mesh, world_vertices, OrthographicProjectionView::Front, configuration),
		rasterize_view(mesh, world_vertices, OrthographicProjectionView::Top, configuration));
}
