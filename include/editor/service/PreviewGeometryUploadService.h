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
};
