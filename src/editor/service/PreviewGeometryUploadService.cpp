#include "editor/service/PreviewGeometryUploadService.h"

#include "Context.h"

#include <cmath>
#include <utility>

namespace {

constexpr std::size_t kPreviewVertexStride = 8;
constexpr GLsizei kCubeVertexCount = 36;

glm::vec3 packed_vertex_position(const GLfloat *vertex)
{
	return {vertex[0], vertex[1], vertex[2]};
}

bool uses_undeformed_cube_template(const PrimitiveInstance &instance)
{
	if (!instance.immovable || instance.type != "Cube" || !instance.resolved_geometry ||
	    instance.resolved_geometry->hasTriangleMesh() ||
	    !instance.resolved_geometry->usesCubeVertexTemplate()) {
		return false;
	}

	// Even the default Cube can transform opposite halves separately and warp individual faces.
	const auto &transform = instance.primary_transform;
	if (transform != instance.secondary_transform || transform[0].w != 0.0f ||
	    transform[1].w != 0.0f || transform[2].w != 0.0f || transform[3].w != 1.0f) {
		return false;
	}
	for (std::size_t axis = 0; axis < instance.dual_scales.size(); ++axis) {
		if (instance.dual_scales[axis] != glm::vec3(1.0f) ||
		    instance.dual_translations[axis] != glm::vec3(0.0f)) {
			return false;
		}
	}
	const double determinant = glm::determinant(glm::dmat3(transform));
	return std::isfinite(determinant) && determinant > 0.0;
}

bool matches_outward_cube_winding(const GLfloat *vertices, const PreviewOpaqueDrawRange &range)
{
	const glm::dvec3 center = (glm::dvec3(range.minimum) + glm::dvec3(range.maximum)) * 0.5;
	for (GLsizei triangle = 0; triangle < kCubeVertexCount; triangle += 3) {
		const GLfloat *packed_triangle = vertices + triangle * kPreviewVertexStride;
		const glm::dvec3 first(packed_vertex_position(packed_triangle));
		const glm::dvec3 second(packed_vertex_position(packed_triangle + kPreviewVertexStride));
		const glm::dvec3 third(packed_vertex_position(packed_triangle + 2 * kPreviewVertexStride));
		if (!(glm::dot(glm::cross(second - first, third - first), first - center) > 0.0)) {
			return false;
		}
	}
	return true;
}

bool matches_axis_aligned_cube(const glm::mat4 &transform, const GLfloat *vertices,
                              const PreviewOpaqueDrawRange &range)
{
	if (transform[0].y != 0.0f || transform[0].z != 0.0f ||
	    transform[1].x != 0.0f || transform[1].z != 0.0f ||
	    transform[2].x != 0.0f || transform[2].y != 0.0f) {
		return false;
	}
	for (GLsizei vertex = 0; vertex < kCubeVertexCount; ++vertex) {
		const glm::vec3 position = packed_vertex_position(vertices + vertex * kPreviewVertexStride);
		for (int axis = 0; axis < 3; ++axis) {
			if (!(range.minimum[axis] < range.maximum[axis]) ||
			    (position[axis] != range.minimum[axis] && position[axis] != range.maximum[axis])) {
				return false;
			}
		}
	}
	return true;
}

PreviewOpaqueDrawRange describe_opaque_draw_range(const PrimitiveInstance &instance, const GLfloat *vertices,
                                                std::size_t float_count, std::size_t batch_float_offset)
{
	PreviewOpaqueDrawRange range;
	range.first_vertex = static_cast<GLint>(batch_float_offset / kPreviewVertexStride);
	range.vertex_count = static_cast<GLsizei>(float_count / kPreviewVertexStride);
	if (range.vertex_count <= 0 || float_count % kPreviewVertexStride != 0) return range;

	range.minimum = packed_vertex_position(vertices);
	range.maximum = range.minimum;
	for (std::size_t offset = 0; offset < float_count; offset += kPreviewVertexStride) {
		const glm::vec3 position = packed_vertex_position(vertices + offset);
		if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z)) {
			return range;
		}
		range.minimum = glm::min(range.minimum, position);
		range.maximum = glm::max(range.maximum, position);
	}
	range.bounds_valid = true;
	if (range.vertex_count == kCubeVertexCount && uses_undeformed_cube_template(instance)) {
		range.outward_wound_opaque_cube = matches_outward_cube_winding(vertices, range);
		range.axis_aligned_cube = range.outward_wound_opaque_cube &&
			matches_axis_aligned_cube(instance.primary_transform, vertices, range);
	}
	return range;
}

bool is_preview_dynamic_cube_instance(const PrimitiveInstance &instance)
{
	return !instance.removed &&
	       !instance.immovable &&
	       (instance.type == "Cube" ||
	        instance.type == "CubeX" ||
	        instance.type == "CubeY" ||
	        instance.type == "CubeZ");
}

PreviewDynamicCubeMode preview_dynamic_cube_mode_for_instance(const PrimitiveInstance &instance)
{
	if (instance.type == "CubeX") {
		return PreviewDynamicCubeMode::CubeX;
	}
	if (instance.type == "CubeY") {
		return PreviewDynamicCubeMode::CubeY;
	}
	if (instance.type == "CubeZ") {
		return PreviewDynamicCubeMode::CubeZ;
	}
	return PreviewDynamicCubeMode::Default;
}

}

void PreviewGeometryUploadService::uploadStatic(
	const Context *context, const std::vector<PreviewMaterial> &materials)
{
	PreviewGeometryUploadService().uploadBakedGeometry(context, materials, false);
}

