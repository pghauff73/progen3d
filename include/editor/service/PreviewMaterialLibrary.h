#pragma once

#include "imgui_render.h"

#include <string_view>

class RenderRowExecutor;

namespace preview_materials {

struct MaterialSwatch {
	float r = 1.0f;
	float g = 1.0f;
	float b = 1.0f;
};

enum class MaterialLexemeKind {
	Color,
	Family,
	Opacity,
	Finish
};

struct MaterialLexeme {
	std::string_view token;
	MaterialLexemeKind kind = MaterialLexemeKind::Color;
	MaterialSwatch swatch{};
};

const std::array<MaterialLexeme, 78> &material_lexemes();
GLuint upload_rgba_texture(const unsigned char *pixels, int width, int height, bool color_texture = false);
PreviewMaterial generate_procedural_material(const std::string &material_name, int resolution,
                                             RenderRowExecutor *rows = nullptr,
                                             PreviewRenderMode mode = PreviewRenderMode::Full);
bool is_user_texture_slot_name(std::string_view material_name);
void auto_generate_missing_texture_maps(PreviewMaterial *material,
                                        PreviewRenderMode mode = PreviewRenderMode::Full);
void destroy_preview_material_textures(PreviewMaterial *material);

}
