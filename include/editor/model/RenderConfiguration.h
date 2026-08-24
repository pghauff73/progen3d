#pragma once

class RenderConfiguration
{
public:
	bool lens_flare_enabled = false;
	bool shadows_enabled = true;
	int anti_aliasing_level = 0;
	bool cubemap_enabled = false;
	unsigned int cubemap_texture = 0;
	int cubemap_resolution = 512;
	float cubemap_intensity = 1.0f;
	bool bloom_enabled = false;
	float bloom_threshold = 1.0f;
	float bloom_intensity = 0.5f;
	bool ambient_occlusion_enabled = false;
	float exposure = 1.0f;
	float gamma = 2.2f;
	int material_texture_resolution = 512;
	bool changed = true;
};