void PreviewGeometryUploadService::uploadBakedGeometry(
	const Context *context, const std::vector<PreviewMaterial> &materials, bool track_movable)
{
	if (context == nullptr) {
		upload_render_buffers(std::vector<std::vector<GLfloat>>{},
		                      std::vector<int>{},
		                      materials.size());
		upload_transparent_render_batches(std::vector<PreviewTransparentBatch>{});
		return;
	}

	const auto base_vertices = build_base_vertex_data();
	std::vector<std::vector<GLfloat>> buffers(materials.size());
	std::vector<std::vector<PreviewOpaqueDrawRange>> opaque_ranges(materials.size());
	std::vector<int> counts(materials.size(), 0);
	std::vector<PreviewTransparentBatch> transparent_batches;
	transparent_batches.reserve(context->primitive_instances.size());
	removed_instances_.clear();
	movable_ranges_.clear();
	if (track_movable) removed_instances_.reserve(context->primitive_instances.size());
	for (std::size_t index = 0; index < context->primitive_instances.size(); ++index) {
		const PrimitiveInstance &instance = context->primitive_instances[index];
		if (track_movable) removed_instances_.push_back(instance.removed);
		if (instance.removed ||
		    is_preview_dynamic_cube_instance(instance) ||
		    instance.material_index < 0) {
			continue;
		}
		const std::size_t material_index = static_cast<std::size_t>(instance.material_index);
		if (material_index >= materials.size()) {
			continue;
		}
		if (materials[material_index].opacity < 0.999f) {
			PreviewTransparentBatch batch;
			batch.material_index = instance.material_index;
			batch.center = context->getInstanceCenter(instance);
			context->buildInstanceBuffer(instance, base_vertices.data(), &batch.vertices);
			if (!batch.vertices.empty()) {
				if (track_movable && !instance.immovable) {
					movable_ranges_.push_back({index, transparent_batches.size(), 0, batch.vertices.size(), true});
				}
				transparent_batches.push_back(std::move(batch));
			}
			continue;
		}

		auto &buffer = buffers[material_index];
		const std::size_t offset = buffer.size();
		context->buildInstanceBuffer(instance, base_vertices.data(), &buffer);
		const std::size_t float_count = buffer.size() - offset;
		if (float_count != 0) {
			opaque_ranges[material_index].push_back(
				describe_opaque_draw_range(instance, buffer.data() + offset, float_count, offset));
			if (track_movable && !instance.immovable) {
				movable_ranges_.push_back({index, material_index, offset, float_count, false});
			}
		}
	}

	for (std::size_t material_index = 0; material_index < buffers.size(); ++material_index) {
		counts[material_index] = static_cast<int>(buffers[material_index].size() / kPreviewVertexStride);
	}
	upload_render_buffers(buffers, counts, materials.size(), opaque_ranges);
	upload_transparent_render_batches(transparent_batches);
	has_physics_layout_ = track_movable;
}

bool PreviewGeometryUploadService::updateMovableGeometry(const Context &context) const
{
	const auto &instances = context.primitive_instances;
	if (!has_physics_layout_ || instances.size() != removed_instances_.size()) return false;
	for (std::size_t index = 0; index < instances.size(); ++index) {
		if (instances[index].removed != removed_instances_[index]) return false;
	}
	const auto base_vertices = build_base_vertex_data();
	std::vector<GLfloat> vertices;
	PreviewRenderer &renderer = preview_renderer();
	for (const MovableVertexRange &range : movable_ranges_) {
		const auto &instance = instances[range.instance_index];
		vertices.clear();
		context.buildInstanceBuffer(instance, base_vertices.data(), &vertices);
		if (vertices.size() != range.float_count) return false;
		if (range.transparent) {
			renderer.updateTransparentBatch(range.batch_index, context.getInstanceCenter(instance), vertices);
		} else {
			renderer.updateOpaqueBatch(range.batch_index, range.float_offset, vertices,
				describe_opaque_draw_range(instance, vertices.data(), vertices.size(), range.float_offset));
		}
	}
	return true;
}

void PreviewGeometryUploadService::uploadPhysics(
	const Context &context, const std::vector<PreviewMaterial> &materials)
{
	if (!updateMovableGeometry(context)) uploadBakedGeometry(&context, materials, true);
	uploadDynamic(&context, materials.size());
}

void PreviewGeometryUploadService::uploadDynamic(const Context *context,
                                                  std::size_t material_count)
{
	std::vector<PreviewDynamicCubeDraw> draws;
	if (context == nullptr) {
		upload_dynamic_cube_draws(draws);
		return;
	}

	draws.reserve(context->primitive_instances.size());
	for (const PrimitiveInstance &instance : context->primitive_instances) {
		if (!is_preview_dynamic_cube_instance(instance) || instance.material_index < 0) {
			continue;
		}
		const std::size_t material_index = static_cast<std::size_t>(instance.material_index);
		if (material_index >= material_count) {
			continue;
		}

		PreviewDynamicCubeDraw draw;
		draw.previous_primary_transform =
			instance.has_previous_render_state ? instance.previous_primary_transform : instance.primary_transform;
		draw.previous_secondary_transform =
			instance.has_previous_render_state ? instance.previous_secondary_transform : instance.secondary_transform;
		draw.current_primary_transform = instance.primary_transform;
		draw.current_secondary_transform = instance.secondary_transform;
		draw.previous_center =
			instance.has_previous_render_state ? instance.previous_bounds_center : context->getInstanceCenter(instance);
		draw.current_center = context->getInstanceCenter(instance);
		draw.dual_scales = instance.dual_scales;
		draw.dual_translations = instance.dual_translations;
		draw.texscale = instance.texscale;
		draw.material_index = instance.material_index;
		draw.mode = preview_dynamic_cube_mode_for_instance(instance);
		draws.push_back(std::move(draw));
	}
	upload_dynamic_cube_draws(draws);
}
