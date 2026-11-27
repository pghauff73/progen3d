#include "editor/service/PreviewGeometryUploadService.h"

#include "Context.h"

#include <utility>

namespace {

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
	if (context == nullptr) {
		upload_render_buffers(std::vector<std::vector<GLfloat>>{},
		                      std::vector<int>{},
		                      materials.size());
		upload_transparent_render_batches(std::vector<PreviewTransparentBatch>{});
		return;
	}

	const auto base_vertices = build_base_vertex_data();
	std::vector<std::vector<GLfloat>> buffers(materials.size());
	std::vector<int> counts(materials.size(), 0);
	std::vector<PreviewTransparentBatch> transparent_batches;
	transparent_batches.reserve(context->primitive_instances.size());
	for (const PrimitiveInstance &instance : context->primitive_instances) {
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
				transparent_batches.push_back(std::move(batch));
			}
			continue;
		}

		context->buildInstanceBuffer(instance, base_vertices.data(), &buffers[material_index]);
	}

	for (std::size_t material_index = 0; material_index < buffers.size(); ++material_index) {
		counts[material_index] = static_cast<int>(buffers[material_index].size() / 8u);
	}
	upload_render_buffers(buffers, counts, materials.size());
	upload_transparent_render_batches(transparent_batches);
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
