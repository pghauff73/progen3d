#include "geometry/service/CurvedPanelMeshGenerator.h"

#include "geometry/model/CurvedPanelShapeSpecification.h"

#include <glm/geometric.hpp>

#include <memory>
#include <utility>

namespace {

class CurvedPanelMeshConstruction
{
public:
	CurvedPanelMeshConstruction(
		const CurvedPanelShapeSpecification &specification,
		const GeometryComplexityLimits &complexity_limits)
		: specification_(specification), complexity_limits_(complexity_limits),
		  mesh_(std::make_shared<Mesh>())
	{
	}

	GeometryBuildResult build()
	{
		const int x_count = specification_.horizontalSegments() + 1;
		const int y_count = specification_.verticalSegments() + 1;
		const int layer_count = specification_.thickness() > 0.0f ? 2 : 1;
		const std::size_t vertex_count =
			static_cast<std::size_t>(x_count * y_count * layer_count);
		const std::size_t front_back_triangles =
			static_cast<std::size_t>(specification_.horizontalSegments()) *
			static_cast<std::size_t>(specification_.verticalSegments()) * 2u *
			static_cast<std::size_t>(layer_count);
		const std::size_t rim_triangles = specification_.thickness() > 0.0f
			? static_cast<std::size_t>(
				specification_.horizontalSegments() +
				specification_.verticalSegments()) * 4u
			: 0u;
		if (vertex_count > complexity_limits_.maximumGeneratedVertices() ||
		    front_back_triangles + rim_triangles >
		        complexity_limits_.maximumGeneratedTriangles()) {
			return GeometryBuildResult::createFailure(
				GeometryBuildStatus::TriangleLimitExceeded,
				"CurvedPanel exceeds the configured generated mesh limits.");
		}

		mesh_->vertices.reserve(vertex_count);
		mesh_->normals.reserve(vertex_count);
		mesh_->texcoords.reserve(vertex_count);
		for (int layer = 0; layer < layer_count; ++layer) {
			const float layer_sign = layer == 0 ? 1.0f : -1.0f;
			for (int y_index = 0; y_index < y_count; ++y_index) {
				const float v = static_cast<float>(y_index) /
				                static_cast<float>(specification_.verticalSegments());
				const float y = (v - 0.5f) * specification_.height();
				for (int x_index = 0; x_index < x_count; ++x_index) {
					const float u = static_cast<float>(x_index) /
					                static_cast<float>(specification_.horizontalSegments());
					const float x = (u - 0.5f) * specification_.width();
					const float normalized_x = 2.0f * x / specification_.width();
					const float normalized_y = 2.0f * y / specification_.height();
					const float z =
						specification_.horizontalCurvature() *
							(1.0f - normalized_x * normalized_x) +
						specification_.verticalCurvature() *
							(1.0f - normalized_y * normalized_y);
					const float dz_dx = -4.0f *
						specification_.horizontalCurvature() * x /
						(specification_.width() * specification_.width());
					const float dz_dy = -4.0f *
						specification_.verticalCurvature() * y /
						(specification_.height() * specification_.height());
					const glm::vec3 surface_normal = glm::normalize(
						glm::vec3(-dz_dx, -dz_dy, 1.0f));
					const glm::vec3 normal = surface_normal * layer_sign;
					const float offset = specification_.thickness() * 0.5f * layer_sign;
					mesh_->vertices.push_back(
						glm::vec3(x, y, z) + surface_normal * offset);
					mesh_->normals.push_back(normal);
					mesh_->texcoords.emplace_back(u, v, 0.0f);
				}
			}
		}

		add_grid_faces(0, false, MeshSurfaceRole::Outer);
		if (layer_count == 2) {
			const int back_offset = x_count * y_count;
			add_grid_faces(back_offset, true, MeshSurfaceRole::Inner);
			add_rim_faces(back_offset);
		}
		mesh_->buildCollisionAccel();
		return GeometryBuildResult::createSuccess(
			GeneratedPrimitiveMesh(mesh_, std::move(tags_)));
	}

private:
	int index(int x, int y, int offset = 0) const
	{
		return offset + y * (specification_.horizontalSegments() + 1) + x;
	}

	void add_triangle(int first, int second, int third, MeshSurfaceRole role)
	{
		mesh_->faces.emplace_back(first, second, third);
		tags_.emplace_back(role, 0u);
	}

	void add_grid_faces(int offset, bool reverse, MeshSurfaceRole role)
	{
		for (int y = 0; y < specification_.verticalSegments(); ++y) {
			for (int x = 0; x < specification_.horizontalSegments(); ++x) {
				const int lower_left = index(x, y, offset);
				const int lower_right = index(x + 1, y, offset);
				const int upper_right = index(x + 1, y + 1, offset);
				const int upper_left = index(x, y + 1, offset);
				if (reverse) {
					add_triangle(lower_left, upper_right, lower_right, role);
					add_triangle(lower_left, upper_left, upper_right, role);
				}
				else {
					add_triangle(lower_left, lower_right, upper_right, role);
					add_triangle(lower_left, upper_right, upper_left, role);
				}
			}
		}
	}

	void add_quad(int outer_first, int outer_second,
	              int inner_first, int inner_second)
	{
		add_triangle(outer_first, inner_first, inner_second, MeshSurfaceRole::Rim);
		add_triangle(outer_first, inner_second, outer_second, MeshSurfaceRole::Rim);
	}

	void add_rim_faces(int back_offset)
	{
		for (int x = 0; x < specification_.horizontalSegments(); ++x) {
			add_quad(index(x, 0), index(x + 1, 0),
			         index(x, 0, back_offset), index(x + 1, 0, back_offset));
			add_quad(index(x + 1, specification_.verticalSegments()),
			         index(x, specification_.verticalSegments()),
			         index(x + 1, specification_.verticalSegments(), back_offset),
			         index(x, specification_.verticalSegments(), back_offset));
		}
		for (int y = 0; y < specification_.verticalSegments(); ++y) {
			add_quad(index(0, y + 1), index(0, y),
			         index(0, y + 1, back_offset), index(0, y, back_offset));
			add_quad(index(specification_.horizontalSegments(), y),
			         index(specification_.horizontalSegments(), y + 1),
			         index(specification_.horizontalSegments(), y, back_offset),
			         index(specification_.horizontalSegments(), y + 1, back_offset));
		}
	}

	const CurvedPanelShapeSpecification &specification_;
	const GeometryComplexityLimits &complexity_limits_;
	std::shared_ptr<Mesh> mesh_;
	std::vector<MeshSurfaceTag> tags_;
};

}

GeometryBuildResult CurvedPanelMeshGenerator::build(
	const ShapeSpecification &specification) const
{
	const auto *curved_panel =
		dynamic_cast<const CurvedPanelShapeSpecification *>(&specification);
	if (curved_panel == nullptr) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::InvalidSpecification,
			"CurvedPanelMeshGenerator requires a CurvedPanelShapeSpecification.");
	}
	return CurvedPanelMeshConstruction(*curved_panel, complexity_limits_).build();
}

GeneratedPrimitiveMesh CurvedPanelMeshGenerator::generate(
	const ShapeSpecification &specification,
	std::string *diagnostic) const
{
	const GeometryBuildResult result = build(specification);
	if (!result.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = result.firstDiagnostic();
		return GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {});
	}
	return result.generatedMesh();
}
