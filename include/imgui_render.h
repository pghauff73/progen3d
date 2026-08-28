#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "ProGen3dGl.h"
#include "lighting/model/PreviewLightCollection.h"
#include <glm/glm.hpp>

class PreviewRenderData {
public:
	virtual ~PreviewRenderData() = default;
};

class PreviewSurfaceMaterial : public PreviewRenderData {
public:
	GLuint texture = 0;
	GLuint height_texture = 0;
	GLuint emissive_texture = 0;
	GLuint alpha_texture = 0;
	GLuint glow_texture = 0;
	GLuint normal_texture = 0;
	GLuint roughness_texture = 0;
	GLuint metallic_texture = 0;
	GLuint ao_texture = 0;
	std::string name;
	std::string family;
	std::string description;
	float opacity = 1.0f;
	float reflectance = 0.25f;
	float smoothness = 0.5f;
	float height_scale = 0.04f;
	float emission_strength = 0.0f;
	float uv_rotation_speed = 0.0f;
	float uv_activity = 0.0f;
	float pulse_strength = 0.0f;
	float metallic = 0.0f;
	float roughness = 0.5f;
	float ao = 1.0f;
	float subsurface = 0.0f;
	float anisotropic = 0.0f;
	float sheen = 0.0f;
	float clearcoat = 0.0f;
	float clearcoat_roughness = 0.0f;
	float normal_strength = 1.0f;
	float transmission = 0.0f;
	float index_of_refraction = 1.5f;
	float thickness = 0.01f;
	glm::vec3 attenuation_color{1.0f, 1.0f, 1.0f};
	int texture_resolution = 1;
	std::array<glm::vec3, 3> swatches{
		glm::vec3(0.88f, 0.89f, 0.92f),
		glm::vec3(0.68f, 0.70f, 0.74f),
		glm::vec3(0.96f, 0.97f, 0.99f)};
	int mapping_mode = 0;
	float mapping_scale = 0.25f;
	std::array<glm::vec3, 3> projection_axes{
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f)};
};

using PreviewMaterial = PreviewSurfaceMaterial;

enum class PreviewMappingMode : int {
	UV = 0,
	Triplanar = 1,
};

enum class PreviewTextureDebugView : int {
	Shaded = 0,
	UVs = 1,
	Checker = 2,
	TriplanarWeights = 3,
};

struct PreviewOverlayVertex {
	GLfloat x = 0.0f;
	GLfloat y = 0.0f;
	GLfloat z = 0.0f;
	GLfloat r = 1.0f;
	GLfloat g = 1.0f;
	GLfloat b = 1.0f;
	GLfloat a = 1.0f;
};

class PreviewVertexBatch : public PreviewRenderData {
public:
	virtual ~PreviewVertexBatch() = default;
	std::vector<GLfloat> vertices;

	std::size_t vertexCount() const
	{
		return vertices.size() / 8u;
	}
};

class PreviewTransparentBatch : public PreviewVertexBatch {
public:
	glm::vec3 center{0.0f};
	int material_index = -1;
};

class PreviewOutlineBatch : public PreviewVertexBatch {
public:
	glm::vec4 color{1.0f};
	float width = 0.03f;
};

enum class PreviewDynamicCubeMode : int {
	Default = 0,
	CubeX = 1,
	CubeY = 2,
	CubeZ = 3,
};

struct PreviewDynamicCubeDraw {
	glm::mat4 previous_primary_transform{1.0f};
	glm::mat4 previous_secondary_transform{1.0f};
	glm::mat4 current_primary_transform{1.0f};
	glm::mat4 current_secondary_transform{1.0f};
	glm::vec3 previous_center{0.0f};
	glm::vec3 current_center{0.0f};
	std::array<glm::vec3, 3> dual_scales{glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f)};
	std::array<glm::vec3, 3> dual_translations{glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)};
	float texscale = 0.125f;
	int material_index = -1;
	PreviewDynamicCubeMode mode = PreviewDynamicCubeMode::Default;
};

class PreviewCameraState : public PreviewRenderData {
public:
	int width = 1;
	int height = 1;
	float scale_global = 1.0f;
	float camera_distance = 5.0f;
	float angle_view = 0.0f;
	float elevation_view = 0.0f;
	float roll_view = 0.0f;
	float vertical_field_of_view_radians = 0.0f;
	glm::vec3 target{0.0f};
};

