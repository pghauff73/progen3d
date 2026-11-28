#pragma once

#include "imgui_render.h"

class SceneGenerationContext;

class PreviewGeometryUploadService
{
public:
	static void uploadStatic(const SceneGenerationContext *context,
	                         const std::vector<PreviewMaterial> &materials);
	static void uploadDynamic(const SceneGenerationContext *context,
	                          std::size_t material_count);
	// A physics timeline with fixed grammar geometry can update its baked movable vertices in place.
	void uploadPhysics(const SceneGenerationContext &context, const std::vector<PreviewMaterial> &materials);

private:
	struct MovableVertexRange {
		std::size_t instance_index;
		std::size_t batch_index;
		std::size_t float_offset;
		std::size_t float_count;
		bool transparent;
	};

	void uploadBakedGeometry(const SceneGenerationContext *context,
	                         const std::vector<PreviewMaterial> &materials, bool track_movable);
	bool updateMovableGeometry(const SceneGenerationContext &context) const;

	bool has_physics_layout_ = false;
	std::vector<bool> removed_instances_;
	std::vector<MovableVertexRange> movable_ranges_;
};