class PreviewRenderSettings : public PreviewRenderData {
public:
	float simulation_alpha = 1.0f;
	float preview_time = 0.0f;
	PreviewTextureDebugView debug_view = PreviewTextureDebugView::Shaded;
	int mapping_mode_override = -1;
	bool lens_flare = false;
	bool shadows = true;
	int anti_aliasing = 0;
	bool cubemap_enabled = false;
	GLuint cubemap_texture = 0;
	float cubemap_intensity = 1.0f;
	bool bloom_enabled = false;
	float bloom_threshold = 1.0f;
	float bloom_intensity = 0.5f;
	bool ambient_occlusion = false;
	float exposure = 1.0f;
	float gamma = 2.2f;
};

class PreviewCubemapGenerator {
public:
	virtual ~PreviewCubemapGenerator() = default;
	virtual GLuint generateSkyCubemap(int resolution, float intensity) = 0;
	virtual GLuint generateEnhancedSkyCubemap(int resolution, float intensity) = 0;
};

class PreviewRenderer {
public:
	virtual ~PreviewRenderer() = default;
	virtual bool initialize() = 0;
	virtual void shutdown() = 0;
	virtual void uploadOpaqueBatches(const std::vector<std::vector<GLfloat>> &buffers,
	                                 const std::vector<int> &counts,
	                                 std::size_t material_capacity) = 0;
	virtual void uploadTransparentBatches(const std::vector<PreviewTransparentBatch> &batches) = 0;
	virtual void uploadDynamicCubeDraws(const std::vector<PreviewDynamicCubeDraw> &draws) = 0;
	virtual void uploadOverlayLines(const std::vector<PreviewOverlayVertex> &vertices) = 0;
	virtual void uploadOutlineBatches(const std::vector<PreviewOutlineBatch> &batches) = 0;
	virtual void render(const PreviewCameraState &camera,
	                    const PreviewRenderSettings &settings,
	                    const std::vector<PreviewMaterial> &materials,
	                    const PreviewLightCollection &lights) = 0;
	virtual GLuint previewTextureId() const = 0;
};

PreviewRenderer &preview_renderer();
PreviewCubemapGenerator &preview_cubemap_generator();

std::array<GLfloat, 36 * 8> build_base_vertex_data();
bool initialize_renderer();
void shutdown_renderer();
void upload_render_buffers(const std::vector<std::vector<GLfloat>> &buffers,
                           const std::vector<int> &counts,
                           std::size_t material_capacity);
void upload_transparent_render_batches(const std::vector<PreviewTransparentBatch> &batches);
void upload_dynamic_cube_draws(const std::vector<PreviewDynamicCubeDraw> &draws);
void upload_overlay_lines(const std::vector<PreviewOverlayVertex> &vertices);
void upload_outline_batches(const std::vector<PreviewOutlineBatch> &batches);
void render_scene_to_preview(int width,
                             int height,
                             const std::vector<PreviewMaterial> &materials,
	                         const PreviewLightCollection &lights,
                             float simulation_alpha,
                             float preview_time,
                             float scale_global,
	                             float camera_distance,
	                             float angle_view,
	                             float elevation_view,
	                             float roll_view,
	                             float target_x,
	                             float target_y,
	                             float target_z,
	                             float vertical_field_of_view_radians,
	                             PreviewTextureDebugView debug_view = PreviewTextureDebugView::Shaded,
                             int mapping_mode_override = -1,
                             bool lens_flare = false,
                             bool shadows = true,
                             int anti_aliasing = 0,
                             bool cubemap_enabled = false,
                             GLuint cubemap_texture = 0,
                             float cubemap_intensity = 1.0f,
                             bool bloom_enabled = false,
                             float bloom_threshold = 1.0f,
                             float bloom_intensity = 0.5f,
                             bool ambient_occlusion = false,
                             float exposure = 1.0f,
                             float gamma = 2.2f);
GLuint generate_procedural_cubemap(int resolution, float intensity);
GLuint generate_enhanced_procedural_cubemap(int resolution, float intensity);
GLuint preview_texture_id();
bool read_preview_rgb_pixels(std::vector<std::uint8_t> *pixels,
	                         int *width,
	                         int *height);
