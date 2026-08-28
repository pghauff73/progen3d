#include "imgui_render.h"
#include "lighting/rendering/PreviewLightBuffer.h"
#include "lighting/service/PreviewLightingFrameFactory.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using glm::lookAt;
using glm::mat4;
using glm::perspective;
using glm::scale;
using glm::vec3;
using glm::vec4;

void debugout(const std::string &message);

namespace imgui_render_internal {

struct CameraMatrices {
	mat4 view{1.0f};
	mat4 projection{1.0f};
	vec3 position{0.0f, 0.0f, 0.0f};
	vec3 target{0.0f, 0.0f, 0.0f};
	float orbit_distance = 5.0f;
};

class PreviewShaderProgramLibrary {
public:
	GLuint opaque_program = 0;
	GLuint dynamic_cube_program = 0;
	GLuint grid_program = 0;
	GLuint overlay_program = 0;
	GLuint background_program = 0;
	GLuint outline_program = 0;
	GLuint shadow_program = 0;
	GLuint dynamic_cube_shadow_program = 0;
};

class PreviewGeometryRepository {
public:
	GLuint base_vao = 0;
	GLuint base_buffer = 0;
	std::vector<GLuint> opaque_vaos;
	std::vector<GLuint> opaque_buffers;
	std::vector<int> opaque_render_counts;
	std::vector<PreviewDynamicCubeDraw> dynamic_cube_draws;
	std::vector<GLuint> transparent_vaos;
	std::vector<GLuint> transparent_buffers;
	std::vector<GLsizei> transparent_counts;
	std::vector<int> transparent_material_indices;
	std::vector<vec3> transparent_centers;
	GLuint grid_vao = 0;
	GLuint grid_buffer = 0;
	GLsizei grid_vertex_count = 0;
	GLuint overlay_vao = 0;
	GLuint overlay_buffer = 0;
	GLsizei overlay_vertex_count = 0;
	GLuint background_vao = 0;
	std::vector<GLuint> outline_vaos;
	std::vector<GLuint> outline_buffers;
	std::vector<GLsizei> outline_counts;
	std::vector<vec4> outline_colors;
	std::vector<float> outline_widths;
};

class PreviewRenderTargetRepository {
public:
	GLuint shadow_fbo = 0;
	GLuint shadow_depth_texture = 0;
	GLuint preview_fbo = 0;
	GLuint preview_resolve_fbo = 0;
	GLuint preview_color_ms = 0;
	GLuint preview_color = 0;
	GLuint preview_depth = 0;
	int preview_width = 0;
	int preview_height = 0;
};

class PreviewRenderRuntimeEnvironment {
public:
	PreviewShaderProgramLibrary shader_programs;
	PreviewGeometryRepository geometry_repository;
	PreviewRenderTargetRepository render_targets;
	PreviewLightBuffer light_buffer;
};

struct PreviewFrameRequest {
	PreviewCameraState camera;
	PreviewRenderSettings settings;
	const std::vector<PreviewMaterial> *materials = nullptr;
	const PreviewLightCollection *lights = nullptr;
};

vec3 rotate_around_axis(const vec3 &vector, const vec3 &axis, float radians)
{
	const vec3 normalized_axis = glm::normalize(axis);
	const float cosine = std::cos(radians);
	const float sine = std::sin(radians);
	return vector * cosine +
	       glm::cross(normalized_axis, vector) * sine +
	       normalized_axis * glm::dot(normalized_axis, vector) * (1.0f - cosine);
}

const GLchar *VERTEX_SOURCE =
"#version 460 core\n"
"in vec3 position;\n"
"in vec3 normal;\n"
"in vec2 texture;\n"
"out vec3 v_world_normal;\n"
"out vec3 v_world_pos;\n"
"out vec2 v_tex_coord;\n"
"out vec4 v_shadow_pos;\n"
"uniform mat4 projection;\n"
"uniform mat4 view;\n"
"uniform mat4 model;\n"
"uniform mat4 light_view_projection;\n"
"void main(){\n"
"    v_tex_coord = texture;\n"
"    vec4 world_pos = model * vec4(position, 1.0);\n"
"    mat3 normal_matrix = transpose(inverse(mat3(model)));\n"
"    v_world_normal = normalize(normal_matrix * normal);\n"
"    v_world_pos = world_pos.xyz;\n"
"    v_shadow_pos = light_view_projection * world_pos;\n"
"    gl_Position = projection * view * world_pos;\n"
"}\n";

const GLchar *FRAGMENT_SOURCE =
"#version 460 core\n"
"in vec3 v_world_normal;\n"
"in vec3 v_world_pos;\n"
"in vec2 v_tex_coord;\n"
"in vec4 v_shadow_pos;\n"
"uniform vec4 ambientproduct, diffuseproduct, specularproduct ;\n"
"struct PreviewLightRecord {\n"
"    vec4 position_and_type;\n"
"    vec4 direction_and_range;\n"
"    vec4 color_and_intensity;\n"
"    vec4 cone_and_area;\n"
"    uvec4 metadata;\n"
"};\n"
"layout(std430, binding = 2) readonly buffer PreviewLightRecords {\n"
"    PreviewLightRecord preview_lights[];\n"
"};\n"
"uniform uint preview_light_count;\n"
"uniform int shadows_enabled;\n"
"uniform vec3 camera_position;\n"
"uniform float shinyness ;\n"
"uniform sampler2D texture1;\n"
"uniform sampler2D height_texture;\n"
"uniform sampler2D emissive_texture;\n"
"uniform sampler2D alpha_texture;\n"
"uniform sampler2D normal_texture;\n"
"uniform sampler2D roughness_texture;\n"
"uniform sampler2D metallic_texture;\n"
"uniform sampler2D ao_texture;\n"
"uniform sampler2D shadow_map;\n"
"uniform samplerCube environment_map;\n"
"uniform int environment_map_enabled;\n"
"uniform float environment_intensity;\n"
"uniform float alpha;\n"
"uniform float material_reflectance;\n"
"uniform float material_smoothness;\n"
"uniform float material_height_scale;\n"
"uniform float material_emission_strength;\n"
"uniform float material_uv_rotation_speed;\n"
"uniform float material_uv_activity;\n"
"uniform float material_pulse_strength;\n"
"uniform float material_metallic;\n"
"uniform float material_roughness;\n"
"uniform float material_ao;\n"
"uniform float material_subsurface;\n"
"uniform float material_anisotropic;\n"
"uniform float material_sheen;\n"
"uniform float material_clearcoat;\n"
"uniform float material_clearcoat_roughness;\n"
"uniform float material_normal_strength;\n"
"uniform float material_transmission;\n"
"uniform float material_index_of_refraction;\n"
"uniform float material_thickness;\n"
"uniform vec3 material_attenuation_color;\n"
"uniform float preview_time;\n"
"uniform int mapping_mode;\n"
"uniform int debug_view;\n"
"uniform vec3 projection_axes[3];\n"
"uniform float mapping_scale;\n"
"uniform vec3 fog_color;\n"
"uniform float fog_start;\n"
"uniform float fog_density;\n"
"out vec4 fragColor;\n"
"vec3 safe_normalize(vec3 value, vec3 fallback){\n"
"    float magnitude = length(value);\n"
"    if (magnitude <= 1e-6) return fallback;\n"
"    return value / magnitude;\n"
"}\n"
"mat3 build_projection_basis(){\n"
"    vec3 axis_x = safe_normalize(projection_axes[0], vec3(1.0, 0.0, 0.0));\n"
"    vec3 axis_y_seed = projection_axes[1] - axis_x * dot(projection_axes[1], axis_x);\n"
"    vec3 axis_y_fallback = abs(axis_x.y) < 0.95\n"
"        ? normalize(cross(vec3(0.0, 1.0, 0.0), axis_x))\n"
"        : normalize(cross(vec3(1.0, 0.0, 0.0), axis_x));\n"
"    vec3 axis_y = safe_normalize(axis_y_seed, axis_y_fallback);\n"
"    vec3 axis_z = safe_normalize(cross(axis_x, axis_y), vec3(0.0, 0.0, 1.0));\n"
"    axis_y = safe_normalize(cross(axis_z, axis_x), axis_y);\n"
"    return mat3(axis_x, axis_y, axis_z);\n"
"}\n"
"vec3 compute_triplanar_weights(mat3 basis, vec3 world_normal){\n"
"    vec3 local_normal = abs(safe_normalize(transpose(basis) * world_normal, vec3(0.0, 1.0, 0.0)));\n"
"    vec3 weights = pow(local_normal, vec3(4.0));\n"
"    return weights / max(dot(weights, vec3(1.0)), 1e-5);\n"
"}\n"
"vec2 projected_uv(vec3 projection_space_pos, vec3 projection_space_normal, int axis_index){\n"
"    float scale = max(mapping_scale, 1e-4);\n"
"    if (axis_index == 0) {\n"
"        float u = projection_space_normal.x < 0.0 ? -projection_space_pos.z : projection_space_pos.z;\n"
"        return vec2(u, projection_space_pos.y) * scale;\n"
"    }\n"
"    if (axis_index == 1) {\n"
"        float v = projection_space_normal.y < 0.0 ? -projection_space_pos.z : projection_space_pos.z;\n"
"        return vec2(projection_space_pos.x, v) * scale;\n"
"    }\n"
"    float u = projection_space_normal.z < 0.0 ? -projection_space_pos.x : projection_space_pos.x;\n"
"    return vec2(u, projection_space_pos.y) * scale;\n"
"}\n"
"vec3 uv_debug_color(vec2 uv){\n"
"    vec2 wrapped = fract(uv);\n"
"    float bands = 0.5 + 0.5 * sin((uv.x + uv.y) * 6.2831853);\n"
"    return vec3(wrapped, bands);\n"
"}\n"
"vec3 checker_color(vec2 uv){\n"
"    vec2 grid = floor(uv * 8.0);\n"
"    float checker = mod(grid.x + grid.y, 2.0);\n"
"    return mix(vec3(0.16, 0.18, 0.22), vec3(0.92, 0.94, 0.97), checker);\n"
"}\n"
"vec2 rotate_tile_uv(vec2 uv, float angle){\n"
"    vec2 tile = floor(uv);\n"
"    vec2 local = fract(uv) - vec2(0.5);\n"
"    float s = sin(angle);\n"
"    float c = cos(angle);\n"
"    vec2 rotated = mat2(c, -s, s, c) * local;\n"
"    return tile + rotated + vec2(0.5);\n"
"}\n"
"vec2 animate_uv(vec2 uv){\n"
"    vec2 animated = uv;\n"
"    if (abs(material_uv_rotation_speed) > 1e-4) {\n"
"        animated = rotate_tile_uv(animated, preview_time * material_uv_rotation_speed);\n"
"    }\n"
"    if (material_uv_activity > 1e-4) {\n"
"        float wave_a = sin(animated.y * 6.2831853 + preview_time * (1.05 + material_uv_activity * 0.40));\n"
"        float wave_b = cos(animated.x * 6.2831853 - preview_time * (1.55 + material_uv_activity * 0.35));\n"
"        animated += vec2(wave_a, wave_b) * (0.030 * material_uv_activity);\n"
"    }\n"
"    return animated;\n"
"}\n"
"void compute_triplanar_projection(mat3 basis,\n"
"                                  out vec3 blend_weights,\n"
"                                  out vec2 uv_x,\n"
"                                  out vec2 uv_y,\n"
"                                  out vec2 uv_z,\n"
"                                  out vec3 uv_debug,\n"
"                                  out vec3 checker_debug){\n"
"    vec3 projection_space_pos = transpose(basis) * v_world_pos;\n"
"    vec3 projection_space_normal = safe_normalize(transpose(basis) * v_world_normal, vec3(0.0, 1.0, 0.0));\n"
"    uv_x = projected_uv(projection_space_pos, projection_space_normal, 0);\n"
"    uv_y = projected_uv(projection_space_pos, projection_space_normal, 1);\n"
"    uv_z = projected_uv(projection_space_pos, projection_space_normal, 2);\n"
"    blend_weights = compute_triplanar_weights(basis, v_world_normal);\n"
"    uv_debug = uv_debug_color(uv_x) * blend_weights.x\n"
"             + uv_debug_color(uv_y) * blend_weights.y\n"
"             + uv_debug_color(uv_z) * blend_weights.z;\n"
"    checker_debug = checker_color(uv_x) * blend_weights.x\n"
"                  + checker_color(uv_y) * blend_weights.y\n"
"                  + checker_color(uv_z) * blend_weights.z;\n"
"}\n"
"vec4 sample_triplanar_map(sampler2D tex,\n"
"                          vec2 uv_x,\n"
"                          vec2 uv_y,\n"
"                          vec2 uv_z,\n"
"                          vec3 blend_weights){\n"
"    return texture(tex, uv_x) * blend_weights.x\n"
"         + texture(tex, uv_y) * blend_weights.y\n"
"         + texture(tex, uv_z) * blend_weights.z;\n"
"}\n"
"float sample_triplanar_scalar(sampler2D tex,\n"
"                              vec2 uv_x,\n"
"                              vec2 uv_y,\n"
"                              vec2 uv_z,\n"
"                              vec3 blend_weights){\n"
"    return texture(tex, uv_x).r * blend_weights.x\n"
"         + texture(tex, uv_y).r * blend_weights.y\n"
"         + texture(tex, uv_z).r * blend_weights.z;\n"
"}\n"
"mat3 build_surface_tbn(vec2 uv, vec3 base_normal){\n"
"    vec3 dpdx = dFdx(v_world_pos);\n"
"    vec3 dpdy = dFdy(v_world_pos);\n"
"    vec2 duvdx = dFdx(uv);\n"
"    vec2 duvdy = dFdy(uv);\n"
"    vec3 tangent = dpdx * duvdy.y - dpdy * duvdx.y;\n"
"    vec3 bitangent = dpdy * duvdx.x - dpdx * duvdy.x;\n"
"    vec3 fallback_tangent = abs(base_normal.y) < 0.99\n"
"        ? normalize(cross(vec3(0.0, 1.0, 0.0), base_normal))\n"
"        : normalize(cross(vec3(1.0, 0.0, 0.0), base_normal));\n"
"    tangent = safe_normalize(tangent - base_normal * dot(base_normal, tangent), fallback_tangent);\n"
"    vec3 fallback_bitangent = safe_normalize(cross(base_normal, tangent), vec3(0.0, 0.0, 1.0));\n"
"    bitangent = safe_normalize(bitangent - base_normal * dot(base_normal, bitangent), fallback_bitangent);\n"
"    bitangent = safe_normalize(cross(base_normal, tangent), bitangent);\n"
"    return mat3(tangent, bitangent, base_normal);\n"
"}\n"
"vec2 apply_parallax_offset(vec2 uv, vec3 base_normal, vec3 view_dir){\n"
"    if (material_height_scale <= 1e-4) return uv;\n"
"    mat3 tbn = build_surface_tbn(uv, base_normal);\n"
"    vec3 tangent_view = transpose(tbn) * safe_normalize(view_dir, base_normal);\n"
"    float view_depth = max(abs(tangent_view.z), 0.22);\n"
"    float height = texture(height_texture, uv).r - 0.5;\n"
"    vec2 offset = tangent_view.xy / view_depth * (height * material_height_scale * 0.18);\n"
"    return uv - offset;\n"
"}\n"
"vec3 perturb_normal_from_height(vec2 uv, vec3 base_normal){\n"
"    if (material_height_scale <= 1e-4) return base_normal;\n"
"    ivec2 texture_size = textureSize(height_texture, 0);\n"
"    vec2 texel = 1.0 / vec2(max(texture_size.x, 1), max(texture_size.y, 1));\n"
"    float h_left = texture(height_texture, uv - vec2(texel.x, 0.0)).r;\n"
"    float h_right = texture(height_texture, uv + vec2(texel.x, 0.0)).r;\n"
"    float h_down = texture(height_texture, uv - vec2(0.0, texel.y)).r;\n"
"    float h_up = texture(height_texture, uv + vec2(0.0, texel.y)).r;\n"
"    vec3 tangent_space_normal = normalize(vec3((h_left - h_right) * material_height_scale * 18.0,\n"
"                                               (h_down - h_up) * material_height_scale * 18.0,\n"
"                                               1.0));\n"
"    return normalize(build_surface_tbn(uv, base_normal) * tangent_space_normal);\n"
"}\n"
"vec3 sample_environment(vec3 direction){\n"
"    vec3 dir = safe_normalize(direction, vec3(0.0, 1.0, 0.0));\n"
"    float upward = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);\n"
"    vec3 zenith = vec3(0.63, 0.73, 0.88);\n"
"    vec3 horizon = vec3(0.88, 0.84, 0.79);\n"
"    vec3 ground_near = vec3(0.56, 0.59, 0.63);\n"
"    vec3 ground_far = vec3(0.34, 0.35, 0.37);\n"
"    vec3 sky = mix(horizon, zenith, pow(upward, 0.78));\n"
"    vec3 ground = mix(ground_near, ground_far, clamp(-dir.y * 0.85, 0.0, 1.0));\n"
"    vec3 env = mix(ground, sky, smoothstep(-0.08, 0.16, dir.y));\n"
"    float horizon_band = exp(-pow(dir.y * 8.0, 2.0));\n"
"    env = mix(env, horizon, horizon_band * 0.18);\n"
"    vec3 sun_dir = normalize(vec3(0.42, 0.72, 0.55));\n"
"    float sun_focus = mix(36.0, 320.0, material_smoothness);\n"
"    float sun = pow(max(dot(dir, sun_dir), 0.0), sun_focus) * mix(0.04, 0.60, material_smoothness);\n"
"    env += vec3(0.95, 0.82, 0.62) * sun;\n"
"    return env;\n"
"}\n"
"vec3 fresnel_schlick(float cosTheta, vec3 F0){\n"
"    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);\n"
"}\n"
"vec3 fresnel_schlick_roughness(float cosTheta, vec3 F0, float roughness){\n"
"    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);\n"
"}\n"
"float distribution_ggx(vec3 N, vec3 H, float roughness){\n"
"    float a = roughness * roughness;\n"
"    float a2 = a * a;\n"
"    float ndoth = max(dot(N, H), 0.0);\n"
"    float denominator = ndoth * ndoth * (a2 - 1.0) + 1.0;\n"
"    return a2 / max(3.14159265 * denominator * denominator, 1e-5);\n"
"}\n"
"float geometry_schlick_ggx(float ndotv, float roughness){\n"
"    float r = roughness + 1.0;\n"
"    float k = (r * r) / 8.0;\n"
"    return ndotv / max(ndotv * (1.0 - k) + k, 1e-5);\n"
"}\n"
"float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness){\n"
"    return geometry_schlick_ggx(max(dot(N, V), 0.0), roughness)\n"
"         * geometry_schlick_ggx(max(dot(N, L), 0.0), roughness);\n"
"}\n"
"vec3 sample_material_environment(vec3 direction, float roughness){\n"
"    if (environment_map_enabled != 0) {\n"
"        return textureLod(environment_map, safe_normalize(direction, vec3(0.0, 1.0, 0.0)), roughness * 7.0).rgb\n"
"             * environment_intensity;\n"
"    }\n"
"    return sample_environment(direction);\n"
"}\n"
"float compute_shadow(vec4 shadow_pos, vec3 normal, vec3 light_dir){\n"
"    vec3 proj = shadow_pos.xyz / max(shadow_pos.w, 1e-6);\n"
"    proj = proj * 0.5 + 0.5;\n"
"    if (proj.z <= 0.0 || proj.z >= 1.0) return 0.0;\n"
"    if (proj.x <= 0.0 || proj.x >= 1.0 || proj.y <= 0.0 || proj.y >= 1.0) return 0.0;\n"
"    float bias = max(0.00035 * (1.0 - max(dot(normal, light_dir), 0.0)), 0.00008);\n"
"    vec2 texel = 1.0 / vec2(textureSize(shadow_map, 0));\n"
"    float occlusion = 0.0;\n"
"    for (int x = -1; x <= 1; ++x) {\n"
"        for (int y = -1; y <= 1; ++y) {\n"
"            float sampled = texture(shadow_map, proj.xy + vec2(x, y) * texel).r;\n"
"            occlusion += (proj.z - bias) > sampled ? 1.0 : 0.0;\n"
"        }\n"
"    }\n"
"    return occlusion / 9.0;\n"
"}\n"
"float smooth_range_attenuation(float light_distance, float range){\n"
"    if (range <= 0.0) return 1.0;\n"
"    float normalized_distance = light_distance / range;\n"
"    if (normalized_distance >= 1.0) return 0.0;\n"
"    float transition = smoothstep(0.80, 1.0, normalized_distance);\n"
"    float remaining = 1.0 - transition;\n"
"    return remaining * remaining;\n"
"}\n"
"void main(){\n"
"    mat3 projection_basis = build_projection_basis();\n"
"    vec3 triplanar_weights = compute_triplanar_weights(projection_basis, v_world_normal);\n"
"    vec3 base_normal = normalize(v_world_normal);\n"
"    vec3 N = base_normal;\n"
"    vec3 V = normalize(camera_position - v_world_pos);\n"
"    vec4 texel = texture(texture1, v_tex_coord);\n"
"    vec3 emissive_texel = vec3(0.0);\n"
"    float height_value = 0.5;\n"
"    float alpha_sample = 1.0;\n"
"    float roughness_sample = 1.0;\n"
"    float metallic_sample = 1.0;\n"
"    float ao_sample = 1.0;\n"
"    vec3 uv_debug = uv_debug_color(v_tex_coord);\n"
"    vec3 checker_debug = checker_color(v_tex_coord);\n"
"    vec2 uv_x = vec2(0.0);\n"
"    vec2 uv_y = vec2(0.0);\n"
"    vec2 uv_z = vec2(0.0);\n"
"    if (mapping_mode == 1) {\n"
"        compute_triplanar_projection(projection_basis,\n"
"                                     triplanar_weights,\n"
"                                     uv_x,\n"
"                                     uv_y,\n"
"                                     uv_z,\n"
"                                     uv_debug,\n"
"                                     checker_debug);\n"
"        uv_x = animate_uv(uv_x);\n"
"        uv_y = animate_uv(uv_y);\n"
"        uv_z = animate_uv(uv_z);\n"
"        uv_debug = uv_debug_color(uv_x) * triplanar_weights.x\n"
"                 + uv_debug_color(uv_y) * triplanar_weights.y\n"
"                 + uv_debug_color(uv_z) * triplanar_weights.z;\n"
"        checker_debug = checker_color(uv_x) * triplanar_weights.x\n"
"                      + checker_color(uv_y) * triplanar_weights.y\n"
"                      + checker_color(uv_z) * triplanar_weights.z;\n"
"        texel = sample_triplanar_map(texture1, uv_x, uv_y, uv_z, triplanar_weights);\n"
"        emissive_texel = sample_triplanar_map(emissive_texture, uv_x, uv_y, uv_z, triplanar_weights).rgb;\n"
"        height_value = sample_triplanar_scalar(height_texture, uv_x, uv_y, uv_z, triplanar_weights);\n"
"        alpha_sample = sample_triplanar_scalar(alpha_texture, uv_x, uv_y, uv_z, triplanar_weights);\n"
"        roughness_sample = sample_triplanar_scalar(roughness_texture, uv_x, uv_y, uv_z, triplanar_weights);\n"
"        metallic_sample = sample_triplanar_scalar(metallic_texture, uv_x, uv_y, uv_z, triplanar_weights);\n"
"        ao_sample = sample_triplanar_scalar(ao_texture, uv_x, uv_y, uv_z, triplanar_weights);\n"
"    } else {\n"
"        vec2 shaded_uv = animate_uv(v_tex_coord);\n"
"        if (material_height_scale > 1e-4) {\n"
"            shaded_uv = apply_parallax_offset(shaded_uv, base_normal, V);\n"
"            N = perturb_normal_from_height(shaded_uv, base_normal);\n"
"        }\n"
"        texel = texture(texture1, shaded_uv);\n"
"        emissive_texel = texture(emissive_texture, shaded_uv).rgb;\n"
"        height_value = texture(height_texture, shaded_uv).r;\n"
"        alpha_sample = texture(alpha_texture, shaded_uv).r;\n"
"        roughness_sample = texture(roughness_texture, shaded_uv).r;\n"
"        metallic_sample = texture(metallic_texture, shaded_uv).r;\n"
"        ao_sample = texture(ao_texture, shaded_uv).r;\n"
"        vec3 tangent_normal = texture(normal_texture, shaded_uv).xyz * 2.0 - 1.0;\n"
"        tangent_normal.xy *= material_normal_strength;\n"
"        tangent_normal = safe_normalize(tangent_normal, vec3(0.0, 0.0, 1.0));\n"
"        vec3 mapped_normal = build_surface_tbn(shaded_uv, N) * tangent_normal;\n"
"        N = safe_normalize(mapped_normal, N);\n"
"        uv_debug = uv_debug_color(shaded_uv);\n"
"        checker_debug = checker_color(shaded_uv);\n"
"    }\n"
"    float surface_alpha = clamp(texel.a * alpha * alpha_sample, 0.0, 1.0);\n"
"    if(surface_alpha < 0.02) discard;\n"
"    if (debug_view == 1) {\n"
"        fragColor = vec4(uv_debug * surface_alpha, surface_alpha);\n"
"        return;\n"
"    }\n"
"    if (debug_view == 2) {\n"
"        fragColor = vec4(checker_debug * surface_alpha, surface_alpha);\n"
"        return;\n"
"    }\n"
"    if (debug_view == 3) {\n"
"        fragColor = vec4(triplanar_weights * surface_alpha, surface_alpha);\n"
"        return;\n"
"    }\n"
"    vec3 albedo = max(texel.rgb, vec3(0.0));\n"
"    vec3 emissive_color = max(emissive_texel, vec3(0.0)) * material_emission_strength;\n"
"    float pulse = 1.0 + sin(preview_time * (2.2 + material_uv_activity * 0.8)\n"
"                             + dot(v_world_pos, vec3(0.41, 0.23, 0.37)) * 0.85)\n"
"                         * 0.5 * material_pulse_strength;\n"
"    pulse = max(pulse, 0.0);\n"
"    emissive_color *= pulse;\n"
"    float roughness = clamp(material_roughness * mix(0.88, 1.12, roughness_sample), 0.045, 1.0);\n"
"    float metallic = clamp(material_metallic * metallic_sample, 0.0, 1.0);\n"
"    float ambient_occlusion = clamp(material_ao * ao_sample, 0.0, 1.0);\n"
"    float cavity = mix(0.90, 1.06, clamp(height_value, 0.0, 1.0));\n"
"    float ndotv = max(dot(N, V), 0.0);\n"
"    vec3 dielectric_f0 = mix(vec3(0.04), vec3(0.16), material_reflectance);\n"
"    vec3 F0 = mix(dielectric_f0, albedo, metallic);\n"
"    vec3 direct_lighting = vec3(0.0);\n"
"    uint bounded_light_count = min(preview_light_count, 32u);\n"
"    for (uint light_index = 0u; light_index < bounded_light_count; ++light_index) {\n"
"        PreviewLightRecord light = preview_lights[light_index];\n"
"        if ((light.metadata.x & 1u) == 0u) continue;\n"
"        uint light_type = uint(round(light.position_and_type.w));\n"
"        vec3 L = vec3(0.0, 1.0, 0.0);\n"
"        float light_distance = 1.0;\n"
"        float attenuation = 1.0;\n"
"        if (light_type == 0u) {\n"
"            L = safe_normalize(-light.direction_and_range.xyz, vec3(0.0, 1.0, 0.0));\n"
"        } else {\n"
"            vec3 light_vector = light.position_and_type.xyz - v_world_pos;\n"
"            light_distance = max(length(light_vector), 0.001);\n"
"            L = light_vector / light_distance;\n"
"            attenuation = smooth_range_attenuation(light_distance, light.direction_and_range.w)\n"
"                        / max(light_distance * light_distance, 0.01);\n"
"            if (light_type == 2u) {\n"
"                float cone_cosine = dot(safe_normalize(light.direction_and_range.xyz, vec3(0.0, -1.0, 0.0)), -L);\n"
"                attenuation *= smoothstep(light.cone_and_area.y, light.cone_and_area.x, cone_cosine);\n"
"            } else if (light_type == 3u) {\n"
"                float facing = dot(safe_normalize(light.direction_and_range.xyz, vec3(0.0, -1.0, 0.0)), -L);\n"
"                if ((light.metadata.x & 4u) != 0u) facing = abs(facing);\n"
"                attenuation *= max(facing, 0.0) * max(light.cone_and_area.z * light.cone_and_area.w, 0.0001);\n"
"            }\n"
"        }\n"
"        if (attenuation <= 0.0) continue;\n"
"        vec3 H = normalize(L + V);\n"
"        float ndotl = max(dot(N, L), 0.0);\n"
"        float wrapped_ndotl = clamp((dot(N, L) + material_subsurface * 0.35)\n"
"                                    / (1.0 + material_subsurface * 0.35), 0.0, 1.0);\n"
"        vec3 tangent = safe_normalize(dFdx(v_world_pos), vec3(1.0, 0.0, 0.0));\n"
"        float anisotropic_alignment = abs(dot(H, tangent));\n"
"        float effective_roughness = clamp(roughness * mix(1.0, 0.62 + 0.38 * anisotropic_alignment, material_anisotropic), 0.045, 1.0);\n"
"        float ndf = distribution_ggx(N, H, effective_roughness);\n"
"        float geometry = geometry_smith(N, V, L, effective_roughness);\n"
"        vec3 fresnel = fresnel_schlick(max(dot(H, V), 0.0), F0);\n"
"        vec3 specular = (ndf * geometry * fresnel)\n"
"                      / max(4.0 * ndotv * ndotl, 0.001);\n"
"        vec3 energy_conserving_diffuse = (vec3(1.0) - fresnel) * (1.0 - metallic);\n"
"        float clearcoat_ndf = distribution_ggx(N, H, clamp(material_clearcoat_roughness, 0.045, 1.0));\n"
"        vec3 clearcoat_fresnel = fresnel_schlick(max(dot(H, V), 0.0), vec3(0.04));\n"
"        vec3 clearcoat_specular = material_clearcoat * clearcoat_ndf * clearcoat_fresnel\n"
"                                / max(4.0 * ndotv * ndotl, 0.001);\n"
"        bool uses_primary_shadow = shadows_enabled != 0\n"
"            && (light.metadata.x & 2u) != 0u\n"
"            && light.metadata.y == 0u;\n"
"        float shadow = uses_primary_shadow ? compute_shadow(v_shadow_pos, N, L) : 0.0;\n"
"        float lit = 1.0 - shadow * 0.48;\n"
"        vec3 radiance = light.color_and_intensity.rgb * light.color_and_intensity.w * attenuation;\n"
"        vec3 diffuse = energy_conserving_diffuse * albedo / 3.14159265;\n"
"        direct_lighting += (diffuse * wrapped_ndotl + (specular + clearcoat_specular) * ndotl)\n"
"                         * radiance * lit;\n"
"    }\n"
"    vec3 reflection_direction = reflect(-V, N);\n"
"    vec3 environment = sample_material_environment(reflection_direction, roughness);\n"
"    vec3 environment_fresnel = fresnel_schlick_roughness(ndotv, F0, roughness);\n"
"    vec3 environment_diffuse = albedo * (1.0 - metallic) * 0.24;\n"
"    vec3 environment_specular = environment * environment_fresnel\n"
"                              * mix(0.32, 1.18, material_reflectance);\n"
"    vec3 sheen_color = albedo * material_sheen * pow(1.0 - ndotv, 5.0) * 0.42;\n"
"    vec3 lit_color = (environment_diffuse + environment_specular) * ambient_occlusion * cavity\n"
"                   + direct_lighting + sheen_color;\n"
"    float transmission = clamp(material_transmission, 0.0, 1.0);\n"
"    if (transmission > 0.0) {\n"
"        float ior = clamp(material_index_of_refraction, 1.0, 2.5);\n"
"        float ior_f0 = pow((ior - 1.0) / (ior + 1.0), 2.0);\n"
"        vec3 glass_fresnel = fresnel_schlick(ndotv, vec3(ior_f0));\n"
"        vec3 refraction_direction = refract(-V, N, 1.0 / ior);\n"
"        if (length(refraction_direction) <= 1e-5) refraction_direction = reflection_direction;\n"
"        vec3 transmitted_environment = sample_material_environment(refraction_direction, roughness);\n"
"        vec3 absorption = pow(clamp(material_attenuation_color, vec3(0.02), vec3(1.0)),\n"
"                              vec3(max(material_thickness, 0.001) * 5.0));\n"
"        vec3 transmitted_color = transmitted_environment * absorption * mix(vec3(1.0), albedo, 0.16);\n"
"        vec3 reflected_color = environment * mix(vec3(1.0), albedo, metallic * 0.30);\n"
"        vec3 glass_color = mix(transmitted_color, reflected_color, glass_fresnel);\n"
"        lit_color = mix(lit_color, glass_color, transmission);\n"
"        float edge_reflection = max(max(glass_fresnel.r, glass_fresnel.g), glass_fresnel.b);\n"
"        surface_alpha = mix(surface_alpha, clamp(surface_alpha + edge_reflection * 0.34, 0.16, 0.94), transmission);\n"
"    }\n"
"    float distance_to_camera = length(camera_position - v_world_pos);\n"
"    float fog_factor = 1.0 - exp(-max(distance_to_camera - fog_start, 0.0) * fog_density);\n"
"    fog_factor = clamp(fog_factor * 0.50, 0.0, 0.58);\n"
"    float height_mix = clamp(exp(-max(v_world_pos.y + 0.75, 0.0) * 0.15), 0.74, 1.0);\n"
"    vec3 fog_tint = mix(fog_color * vec3(0.98, 0.99, 1.01), fog_color, height_mix);\n"
"    vec3 color = mix(lit_color, fog_tint, fog_factor);\n"
"    color += emissive_color * (1.0 - fog_factor * 0.40);\n"
"    color *= 1.14;\n"
"    color = color / (color + vec3(1.0));\n"
"    color = pow(color, vec3(1.0 / 2.2));\n"
"    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));\n"
"    color = mix(vec3(luma), color, 1.16);\n"
"    fragColor = vec4(color * surface_alpha, surface_alpha);\n"
"}\n";

const GLchar *DYNAMIC_CUBE_VERTEX_SOURCE =
"#version 460 core\n"
"in vec3 position;\n"
"in vec3 normal;\n"
"in vec2 texture;\n"
"out vec3 v_world_normal;\n"
"out vec3 v_world_pos;\n"
"out vec2 v_tex_coord;\n"
"out vec4 v_shadow_pos;\n"
"uniform mat4 projection;\n"
"uniform mat4 view;\n"
"uniform mat4 scene_model;\n"
"uniform mat4 light_view_projection;\n"
"uniform mat4 prev_primary_transform;\n"
"uniform mat4 prev_secondary_transform;\n"
"uniform mat4 curr_primary_transform;\n"
"uniform mat4 curr_secondary_transform;\n"
"uniform mat3 prev_primary_normal_transform;\n"
"uniform mat3 prev_secondary_normal_transform;\n"
"uniform mat3 curr_primary_normal_transform;\n"
"uniform mat3 curr_secondary_normal_transform;\n"
"uniform vec3 dual_scales[3];\n"
"uniform vec3 dual_translations[3];\n"
"uniform float texscale;\n"
"uniform float interpolation_alpha;\n"
"uniform int cube_mode;\n"
"const float kFaceEpsilon = 0.0001;\n"
"mat4 translation_matrix(vec3 t){\n"
"    mat4 m = mat4(1.0);\n"
"    m[3] = vec4(t, 1.0);\n"
"    return m;\n"
"}\n"
"mat4 scale_matrix(vec3 s){\n"
"    mat4 m = mat4(1.0);\n"
"    m[0][0] = s.x;\n"
"    m[1][1] = s.y;\n"
"    m[2][2] = s.z;\n"
"    return m;\n"
"}\n"
"mat4 dual_axis_transform(int axis){\n"
"    return translation_matrix(dual_translations[axis]) * scale_matrix(dual_scales[axis]);\n"
"}\n"
"mat3 safe_normal_matrix(mat4 transform){\n"
"    mat3 linear = mat3(transform);\n"
"    float det = determinant(linear);\n"
"    if (abs(det) <= 1e-6) {\n"
"        return mat3(1.0);\n"
"    }\n"
"    return transpose(inverse(linear));\n"
"}\n"
"vec3 face_tangent(vec3 normal){\n"
"    vec3 axis = normalize(length(normal) > 1e-6 ? normal : vec3(0.0, 1.0, 0.0));\n"
"    vec3 abs_axis = abs(axis);\n"
"    if (abs_axis.x >= abs_axis.y && abs_axis.x >= abs_axis.z) {\n"
"        return axis.x >= 0.0 ? vec3(0.0, 0.0, -1.0) : vec3(0.0, 0.0, 1.0);\n"
"    }\n"
"    if (abs_axis.y >= abs_axis.z) {\n"
"        return vec3(1.0, 0.0, 0.0);\n"
"    }\n"
"    return axis.z >= 0.0 ? vec3(1.0, 0.0, 0.0) : vec3(-1.0, 0.0, 0.0);\n"
"}\n"
"vec3 face_bitangent(vec3 normal){\n"
"    vec3 axis = normalize(length(normal) > 1e-6 ? normal : vec3(0.0, 1.0, 0.0));\n"
"    vec3 abs_axis = abs(axis);\n"
"    if (abs_axis.x >= abs_axis.y && abs_axis.x >= abs_axis.z) {\n"
"        return vec3(0.0, 1.0, 0.0);\n"
"    }\n"
"    if (abs_axis.y >= abs_axis.z) {\n"
"        return axis.y >= 0.0 ? vec3(0.0, 0.0, -1.0) : vec3(0.0, 0.0, 1.0);\n"
"    }\n"
"    return vec3(0.0, 1.0, 0.0);\n"
"}\n"
"vec2 face_aligned_uv(vec3 local_pos, vec3 local_normal, mat4 world_transform){\n"
"    vec3 tangent = face_tangent(local_normal);\n"
"    vec3 bitangent = face_bitangent(local_normal);\n"
"    float tangent_scale = max(length(mat3(world_transform) * tangent), 1e-6);\n"
"    float bitangent_scale = max(length(mat3(world_transform) * bitangent), 1e-6);\n"
"    float effective_texscale = texscale <= 0.1255 ? 0.90 : max(texscale, 0.0) * 2.0;\n"
"    return vec2(dot(local_pos, tangent) * tangent_scale,\n"
"                dot(local_pos, bitangent) * bitangent_scale) * effective_texscale;\n"
"}\n"
"vec3 logical_min(){\n"
"    if (cube_mode == 1) return vec3(0.0, -0.5, -0.5);\n"
"    if (cube_mode == 2) return vec3(-0.5, 0.0, -0.5);\n"
"    if (cube_mode == 3) return vec3(-0.5, -0.5, 0.0);\n"
"    return vec3(-0.5, -0.5, -0.5);\n"
"}\n"
"vec3 logical_offset(){\n"
"    if (cube_mode == 1) return vec3(0.5, -0.5, 0.0);\n"
"    if (cube_mode == 2) return vec3(0.0, 0.0, 0.0);\n"
"    if (cube_mode == 3) return vec3(0.0, -0.5, 0.5);\n"
"    return vec3(0.0, -0.5, 0.0);\n"
"}\n"
"bool use_secondary_transform(vec3 logical_pos){\n"
"    if (cube_mode == 1) return logical_pos.x > 0.5;\n"
"    if (cube_mode == 2) return logical_pos.y > 0.5;\n"
"    if (cube_mode == 3) return logical_pos.z > 0.5;\n"
"    return logical_pos.z > 0.0;\n"
"}\n"
"void main(){\n"
"    vec3 logical_pos = position + logical_offset();\n"
"    vec3 min_pos = logical_min();\n"
"    mat4 local_transform = mat4(1.0);\n"
"    if (abs(logical_pos.x - min_pos.x) <= kFaceEpsilon) local_transform = dual_axis_transform(0) * local_transform;\n"
"    if (abs(logical_pos.y - min_pos.y) <= kFaceEpsilon) local_transform = dual_axis_transform(1) * local_transform;\n"
"    if (abs(logical_pos.z - min_pos.z) <= kFaceEpsilon) local_transform = dual_axis_transform(2) * local_transform;\n"
"    mat3 local_normal_transform = safe_normal_matrix(local_transform);\n"
"    vec4 local_vertex = local_transform * vec4(logical_pos, 1.0);\n"
"    bool secondary = use_secondary_transform(logical_pos);\n"
"    mat4 prev_world_transform = prev_primary_transform;\n"
"    mat4 curr_world_transform = curr_primary_transform;\n"
"    mat3 prev_world_normal_transform = prev_primary_normal_transform;\n"
"    mat3 curr_world_normal_transform = curr_primary_normal_transform;\n"
"    if (secondary) {\n"
"        prev_world_transform = prev_secondary_transform;\n"
"        curr_world_transform = curr_secondary_transform;\n"
"        prev_world_normal_transform = prev_secondary_normal_transform;\n"
"        curr_world_normal_transform = curr_secondary_normal_transform;\n"
"    }\n"
"    vec3 prev_world_pos = (prev_world_transform * local_vertex).xyz;\n"
"    vec3 curr_world_pos = (curr_world_transform * local_vertex).xyz;\n"
"    vec3 local_normal = normalize(local_normal_transform * normal);\n"
"    vec3 prev_world_normal = normalize(prev_world_normal_transform * local_normal);\n"
"    vec3 curr_world_normal = normalize(curr_world_normal_transform * local_normal);\n"
"    vec3 mixed_world_pos = mix(prev_world_pos, curr_world_pos, interpolation_alpha);\n"
"    vec3 mixed_world_normal = normalize(mix(prev_world_normal, curr_world_normal, interpolation_alpha));\n"
"    vec4 scaled_world = scene_model * vec4(mixed_world_pos, 1.0);\n"
"    v_world_pos = scaled_world.xyz;\n"
"    v_world_normal = normalize(mat3(scene_model) * mixed_world_normal);\n"
"    v_tex_coord = face_aligned_uv(local_vertex.xyz, local_normal, curr_world_transform);\n"
"    v_shadow_pos = light_view_projection * scaled_world;\n"
"    gl_Position = projection * view * scaled_world;\n"
"}\n";

const GLchar *SHADOW_VERTEX_SOURCE =
"#version 460 core\n"
"in vec3 position;\n"
"uniform mat4 model;\n"
"uniform mat4 light_view_projection;\n"
"void main(){\n"
"    gl_Position = light_view_projection * model * vec4(position, 1.0);\n"
"}\n";

const GLchar *SHADOW_DYNAMIC_VERTEX_SOURCE =
"#version 460 core\n"
"in vec3 position;\n"
"uniform mat4 scene_model;\n"
"uniform mat4 light_view_projection;\n"
"uniform mat4 prev_primary_transform;\n"
"uniform mat4 prev_secondary_transform;\n"
"uniform mat4 curr_primary_transform;\n"
"uniform mat4 curr_secondary_transform;\n"
"uniform vec3 dual_scales[3];\n"
"uniform vec3 dual_translations[3];\n"
"uniform float interpolation_alpha;\n"
"uniform int cube_mode;\n"
"const float kFaceEpsilon = 0.0001;\n"
"mat4 translation_matrix(vec3 t){\n"
"    mat4 m = mat4(1.0);\n"
"    m[3] = vec4(t, 1.0);\n"
"    return m;\n"
"}\n"
"mat4 scale_matrix(vec3 s){\n"
"    mat4 m = mat4(1.0);\n"
"    m[0][0] = s.x;\n"
"    m[1][1] = s.y;\n"
"    m[2][2] = s.z;\n"
"    return m;\n"
"}\n"
"mat4 dual_axis_transform(int axis){\n"
"    return translation_matrix(dual_translations[axis]) * scale_matrix(dual_scales[axis]);\n"
"}\n"
"vec3 logical_min(){\n"
"    if (cube_mode == 1) return vec3(0.0, -0.5, -0.5);\n"
"    if (cube_mode == 2) return vec3(-0.5, 0.0, -0.5);\n"
"    if (cube_mode == 3) return vec3(-0.5, -0.5, 0.0);\n"
"    return vec3(-0.5, -0.5, -0.5);\n"
"}\n"
"vec3 logical_offset(){\n"
"    if (cube_mode == 1) return vec3(0.5, -0.5, 0.0);\n"
"    if (cube_mode == 2) return vec3(0.0, 0.0, 0.0);\n"
"    if (cube_mode == 3) return vec3(0.0, -0.5, 0.5);\n"
"    return vec3(0.0, -0.5, 0.0);\n"
"}\n"
"bool use_secondary_transform(vec3 logical_pos){\n"
"    if (cube_mode == 1) return logical_pos.x > 0.5;\n"
"    if (cube_mode == 2) return logical_pos.y > 0.5;\n"
"    if (cube_mode == 3) return logical_pos.z > 0.5;\n"
"    return logical_pos.z > 0.0;\n"
"}\n"
"void main(){\n"
"    vec3 logical_pos = position + logical_offset();\n"
"    vec3 min_pos = logical_min();\n"
"    mat4 local_transform = mat4(1.0);\n"
"    if (abs(logical_pos.x - min_pos.x) <= kFaceEpsilon) local_transform = dual_axis_transform(0) * local_transform;\n"
"    if (abs(logical_pos.y - min_pos.y) <= kFaceEpsilon) local_transform = dual_axis_transform(1) * local_transform;\n"
"    if (abs(logical_pos.z - min_pos.z) <= kFaceEpsilon) local_transform = dual_axis_transform(2) * local_transform;\n"
"    vec4 local_vertex = local_transform * vec4(logical_pos, 1.0);\n"
"    bool secondary = use_secondary_transform(logical_pos);\n"
"    mat4 prev_world_transform = secondary ? prev_secondary_transform : prev_primary_transform;\n"
"    mat4 curr_world_transform = secondary ? curr_secondary_transform : curr_primary_transform;\n"
"    vec3 prev_world_pos = (prev_world_transform * local_vertex).xyz;\n"
"    vec3 curr_world_pos = (curr_world_transform * local_vertex).xyz;\n"
"    vec3 mixed_world_pos = mix(prev_world_pos, curr_world_pos, interpolation_alpha);\n"
"    vec4 scaled_world = scene_model * vec4(mixed_world_pos, 1.0);\n"
"    gl_Position = light_view_projection * scaled_world;\n"
"}\n";

const GLchar *SHADOW_FRAGMENT_SOURCE =
"#version 460 core\n"
"void main(){\n"
"}\n";

const GLchar *GRID_VERTEX_SOURCE =
"#version 460 core\n"
"layout (location = 0) in vec3 position;\n"
"uniform mat4 projection;\n"
"uniform mat4 view;\n"
"void main(){\n"
"    gl_Position = projection * view * vec4(position, 1.0);\n"
"}\n";

const GLchar *GRID_FRAGMENT_SOURCE =
"#version 460 core\n"
"uniform vec4 grid_color;\n"
"out vec4 fragColor;\n"
"void main(){\n"
"    fragColor = grid_color;\n"
"}\n";

const GLchar *OVERLAY_VERTEX_SOURCE =
"#version 460 core\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec4 color;\n"
"out vec4 v_color;\n"
"uniform mat4 projection;\n"
"uniform mat4 view;\n"
"uniform mat4 model;\n"
"uniform float alpha_scale;\n"
"void main(){\n"
"    v_color = vec4(color.rgb, color.a * alpha_scale);\n"
"    gl_Position = projection * view * model * vec4(position, 1.0);\n"
"}\n";

const GLchar *OVERLAY_FRAGMENT_SOURCE =
"#version 460 core\n"
"in vec4 v_color;\n"
"out vec4 fragColor;\n"
"void main(){\n"
"    fragColor = v_color;\n"
"}\n";

const GLchar *BACKGROUND_VERTEX_SOURCE =
"#version 460 core\n"
"out vec2 v_uv;\n"
"const vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));\n"
"void main(){\n"
"    vec2 position = positions[gl_VertexID];\n"
"    v_uv = position * 0.5 + 0.5;\n"
"    gl_Position = vec4(position, 0.0, 1.0);\n"
"}\n";

const GLchar *BACKGROUND_FRAGMENT_SOURCE =
"#version 460 core\n"
"in vec2 v_uv;\n"
"out vec4 fragColor;\n"
"void main(){\n"
"    float vertical = clamp(pow(v_uv.y, 0.78), 0.0, 1.0);\n"
"    vec3 horizon = vec3(0.88, 0.84, 0.79);\n"
"    vec3 zenith = vec3(0.63, 0.73, 0.88);\n"
"    vec3 nadir = vec3(0.95, 0.92, 0.86);\n"
"    vec3 color = mix(nadir, zenith, vertical);\n"
"    float horizon_band = exp(-pow((v_uv.y - 0.42) * 8.5, 2.0));\n"
"    color = mix(color, horizon, horizon_band * 0.34);\n"
"    vec2 sun_delta = v_uv - vec2(0.72, 0.80);\n"
"    float sun = exp(-dot(sun_delta, sun_delta) * 22.0);\n"
"    color += vec3(0.18, 0.14, 0.09) * sun;\n"
"    fragColor = vec4(color, 1.0);\n"
"}\n";

const GLchar *OUTLINE_VERTEX_SOURCE =
"#version 460 core\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec3 normal;\n"
"uniform mat4 projection;\n"
"uniform mat4 view;\n"
"uniform mat4 scene_model;\n"
"uniform float outline_width;\n"
"void main(){\n"
"    vec4 scaled_world = scene_model * vec4(position, 1.0);\n"
"    vec3 world_normal = normalize(mat3(scene_model) * normal);\n"
"    vec3 extruded = scaled_world.xyz + world_normal * outline_width;\n"
"    gl_Position = projection * view * vec4(extruded, 1.0);\n"
"}\n";

const GLchar *OUTLINE_FRAGMENT_SOURCE =
"#version 460 core\n"
"uniform vec4 outline_color;\n"
"out vec4 fragColor;\n"
"void main(){\n"
"    fragColor = outline_color;\n"
"}\n";



const GLfloat kVertexData[] = {
  1.0, -1.0, -1.0, 0.0, -1.0, 0.0, 1.0,-1.0,
  1.0, -1.0, 1.0, 0.0, -1.0, 0.0, 1.0, 1.0,
 -1.0, -1.0, 1.0, 0.0, -1.0, 0.0,-1.0, 1.0,
 -1.0, -1.0, 1.0, 0.0, -1.0, 0.0,-1.0, 1.0,
 -1.0, -1.0,-1.0, 0.0, -1.0, 0.0,-1.0,-1.0,
  1.0, -1.0, -1.0, 0.0, -1.0, 0.0, 1.0,-1.0,
 -1.0, 1.0, 1.0, 0.0, 1.0, 0.0, -1.0,1.0,
  1.0, 1.0, 1.0, 0.0, 1.0, 0.0, 1.0,1.0,
  1.0, 1.0, -1.0, 0.0, 1.0, 0.0, 1.0,-1.0,
  1.0, 1.0, -1.0, 0.0, 1.0, 0.0, 1.0,-1.0,
 -1.0, 1.0,-1.0, 0.0, 1.0, 0.0, -1.0,-1.0,
 -1.0, 1.0, 1.0, 0.0, 1.0, 0.0, -1.0,1.0,
 -1.0, 1.0, -1.0, -1.0, 0.0, 0.0, -1.0,1.0,
 -1.0, -1.0,-1.0, -1.0, 0.0, 0.0, -1.0,-1.0,
 -1.0, -1.0, 1.0, -1.0, 0.0, 0.0, 1.0,-1.0,
 -1.0, -1.0, 1.0, -1.0, 0.0, 0.0, 1.0,-1.0,
 -1.0, 1.0, 1.0, -1.0, 0.0, 0.0, 1.0,1.0,
 -1.0, 1.0, -1.0, -1.0, 0.0, 0.0, -1.0,1.0,
  1.0, -1.0, 1.0, 1.0, 0.0, 0.0, 1.0,-1.0,
  1.0, -1.0,-1.0, 1.0, 0.0, 0.0, -1.0,-1.0,
  1.0, 1.0, -1.0, 1.0, 0.0, 0.0, -1.0,1.0,
  1.0, 1.0,-1.0, 1.0, 0.0, 0.0, -1.0,1.0,
  1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0,1.0,
  1.0, -1.0, 1.0, 1.0, 0.0, 0.0, 1.0,-1.0,
 -1.0, 1.0, 1.0, 0.0, 0.0, 1.0, -1.0,1.0,
 -1.0, -1.0,1.0, 0.0, 0.0, 1.0, -1.0,-1.0,
  1.0, -1.0, 1.0, 0.0, 0.0, 1.0, 1.0,-1.0,
  1.0, -1.0, 1.0, 0.0, 0.0, 1.0, 1.0,-1.0,
  1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,1.0,
 -1.0, 1.0, 1.0, 0.0, 0.0, 1.0, -1.0,1.0,
 -1.0, 1.0, -1.0, 0.0, 0.0, -1.0, -1.0, 1.0,
  1.0, 1.0, -1.0, 0.0, 0.0, -1.0, 1.0, 1.0,
  1.0, -1.0, -1.0, 0.0, 0.0, -1.0, 1.0,-1.0,
  1.0, -1.0, -1.0, 0.0, 0.0, -1.0, 1.0,-1.0,
 -1.0, -1.0, -1.0, 0.0, 0.0, -1.0, -1.0,-1.0,
 -1.0, 1.0, -1.0, 0.0, 0.0, -1.0, -1.0,1.0
};

constexpr int kShadowMapResolution = 2048;
constexpr GLsizei kPreviewMsaaSamples = 4;

class PreviewShadowMapPass {
public:
	void execute(PreviewRenderRuntimeEnvironment &runtime,
	             const PreviewLightingFrame &lighting_frame,
	             float scale_global,
	             float simulation_alpha,
	             const std::vector<PreviewMaterial> &materials) const;
};

class PreviewScenePass {
public:
	void execute(PreviewRenderRuntimeEnvironment &runtime,
	             const CameraMatrices &camera,
	             const PreviewLightingFrame &lighting_frame,
	             const PreviewRenderSettings &settings,
	             float scale_global,
	             const std::vector<PreviewMaterial> &materials) const;
};

class PreviewResolvePass {
public:
	void execute(PreviewRenderRuntimeEnvironment &runtime) const;
};

PreviewRenderRuntimeEnvironment &preview_runtime_environment()
{
	static PreviewRenderRuntimeEnvironment runtime_environment;
	return runtime_environment;
}


CameraMatrices build_camera_matrices(int width,
                                     int height,
                                     float camera_distance,
                                     float angle_view,
                                     float elevation_view,
	                                 float roll_view,
	                                 float vertical_field_of_view_radians,
                                     const vec3 &target,
                                     float scene_scale)
{
	const float angle_radians = -angle_view * static_cast<float>(M_PI) / 180.0f;
	const float elevation_radians = elevation_view * static_cast<float>(M_PI) / 180.0f;
	const float radius = std::max(camera_distance, 0.25f);
	const vec3 scaled_target = target * scene_scale;

	CameraMatrices camera;
	camera.position =
		scaled_target +
		vec3(radius * std::cos(elevation_radians) * std::cos(angle_radians),
		     radius * std::sin(elevation_radians),
		     radius * std::cos(elevation_radians) * std::sin(angle_radians));
	camera.target = scaled_target;
	camera.orbit_distance = radius;
	const vec3 forward = glm::normalize(scaled_target - camera.position);
	vec3 right = glm::cross(forward, vec3(0.0f, 1.0f, 0.0f));
	if (glm::length(right) <= 0.0001f) {
		right = glm::cross(forward, vec3(0.0f, 0.0f, 1.0f));
	}
	right = glm::normalize(right);
	right = rotate_around_axis(right, forward, glm::radians(roll_view));
	const vec3 up = glm::normalize(glm::cross(right, forward));
	camera.view = lookAt(camera.position, scaled_target, up);
	const double far_plane = std::max(100.0, static_cast<double>(radius * 4.5f + 24.0f));
	camera.projection = perspective(static_cast<double>(vertical_field_of_view_radians),
	                                static_cast<double>(width) / static_cast<double>(height),
	                                0.01,
	                                far_plane);
	return camera;
}

PreviewLightCameraContext build_preview_light_camera_context(
	const CameraMatrices &camera,
	float scale_global)
{
	PreviewLightCameraContext context;
	context.camera_position = camera.position;
	context.camera_target = camera.target;
	context.camera_view_direction = glm::normalize(camera.position - camera.target);
	if (glm::dot(context.camera_view_direction, context.camera_view_direction) <= 1e-6f) {
		context.camera_view_direction = vec3(0.42f, 0.38f, 0.82f);
	}
	context.camera_right = glm::cross(
		vec3(0.0f, 1.0f, 0.0f), context.camera_view_direction);
	if (glm::dot(context.camera_right, context.camera_right) <= 1e-6f) {
		context.camera_right = vec3(1.0f, 0.0f, 0.0f);
	} else {
		context.camera_right = glm::normalize(context.camera_right);
	}
	context.camera_up = glm::normalize(glm::cross(
		context.camera_view_direction, context.camera_right));
	context.orbit_distance = camera.orbit_distance;
	context.scene_scale = scale_global;
	return context;
}

glm::mat3 safe_normal_transform(const glm::mat4 &transform)
{
	const glm::mat3 linear(transform);
	const float determinant = glm::determinant(linear);
	if (std::fabs(determinant) <= 0.000001f) {
		return glm::mat3(1.0f);
	}
	return glm::transpose(glm::inverse(linear));
}

std::vector<GLfloat> build_grid_vertices()
{
	constexpr int grid_extent = 20;
	constexpr float y = -0.001f;
	std::vector<GLfloat> vertices;
	vertices.reserve(static_cast<std::size_t>((grid_extent * 2 + 1) * 4 * 3));
	for (int coordinate = -grid_extent; coordinate <= grid_extent; ++coordinate) {
		vertices.push_back(static_cast<float>(coordinate));
		vertices.push_back(y);
		vertices.push_back(static_cast<float>(-grid_extent));
		vertices.push_back(static_cast<float>(coordinate));
		vertices.push_back(y);
		vertices.push_back(static_cast<float>(grid_extent));

		vertices.push_back(static_cast<float>(-grid_extent));
		vertices.push_back(y);
		vertices.push_back(static_cast<float>(coordinate));
		vertices.push_back(static_cast<float>(grid_extent));
		vertices.push_back(y);
		vertices.push_back(static_cast<float>(coordinate));
	}
	return vertices;
}

void log_gl_errors(const char *phase)
{
	GLenum error_code = glGetError();
	if (error_code == GL_NO_ERROR) {
		return;
	}
	std::ostringstream ss;
	ss << "OpenGL error at " << phase;
	while (error_code != GL_NO_ERROR) {
		ss << " 0x" << std::hex << error_code;
		error_code = glGetError();
	}
	debugout(ss.str());
}

GLuint create_shader(GLenum type, const GLchar *source)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	GLint status = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_TRUE) {
		return shader;
	}

	GLint log_len = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
	std::vector<char> buffer(static_cast<std::size_t>(std::max(log_len, 1)));
	glGetShaderInfoLog(shader, log_len, NULL, buffer.data());
	debugout(std::string("Shader compile failure: ") + buffer.data());
	glDeleteShader(shader);
	return 0;
}

void setup_buffer_layout(GLuint vao, GLuint buffer)
{
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
}

void setup_overlay_buffer_layout(GLuint vao, GLuint buffer)
{
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
}

void ensure_geometry_capacity(std::size_t texture_capacity)
{
	PreviewGeometryRepository &geometry_repository = preview_runtime_environment().geometry_repository;
	const std::size_t current_capacity = geometry_repository.opaque_vaos.size();
	if (current_capacity >= texture_capacity) {
		if (geometry_repository.opaque_render_counts.size() < texture_capacity) {
			geometry_repository.opaque_render_counts.resize(texture_capacity, 0);
		}
		return;
	}

	geometry_repository.opaque_vaos.resize(texture_capacity, 0);
	geometry_repository.opaque_buffers.resize(texture_capacity, 0);
	geometry_repository.opaque_render_counts.resize(texture_capacity, 0);

	for (std::size_t i = current_capacity; i < texture_capacity; ++i) {
		glGenVertexArrays(1, &geometry_repository.opaque_vaos[i]);
		glGenBuffers(1, &geometry_repository.opaque_buffers[i]);
		setup_buffer_layout(geometry_repository.opaque_vaos[i], geometry_repository.opaque_buffers[i]);
		glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void ensure_transparent_batch_capacity(std::size_t batch_capacity)
{
	PreviewGeometryRepository &geometry_repository = preview_runtime_environment().geometry_repository;
	const std::size_t current_capacity = geometry_repository.transparent_vaos.size();
	if (current_capacity >= batch_capacity) {
		return;
	}

	geometry_repository.transparent_vaos.resize(batch_capacity, 0);
	geometry_repository.transparent_buffers.resize(batch_capacity, 0);

	for (std::size_t index = current_capacity; index < batch_capacity; ++index) {
		glGenVertexArrays(1, &geometry_repository.transparent_vaos[index]);
		glGenBuffers(1, &geometry_repository.transparent_buffers[index]);
		setup_buffer_layout(geometry_repository.transparent_vaos[index],
		                    geometry_repository.transparent_buffers[index]);
		glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void ensure_outline_capacity(std::size_t batch_capacity)
{
	PreviewGeometryRepository &geometry_repository = preview_runtime_environment().geometry_repository;
	const std::size_t current_capacity = geometry_repository.outline_vaos.size();
	if (current_capacity >= batch_capacity) {
		return;
	}

	geometry_repository.outline_vaos.resize(batch_capacity, 0);
	geometry_repository.outline_buffers.resize(batch_capacity, 0);
	geometry_repository.outline_counts.resize(batch_capacity, 0);
	geometry_repository.outline_colors.resize(batch_capacity, vec4(1.0f));
	geometry_repository.outline_widths.resize(batch_capacity, 0.03f);

	for (std::size_t index = current_capacity; index < batch_capacity; ++index) {
		glGenVertexArrays(1, &geometry_repository.outline_vaos[index]);
		glGenBuffers(1, &geometry_repository.outline_buffers[index]);
		setup_buffer_layout(geometry_repository.outline_vaos[index], geometry_repository.outline_buffers[index]);
		glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void destroy_preview_target()
{
	PreviewRenderTargetRepository &render_targets = preview_runtime_environment().render_targets;
	if (render_targets.preview_depth != 0) {
		glDeleteRenderbuffers(1, &render_targets.preview_depth);
		render_targets.preview_depth = 0;
	}
	if (render_targets.preview_color_ms != 0) {
		glDeleteRenderbuffers(1, &render_targets.preview_color_ms);
		render_targets.preview_color_ms = 0;
	}
	if (render_targets.preview_color != 0) {
		glDeleteTextures(1, &render_targets.preview_color);
		render_targets.preview_color = 0;
	}
	if (render_targets.preview_resolve_fbo != 0) {
		glDeleteFramebuffers(1, &render_targets.preview_resolve_fbo);
		render_targets.preview_resolve_fbo = 0;
	}
	if (render_targets.preview_fbo != 0) {
		glDeleteFramebuffers(1, &render_targets.preview_fbo);
		render_targets.preview_fbo = 0;
	}
	render_targets.preview_width = 0;
	render_targets.preview_height = 0;
}

void destroy_shadow_target()
{
	PreviewRenderTargetRepository &render_targets = preview_runtime_environment().render_targets;
	if (render_targets.shadow_depth_texture != 0) {
		glDeleteTextures(1, &render_targets.shadow_depth_texture);
		render_targets.shadow_depth_texture = 0;
	}
	if (render_targets.shadow_fbo != 0) {
		glDeleteFramebuffers(1, &render_targets.shadow_fbo);
		render_targets.shadow_fbo = 0;
	}
}

bool ensure_preview_target(int width, int height)
{
	PreviewRenderTargetRepository &render_targets = preview_runtime_environment().render_targets;
	width = std::max(width, 1);
	height = std::max(height, 1);
	if (width == render_targets.preview_width &&
	    height == render_targets.preview_height &&
	    render_targets.preview_fbo != 0) {
		return true;
	}

	destroy_preview_target();

	glGenFramebuffers(1, &render_targets.preview_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, render_targets.preview_fbo);

	glGenRenderbuffers(1, &render_targets.preview_color_ms);
	glBindRenderbuffer(GL_RENDERBUFFER, render_targets.preview_color_ms);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, kPreviewMsaaSamples, GL_RGBA8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER,
	                          GL_COLOR_ATTACHMENT0,
	                          GL_RENDERBUFFER,
	                          render_targets.preview_color_ms);

	glGenRenderbuffers(1, &render_targets.preview_depth);
	glBindRenderbuffer(GL_RENDERBUFFER, render_targets.preview_depth);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, kPreviewMsaaSamples, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER,
	                          GL_DEPTH_STENCIL_ATTACHMENT,
	                          GL_RENDERBUFFER,
	                          render_targets.preview_depth);
	const GLenum draw_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	glGenFramebuffers(1, &render_targets.preview_resolve_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, render_targets.preview_resolve_fbo);
	glGenTextures(1, &render_targets.preview_color);
	glBindTexture(GL_TEXTURE_2D, render_targets.preview_color);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER,
	                       GL_COLOR_ATTACHMENT0,
	                       GL_TEXTURE_2D,
	                       render_targets.preview_color,
	                       0);
	glGenerateMipmap(GL_TEXTURE_2D);
	const GLenum resolve_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	if (draw_status != GL_FRAMEBUFFER_COMPLETE || resolve_status != GL_FRAMEBUFFER_COMPLETE) {
		debugout("Preview framebuffer is incomplete");
		destroy_preview_target();
		return false;
	}

	render_targets.preview_width = width;
	render_targets.preview_height = height;
	return true;
}

bool ensure_shadow_target()
{
	PreviewRenderTargetRepository &render_targets = preview_runtime_environment().render_targets;
	if (render_targets.shadow_fbo != 0 && render_targets.shadow_depth_texture != 0) {
		return true;
	}

	destroy_shadow_target();

	glGenFramebuffers(1, &render_targets.shadow_fbo);
	glGenTextures(1, &render_targets.shadow_depth_texture);
	glBindTexture(GL_TEXTURE_2D, render_targets.shadow_depth_texture);
	glTexImage2D(GL_TEXTURE_2D,
	             0,
	             GL_DEPTH_COMPONENT24,
	             kShadowMapResolution,
	             kShadowMapResolution,
	             0,
	             GL_DEPTH_COMPONENT,
	             GL_FLOAT,
	             nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	const GLfloat border_color[] = {1.0f, 1.0f, 1.0f, 1.0f};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);

	glBindFramebuffer(GL_FRAMEBUFFER, render_targets.shadow_fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER,
	                       GL_DEPTH_ATTACHMENT,
	                       GL_TEXTURE_2D,
	                       render_targets.shadow_depth_texture,
	                       0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		debugout("Shadow framebuffer is incomplete");
		destroy_shadow_target();
		return false;
	}
	return true;
}

void set_common_uniforms(GLuint program,
	                     const CameraMatrices &camera,
	                     const PreviewLightingFrame &lighting_frame,
	                     float preview_time,
	                     float scale_global,
	                     PreviewTextureDebugView debug_view,
	                     const PreviewRenderSettings &settings,
	                     const char *model_uniform_name = "model")
{
	mat4 model = mat4(1.0f);
	model = scale(model, vec3(scale_global, scale_global, scale_global));
	const GLint model_location = glGetUniformLocation(program, model_uniform_name);
	if (model_location >= 0) {
		glUniformMatrix4fv(model_location, 1, GL_FALSE, &model[0][0]);
	}
	glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, &camera.view[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, &camera.projection[0][0]);

	const GLint light_count_location = glGetUniformLocation(program, "preview_light_count");
	if (light_count_location >= 0) {
		glUniform1ui(
			light_count_location,
			static_cast<GLuint>(lighting_frame.gpu_light_records.size()));
	}
	const GLint shadows_enabled_location = glGetUniformLocation(program, "shadows_enabled");
	if (shadows_enabled_location >= 0) {
		glUniform1i(
			shadows_enabled_location,
			settings.shadows && lighting_frame.primary_shadow_available ? 1 : 0);
	}
	glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, &camera.position[0]);
	const GLint preview_time_location = glGetUniformLocation(program, "preview_time");
	if (preview_time_location >= 0) {
		glUniform1f(preview_time_location, preview_time);
	}
	const GLint light_view_projection_location =
		glGetUniformLocation(program, "light_view_projection");
	if (light_view_projection_location >= 0) {
		glUniformMatrix4fv(light_view_projection_location,
		                   1,
		                   GL_FALSE,
		                   &lighting_frame.primary_shadow_view_projection[0][0]);
	}
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, preview_runtime_environment().render_targets.shadow_depth_texture);
	const GLint shadow_map_location = glGetUniformLocation(program, "shadow_map");
	if (shadow_map_location >= 0) {
		glUniform1i(shadow_map_location, 1);
	}
	glActiveTexture(GL_TEXTURE15);
	glBindTexture(GL_TEXTURE_CUBE_MAP,
	              settings.cubemap_enabled ? settings.cubemap_texture : 0);
	const GLint environment_map_location = glGetUniformLocation(program, "environment_map");
	if (environment_map_location >= 0) {
		glUniform1i(environment_map_location, 15);
	}
	const GLint environment_map_enabled_location =
		glGetUniformLocation(program, "environment_map_enabled");
	if (environment_map_enabled_location >= 0) {
		glUniform1i(environment_map_enabled_location,
		            settings.cubemap_enabled && settings.cubemap_texture != 0 ? 1 : 0);
	}
	const GLint environment_intensity_location =
		glGetUniformLocation(program, "environment_intensity");
	if (environment_intensity_location >= 0) {
		glUniform1f(environment_intensity_location,
		            std::max(settings.cubemap_intensity, 0.0f));
	}
	glActiveTexture(GL_TEXTURE0);

	const glm::vec4 ambientproduct = glm::vec4(0.58f, 0.58f, 0.60f, 1.0f);
	const glm::vec4 diffuseproduct = glm::vec4(0.94f, 0.94f, 0.96f, 1.0f);
	const glm::vec4 specularproduct = glm::vec4(0.20f, 0.20f, 0.20f, 1.0f);
	glUniform4fv(glGetUniformLocation(program, "ambientproduct"), 1, &ambientproduct[0]);
	glUniform4fv(glGetUniformLocation(program, "diffuseproduct"), 1, &diffuseproduct[0]);
	glUniform4fv(glGetUniformLocation(program, "specularproduct"), 1, &specularproduct[0]);
	glUniform1f(glGetUniformLocation(program, "shinyness"), 36.0f);
	const vec3 fog_color(0.82f, 0.87f, 0.94f);
	glUniform3fv(glGetUniformLocation(program, "fog_color"), 1, &fog_color[0]);
	glUniform1f(glGetUniformLocation(program, "fog_start"), std::max(0.9f, camera.orbit_distance * 0.46f));
	glUniform1f(glGetUniformLocation(program,
	                                 "fog_density"),
	            0.050f / std::max(camera.orbit_distance * 0.28f, 1.0f));
	const GLint debug_view_location = glGetUniformLocation(program, "debug_view");
	if (debug_view_location >= 0) {
		glUniform1i(debug_view_location, static_cast<int>(debug_view));
	}
}

void draw_background()
{
	PreviewShaderProgramLibrary &shader_programs = preview_runtime_environment().shader_programs;
	PreviewGeometryRepository &geometry_repository = preview_runtime_environment().geometry_repository;
	if (shader_programs.background_program == 0 || geometry_repository.background_vao == 0) {
		return;
	}

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDisable(GL_BLEND);
	glUseProgram(shader_programs.background_program);
	glBindVertexArray(geometry_repository.background_vao);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glDepthMask(GL_TRUE);
}

void draw_grid(const CameraMatrices &camera)
{
	PreviewShaderProgramLibrary &shader_programs = preview_runtime_environment().shader_programs;
	PreviewGeometryRepository &geometry_repository = preview_runtime_environment().geometry_repository;
	if (shader_programs.grid_program == 0 ||
	    geometry_repository.grid_vao == 0 ||
	    geometry_repository.grid_vertex_count <= 0) {
		return;
	}

	glUseProgram(shader_programs.grid_program);
	glUniformMatrix4fv(glGetUniformLocation(shader_programs.grid_program, "view"), 1, GL_FALSE, &camera.view[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shader_programs.grid_program, "projection"), 1, GL_FALSE, &camera.projection[0][0]);
	glUniform4f(glGetUniformLocation(shader_programs.grid_program, "grid_color"), 0.36f, 0.39f, 0.46f, 1.0f);
	glBindVertexArray(geometry_repository.grid_vao);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glDrawArrays(GL_LINES, 0, geometry_repository.grid_vertex_count);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
}

void draw_overlay_lines(const CameraMatrices &camera, float scale_global)
{
	PreviewShaderProgramLibrary &shader_programs = preview_runtime_environment().shader_programs;
	PreviewGeometryRepository &geometry_repository = preview_runtime_environment().geometry_repository;
	if (shader_programs.overlay_program == 0 ||
	    geometry_repository.overlay_vao == 0 ||
	    geometry_repository.overlay_vertex_count <= 0) {
		return;
	}

	mat4 model = mat4(1.0f);
	model = scale(model, vec3(scale_global, scale_global, scale_global));

	glUseProgram(shader_programs.overlay_program);
	glUniformMatrix4fv(glGetUniformLocation(shader_programs.overlay_program, "view"), 1, GL_FALSE, &camera.view[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shader_programs.overlay_program, "projection"), 1, GL_FALSE, &camera.projection[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shader_programs.overlay_program, "model"), 1, GL_FALSE, &model[0][0]);
	glBindVertexArray(geometry_repository.overlay_vao);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glUniform1f(glGetUniformLocation(shader_programs.overlay_program, "alpha_scale"), 1.0f);
	glDrawArrays(GL_LINES, 0, geometry_repository.overlay_vertex_count);

	glDisable(GL_DEPTH_TEST);
	glUniform1f(glGetUniformLocation(shader_programs.overlay_program, "alpha_scale"), 0.22f);
	glDrawArrays(GL_LINES, 0, geometry_repository.overlay_vertex_count);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glDisable(GL_LINE_SMOOTH);
}

void draw_outline_batches(const CameraMatrices &camera, float scale_global)
{
	PreviewShaderProgramLibrary &shader_programs = preview_runtime_environment().shader_programs;
	PreviewGeometryRepository &geometry_repository = preview_runtime_environment().geometry_repository;
	if (shader_programs.outline_program == 0 || geometry_repository.outline_vaos.empty()) {
		return;
	}

	mat4 scene_model(1.0f);
	scene_model = scale(scene_model, vec3(scale_global, scale_global, scale_global));

	glUseProgram(shader_programs.outline_program);
	glUniformMatrix4fv(glGetUniformLocation(shader_programs.outline_program, "view"), 1, GL_FALSE, &camera.view[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shader_programs.outline_program, "projection"), 1, GL_FALSE, &camera.projection[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shader_programs.outline_program, "scene_model"), 1, GL_FALSE, &scene_model[0][0]);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	auto scaled_outline_color = [](const glm::vec4 &color, float alpha_scale) {
		return glm::vec4(color.r, color.g, color.b, std::clamp(color.a * alpha_scale, 0.0f, 1.0f));
	};

	glDisable(GL_DEPTH_TEST);
	for (std::size_t index = 0; index < geometry_repository.outline_vaos.size(); ++index) {
		if (geometry_repository.outline_counts[index] <= 0) {
			continue;
		}
		const glm::vec4 halo_color = scaled_outline_color(geometry_repository.outline_colors[index], 0.34f);
		glUniform4fv(glGetUniformLocation(shader_programs.outline_program, "outline_color"), 1, &halo_color[0]);
		glUniform1f(glGetUniformLocation(shader_programs.outline_program, "outline_width"),
		            geometry_repository.outline_widths[index] * 1.55f);
		glBindVertexArray(geometry_repository.outline_vaos[index]);
		glDrawArrays(GL_TRIANGLES, 0, geometry_repository.outline_counts[index]);
	}

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	for (std::size_t index = 0; index < geometry_repository.outline_vaos.size(); ++index) {
		if (geometry_repository.outline_counts[index] <= 0) {
			continue;
		}
		glUniform4fv(glGetUniformLocation(shader_programs.outline_program,
		                                  "outline_color"),
		             1,
		             &geometry_repository.outline_colors[index][0]);
		glUniform1f(glGetUniformLocation(shader_programs.outline_program, "outline_width"),
		            geometry_repository.outline_widths[index]);
		glBindVertexArray(geometry_repository.outline_vaos[index]);
		glDrawArrays(GL_TRIANGLES, 0, geometry_repository.outline_counts[index]);
	}

	glDepthMask(GL_TRUE);
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);
}

void apply_material_uniforms(GLuint program,
                             const PreviewMaterial &material,
                             int mapping_mode_override = -1)
{
	const float reflectance = std::clamp(material.reflectance, 0.0f, 1.0f);
	const float smoothness = std::clamp(material.smoothness, 0.0f, 1.0f);
	const float opacity = std::clamp(material.opacity, 0.0f, 1.0f);
	const float height_scale = material.height_texture != 0
	                               ? std::clamp(material.height_scale, 0.0f, 0.25f)
	                               : 0.0f;
	const float emission_strength = material.emissive_texture != 0
	                                    ? std::max(material.emission_strength, 0.0f)
	                                    : 0.0f;
	const float specular_strength = 0.22f + reflectance * 0.78f;
	const glm::vec4 specularproduct(specular_strength, specular_strength, specular_strength, 1.0f);
	const int mapping_mode = mapping_mode_override >= 0 ? mapping_mode_override : material.mapping_mode;
	const float mapping_scale = std::max(material.mapping_scale, 1e-4f);
	const GLuint height_texture = material.height_texture != 0 ? material.height_texture : material.texture;
	const GLuint emissive_texture = material.emissive_texture != 0 ? material.emissive_texture : material.texture;
	const GLuint alpha_texture = material.alpha_texture != 0 ? material.alpha_texture : material.texture;
	const GLuint normal_texture = material.normal_texture != 0 ? material.normal_texture : material.texture;
	const GLuint roughness_texture = material.roughness_texture != 0 ? material.roughness_texture : material.texture;
	const GLuint metallic_texture = material.metallic_texture != 0 ? material.metallic_texture : material.texture;
	const GLuint ao_texture = material.ao_texture != 0 ? material.ao_texture : material.texture;
	const auto bind_texture_uniform = [&](GLenum texture_unit,
	                                     GLuint texture,
	                                     const char *uniform_name,
	                                     GLint sampler_index) {
		glActiveTexture(texture_unit);
		glBindTexture(GL_TEXTURE_2D, texture);
		const GLint location = glGetUniformLocation(program, uniform_name);
		if (location >= 0) {
			glUniform1i(location, sampler_index);
		}
	};

	bind_texture_uniform(GL_TEXTURE0, material.texture, "texture1", 0);
	bind_texture_uniform(GL_TEXTURE2, height_texture, "height_texture", 2);
	bind_texture_uniform(GL_TEXTURE3, emissive_texture, "emissive_texture", 3);
	bind_texture_uniform(GL_TEXTURE4, alpha_texture, "alpha_texture", 4);
	bind_texture_uniform(GL_TEXTURE5, normal_texture, "normal_texture", 5);
	bind_texture_uniform(GL_TEXTURE6, roughness_texture, "roughness_texture", 6);
	bind_texture_uniform(GL_TEXTURE7, metallic_texture, "metallic_texture", 7);
	bind_texture_uniform(GL_TEXTURE8, ao_texture, "ao_texture", 8);
	glActiveTexture(GL_TEXTURE0);
	glUniform4fv(glGetUniformLocation(program, "specularproduct"), 1, &specularproduct[0]);
	glUniform1f(glGetUniformLocation(program, "shinyness"), 22.0f + smoothness * 260.0f);
	glUniform1f(glGetUniformLocation(program, "alpha"), opacity);
	const GLint reflectance_location = glGetUniformLocation(program, "material_reflectance");
	if (reflectance_location >= 0) {
		glUniform1f(reflectance_location, reflectance);
	}
	const GLint smoothness_location = glGetUniformLocation(program, "material_smoothness");
	if (smoothness_location >= 0) {
		glUniform1f(smoothness_location, smoothness);
	}
	const GLint height_scale_location = glGetUniformLocation(program, "material_height_scale");
	if (height_scale_location >= 0) {
		glUniform1f(height_scale_location, height_scale);
	}
	const GLint emission_strength_location = glGetUniformLocation(program, "material_emission_strength");
	if (emission_strength_location >= 0) {
		glUniform1f(emission_strength_location, emission_strength);
	}
	const GLint uv_rotation_speed_location = glGetUniformLocation(program, "material_uv_rotation_speed");
	if (uv_rotation_speed_location >= 0) {
		glUniform1f(uv_rotation_speed_location, material.uv_rotation_speed);
	}
	const GLint uv_activity_location = glGetUniformLocation(program, "material_uv_activity");
	if (uv_activity_location >= 0) {
		glUniform1f(uv_activity_location, material.uv_activity);
	}
	const GLint pulse_strength_location = glGetUniformLocation(program, "material_pulse_strength");
	if (pulse_strength_location >= 0) {
		glUniform1f(pulse_strength_location, material.pulse_strength);
	}
	const auto set_material_float = [&](const char *uniform_name, float value) {
		const GLint location = glGetUniformLocation(program, uniform_name);
		if (location >= 0) {
			glUniform1f(location, value);
		}
	};
	set_material_float("material_metallic", std::clamp(material.metallic, 0.0f, 1.0f));
	set_material_float("material_roughness", std::clamp(material.roughness, 0.045f, 1.0f));
	set_material_float("material_ao", std::clamp(material.ao, 0.0f, 1.0f));
	set_material_float("material_subsurface", std::clamp(material.subsurface, 0.0f, 1.0f));
	set_material_float("material_anisotropic", std::clamp(material.anisotropic, 0.0f, 1.0f));
	set_material_float("material_sheen", std::clamp(material.sheen, 0.0f, 1.0f));
	set_material_float("material_clearcoat", std::clamp(material.clearcoat, 0.0f, 1.0f));
	set_material_float("material_clearcoat_roughness",
	                   std::clamp(material.clearcoat_roughness, 0.045f, 1.0f));
	set_material_float("material_normal_strength",
	                   std::clamp(material.normal_strength, 0.0f, 2.0f));
	set_material_float("material_transmission", std::clamp(material.transmission, 0.0f, 1.0f));
	set_material_float("material_index_of_refraction",
	                   std::clamp(material.index_of_refraction, 1.0f, 2.5f));
	set_material_float("material_thickness", std::clamp(material.thickness, 0.0f, 1.0f));
	const GLint attenuation_color_location =
		glGetUniformLocation(program, "material_attenuation_color");
	if (attenuation_color_location >= 0) {
		const glm::vec3 attenuation_color = glm::clamp(
			material.attenuation_color, glm::vec3(0.0f), glm::vec3(1.0f));
		glUniform3fv(attenuation_color_location, 1, &attenuation_color.x);
	}
	const GLint mapping_mode_location = glGetUniformLocation(program, "mapping_mode");
	if (mapping_mode_location >= 0) {
		glUniform1i(mapping_mode_location, mapping_mode);
	}
	const GLint mapping_scale_location = glGetUniformLocation(program, "mapping_scale");
	if (mapping_scale_location >= 0) {
		glUniform1f(mapping_scale_location, mapping_scale);
	}
	const GLint projection_axes_location = glGetUniformLocation(program, "projection_axes");
	if (projection_axes_location >= 0) {
		glUniform3fv(projection_axes_location, 3, &material.projection_axes[0].x);
	}
}

void draw_vertex_array(GLuint program,
                       GLuint vao,
                       GLsizei vertex_count,
                       const PreviewMaterial &material,
                       int mapping_mode_override = -1)
{
	if (vao == 0 || vertex_count <= 0) {
		return;
	}

	glUseProgram(program);
	apply_material_uniforms(program, material, mapping_mode_override);
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, vertex_count);
}

void draw_buffer(std::size_t index,
                 const std::vector<PreviewMaterial> &materials,
                 int mapping_mode_override = -1)
{
	PreviewGeometryRepository &geometry_repository = preview_runtime_environment().geometry_repository;
	PreviewShaderProgramLibrary &shader_programs = preview_runtime_environment().shader_programs;
	if (index >= geometry_repository.opaque_vaos.size() ||
	    index >= geometry_repository.opaque_render_counts.size() ||
	    index >= materials.size() ||
	    geometry_repository.opaque_render_counts[index] <= 0) {
		return;
	}

	draw_vertex_array(shader_programs.opaque_program,
	                 geometry_repository.opaque_vaos[index],
	                 static_cast<GLsizei>(geometry_repository.opaque_render_counts[index]),
	                 materials[index],
	                 mapping_mode_override);
}

void set_dynamic_cube_uniforms(GLuint program,
                               const PreviewDynamicCubeDraw &draw,
                               float simulation_alpha)
{
	const glm::mat3 previous_primary_normal_transform = safe_normal_transform(draw.previous_primary_transform);
	const glm::mat3 previous_secondary_normal_transform = safe_normal_transform(draw.previous_secondary_transform);
	const glm::mat3 current_primary_normal_transform = safe_normal_transform(draw.current_primary_transform);
	const glm::mat3 current_secondary_normal_transform = safe_normal_transform(draw.current_secondary_transform);

	glUniformMatrix4fv(glGetUniformLocation(program, "prev_primary_transform"),
	                   1,
	                   GL_FALSE,
	                   &draw.previous_primary_transform[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(program, "prev_secondary_transform"),
	                   1,
	                   GL_FALSE,
	                   &draw.previous_secondary_transform[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(program, "curr_primary_transform"),
	                   1,
	                   GL_FALSE,
	                   &draw.current_primary_transform[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(program, "curr_secondary_transform"),
	                   1,
	                   GL_FALSE,
	                   &draw.current_secondary_transform[0][0]);
	glUniformMatrix3fv(glGetUniformLocation(program, "prev_primary_normal_transform"),
	                   1,
	                   GL_FALSE,
	                   &previous_primary_normal_transform[0][0]);
	glUniformMatrix3fv(glGetUniformLocation(program, "prev_secondary_normal_transform"),
	                   1,
	                   GL_FALSE,
	                   &previous_secondary_normal_transform[0][0]);
	glUniformMatrix3fv(glGetUniformLocation(program, "curr_primary_normal_transform"),
	                   1,
	                   GL_FALSE,
	                   &current_primary_normal_transform[0][0]);
	glUniformMatrix3fv(glGetUniformLocation(program, "curr_secondary_normal_transform"),
	                   1,
	                   GL_FALSE,
	                   &current_secondary_normal_transform[0][0]);
	glUniform3fv(glGetUniformLocation(program, "dual_scales"),
	             3,
	             &draw.dual_scales[0].x);
	glUniform3fv(glGetUniformLocation(program, "dual_translations"),
	             3,
	             &draw.dual_translations[0].x);
	glUniform1f(glGetUniformLocation(program, "texscale"), draw.texscale);
	glUniform1f(glGetUniformLocation(program, "interpolation_alpha"), simulation_alpha);
	glUniform1i(glGetUniformLocation(program, "cube_mode"), static_cast<int>(draw.mode));
}

void draw_dynamic_cube(const PreviewDynamicCubeDraw &draw,
                       const PreviewMaterial &material,
                       float simulation_alpha,
                       int mapping_mode_override = -1)
{
	PreviewShaderProgramLibrary &shader_programs = preview_runtime_environment().shader_programs;
	PreviewGeometryRepository &geometry_repository = preview_runtime_environment().geometry_repository;
	if (shader_programs.dynamic_cube_program == 0 || geometry_repository.base_vao == 0) {
		return;
	}

	glUseProgram(shader_programs.dynamic_cube_program);
	apply_material_uniforms(shader_programs.dynamic_cube_program, material, mapping_mode_override);
	set_dynamic_cube_uniforms(shader_programs.dynamic_cube_program, draw, simulation_alpha);
	glBindVertexArray(geometry_repository.base_vao);
	glDrawArrays(GL_TRIANGLES, 0, 36);
}

void draw_dynamic_cube_opaque(float simulation_alpha,
                              const std::vector<PreviewMaterial> &materials,
                              int mapping_mode_override = -1)
{
	const std::vector<PreviewDynamicCubeDraw> &dynamic_cube_draws =
		preview_runtime_environment().geometry_repository.dynamic_cube_draws;
	for (const PreviewDynamicCubeDraw &draw : dynamic_cube_draws) {
		if (draw.material_index < 0 ||
		    static_cast<std::size_t>(draw.material_index) >= materials.size()) {
			continue;
		}
		const PreviewMaterial &material = materials[static_cast<std::size_t>(draw.material_index)];
		if (material.opacity < 0.999f) {
			continue;
		}
		draw_dynamic_cube(draw, material, simulation_alpha, mapping_mode_override);
	}
}

void set_shadow_common_uniforms(GLuint program,
                                const mat4 &light_view_projection,
                                float scale_global,
                                const char *model_uniform_name = "model")
{
	mat4 model = mat4(1.0f);
	model = scale(model, vec3(scale_global, scale_global, scale_global));
	const GLint model_location = glGetUniformLocation(program, model_uniform_name);
	if (model_location >= 0) {
		glUniformMatrix4fv(model_location, 1, GL_FALSE, &model[0][0]);
	}
	glUniformMatrix4fv(glGetUniformLocation(program, "light_view_projection"),
	                   1,
	                   GL_FALSE,
	                   &light_view_projection[0][0]);
}

void PreviewShadowMapPass::execute(PreviewRenderRuntimeEnvironment &runtime,
	                               const PreviewLightingFrame &lighting_frame,
                                   float scale_global,
                                   float simulation_alpha,
                                   const std::vector<PreviewMaterial> &materials) const
{
	PreviewShaderProgramLibrary &shader_programs = runtime.shader_programs;
	PreviewGeometryRepository &geometry_repository = runtime.geometry_repository;
	PreviewRenderTargetRepository &render_targets = runtime.render_targets;
	if (shader_programs.shadow_program == 0 || render_targets.shadow_fbo == 0 ||
	    !lighting_frame.primary_shadow_available) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, render_targets.shadow_fbo);
	glViewport(0, 0, kShadowMapResolution, kShadowMapResolution);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glClear(GL_DEPTH_BUFFER_BIT);

	glUseProgram(shader_programs.shadow_program);
	set_shadow_common_uniforms(
		shader_programs.shadow_program,
		lighting_frame.primary_shadow_view_projection,
		scale_global);
	for (std::size_t i = 0; i < geometry_repository.opaque_render_counts.size() && i < materials.size(); ++i) {
		if (materials[i].opacity < 0.999f ||
		    geometry_repository.opaque_render_counts[i] <= 0 ||
		    i >= geometry_repository.opaque_vaos.size()) {
			continue;
		}
		glBindVertexArray(geometry_repository.opaque_vaos[i]);
		glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(geometry_repository.opaque_render_counts[i]));
	}

	if (shader_programs.dynamic_cube_shadow_program != 0 && geometry_repository.base_vao != 0) {
		glUseProgram(shader_programs.dynamic_cube_shadow_program);
		set_shadow_common_uniforms(shader_programs.dynamic_cube_shadow_program,
		                           lighting_frame.primary_shadow_view_projection,
		                           scale_global,
		                           "scene_model");
		glBindVertexArray(geometry_repository.base_vao);
		for (const PreviewDynamicCubeDraw &draw : geometry_repository.dynamic_cube_draws) {
			if (draw.material_index < 0 ||
			    static_cast<std::size_t>(draw.material_index) >= materials.size() ||
			    materials[static_cast<std::size_t>(draw.material_index)].opacity < 0.999f) {
				continue;
			}
			set_dynamic_cube_uniforms(shader_programs.dynamic_cube_shadow_program, draw, simulation_alpha);
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}
	}

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);
	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void draw_transparent_batches(const CameraMatrices &camera,
                              float scale_global,
                              float simulation_alpha,
                              const std::vector<PreviewMaterial> &materials,
                              int mapping_mode_override = -1)
{
	PreviewGeometryRepository &geometry_repository = preview_runtime_environment().geometry_repository;
	PreviewShaderProgramLibrary &shader_programs = preview_runtime_environment().shader_programs;
	struct TransparentDrawItem {
		bool dynamic_cube = false;
		std::size_t index = 0;
		float depth = 0.0f;
	};

	std::vector<TransparentDrawItem> sorted_items;
	sorted_items.reserve(geometry_repository.transparent_counts.size() +
	                     geometry_repository.dynamic_cube_draws.size());
	for (std::size_t index = 0; index < geometry_repository.transparent_counts.size(); ++index) {
		if (index >= geometry_repository.transparent_material_indices.size() ||
		    index >= geometry_repository.transparent_vaos.size() ||
		    index >= geometry_repository.transparent_centers.size() ||
		    geometry_repository.transparent_counts[index] <= 0) {
			continue;
		}
		const int material_index = geometry_repository.transparent_material_indices[index];
		if (material_index < 0 ||
		    static_cast<std::size_t>(material_index) >= materials.size() ||
		    materials[static_cast<std::size_t>(material_index)].opacity >= 0.999f) {
			continue;
		}
		const glm::vec4 center =
			camera.view * glm::vec4(geometry_repository.transparent_centers[index] * scale_global, 1.0f);
		sorted_items.push_back({false, index, center.z});
	}
	for (std::size_t index = 0; index < geometry_repository.dynamic_cube_draws.size(); ++index) {
		const PreviewDynamicCubeDraw &draw = geometry_repository.dynamic_cube_draws[index];
		if (draw.material_index < 0 ||
		    static_cast<std::size_t>(draw.material_index) >= materials.size()) {
			continue;
		}
		if (materials[static_cast<std::size_t>(draw.material_index)].opacity >= 0.999f) {
			continue;
		}
		const glm::vec3 center = glm::mix(draw.previous_center, draw.current_center, simulation_alpha);
		const glm::vec4 view_center = camera.view * glm::vec4(center * scale_global, 1.0f);
		sorted_items.push_back({true, index, view_center.z});
	}

	if (sorted_items.empty()) {
		return;
	}

	std::sort(sorted_items.begin(), sorted_items.end(),
	          [](const TransparentDrawItem &a, const TransparentDrawItem &b) {
	              return a.depth < b.depth;
	          });

	for (const TransparentDrawItem &item : sorted_items) {
		if (item.dynamic_cube) {
			const PreviewDynamicCubeDraw &draw = geometry_repository.dynamic_cube_draws[item.index];
			draw_dynamic_cube(draw,
			                 materials[static_cast<std::size_t>(draw.material_index)],
			                 simulation_alpha,
			                 mapping_mode_override);
			continue;
		}
		const int material_index = geometry_repository.transparent_material_indices[item.index];
		draw_vertex_array(shader_programs.opaque_program,
		                 geometry_repository.transparent_vaos[item.index],
		                 geometry_repository.transparent_counts[item.index],
		                 materials[static_cast<std::size_t>(material_index)],
		                 mapping_mode_override);
	}
}


std::array<GLfloat, 36 * 8> build_base_vertex_data()
{
	std::array<GLfloat, 36 * 8> output{};
	for (int i = 0; i < 36 * 8; ++i) {
		output[i] = kVertexData[i];
	}
	for (int i = 0; i < 36; ++i) {
		for (int j = 0; j < 8; ++j) {
			if (j == 0 || j == 2) {
				output[i * 8 + j] *= 0.5f;
			}
			if (j == 1 && output[i * 8 + j] < 0.0f) {
				output[i * 8 + j] = 0.0f;
			}
		}
	}
	return output;
}

bool initialize_renderer()
{
	PreviewRenderRuntimeEnvironment &runtime = preview_runtime_environment();
	PreviewShaderProgramLibrary &shader_programs = runtime.shader_programs;
	PreviewGeometryRepository &geometry_repository = runtime.geometry_repository;
	if (shader_programs.opaque_program != 0) {
		return true;
	}
	if (!runtime.light_buffer.initialize()) {
		debugout("Unable to initialize the GL46 preview light SSBO");
		return false;
	}

	auto base_vertices = build_base_vertex_data();
	glGenVertexArrays(1, &geometry_repository.base_vao);
	glGenBuffers(1, &geometry_repository.base_buffer);
	setup_buffer_layout(geometry_repository.base_vao, geometry_repository.base_buffer);
	glBufferData(GL_ARRAY_BUFFER,
	             sizeof(GLfloat) * base_vertices.size(),
	             base_vertices.data(),
	             GL_STATIC_DRAW);

	const GLuint vertex = create_shader(GL_VERTEX_SHADER, VERTEX_SOURCE);
	if (vertex == 0) {
		return false;
	}
	const GLuint fragment = create_shader(GL_FRAGMENT_SHADER, FRAGMENT_SOURCE);
	if (fragment == 0) {
		glDeleteShader(vertex);
		return false;
	}

	shader_programs.opaque_program = glCreateProgram();
	glAttachShader(shader_programs.opaque_program, vertex);
	glAttachShader(shader_programs.opaque_program, fragment);
	glLinkProgram(shader_programs.opaque_program);
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	GLint status = GL_FALSE;
	glGetProgramiv(shader_programs.opaque_program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		GLint log_len = 0;
		glGetProgramiv(shader_programs.opaque_program, GL_INFO_LOG_LENGTH, &log_len);
		std::vector<char> buffer(static_cast<std::size_t>(std::max(log_len, 1)));
		glGetProgramInfoLog(shader_programs.opaque_program, log_len, NULL, buffer.data());
		debugout(std::string("Program link failure: ") + buffer.data());
		glDeleteProgram(shader_programs.opaque_program);
		shader_programs.opaque_program = 0;
		return false;
	}

	const GLuint dynamic_vertex = create_shader(GL_VERTEX_SHADER, DYNAMIC_CUBE_VERTEX_SOURCE);
	if (dynamic_vertex == 0) {
		return false;
	}
	const GLuint dynamic_fragment = create_shader(GL_FRAGMENT_SHADER, FRAGMENT_SOURCE);
	if (dynamic_fragment == 0) {
		glDeleteShader(dynamic_vertex);
		return false;
	}

	shader_programs.dynamic_cube_program = glCreateProgram();
	glAttachShader(shader_programs.dynamic_cube_program, dynamic_vertex);
	glAttachShader(shader_programs.dynamic_cube_program, dynamic_fragment);
	glLinkProgram(shader_programs.dynamic_cube_program);
	glDeleteShader(dynamic_vertex);
	glDeleteShader(dynamic_fragment);

	glGetProgramiv(shader_programs.dynamic_cube_program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		GLint log_len = 0;
		glGetProgramiv(shader_programs.dynamic_cube_program, GL_INFO_LOG_LENGTH, &log_len);
		std::vector<char> buffer(static_cast<std::size_t>(std::max(log_len, 1)));
		glGetProgramInfoLog(shader_programs.dynamic_cube_program, log_len, NULL, buffer.data());
		debugout(std::string("Dynamic cube program link failure: ") + buffer.data());
		glDeleteProgram(shader_programs.dynamic_cube_program);
		shader_programs.dynamic_cube_program = 0;
		return false;
	}

	const GLuint shadow_vertex = create_shader(GL_VERTEX_SHADER, SHADOW_VERTEX_SOURCE);
	if (shadow_vertex == 0) {
		return false;
	}
	const GLuint shadow_fragment = create_shader(GL_FRAGMENT_SHADER, SHADOW_FRAGMENT_SOURCE);
	if (shadow_fragment == 0) {
		glDeleteShader(shadow_vertex);
		return false;
	}

	shader_programs.shadow_program = glCreateProgram();
	glAttachShader(shader_programs.shadow_program, shadow_vertex);
	glAttachShader(shader_programs.shadow_program, shadow_fragment);
	glLinkProgram(shader_programs.shadow_program);
	glDeleteShader(shadow_vertex);
	glDeleteShader(shadow_fragment);

	glGetProgramiv(shader_programs.shadow_program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		GLint log_len = 0;
		glGetProgramiv(shader_programs.shadow_program, GL_INFO_LOG_LENGTH, &log_len);
		std::vector<char> buffer(static_cast<std::size_t>(std::max(log_len, 1)));
		glGetProgramInfoLog(shader_programs.shadow_program, log_len, NULL, buffer.data());
		debugout(std::string("Shadow program link failure: ") + buffer.data());
		glDeleteProgram(shader_programs.shadow_program);
		shader_programs.shadow_program = 0;
		return false;
	}

	const GLuint shadow_dynamic_vertex =
		create_shader(GL_VERTEX_SHADER, SHADOW_DYNAMIC_VERTEX_SOURCE);
	if (shadow_dynamic_vertex == 0) {
		return false;
	}
	const GLuint shadow_dynamic_fragment =
		create_shader(GL_FRAGMENT_SHADER, SHADOW_FRAGMENT_SOURCE);
	if (shadow_dynamic_fragment == 0) {
		glDeleteShader(shadow_dynamic_vertex);
		return false;
	}

	shader_programs.dynamic_cube_shadow_program = glCreateProgram();
	glAttachShader(shader_programs.dynamic_cube_shadow_program, shadow_dynamic_vertex);
	glAttachShader(shader_programs.dynamic_cube_shadow_program, shadow_dynamic_fragment);
	glLinkProgram(shader_programs.dynamic_cube_shadow_program);
	glDeleteShader(shadow_dynamic_vertex);
	glDeleteShader(shadow_dynamic_fragment);

	glGetProgramiv(shader_programs.dynamic_cube_shadow_program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		GLint log_len = 0;
		glGetProgramiv(shader_programs.dynamic_cube_shadow_program, GL_INFO_LOG_LENGTH, &log_len);
		std::vector<char> buffer(static_cast<std::size_t>(std::max(log_len, 1)));
		glGetProgramInfoLog(shader_programs.dynamic_cube_shadow_program, log_len, NULL, buffer.data());
		debugout(std::string("Dynamic cube shadow program link failure: ") + buffer.data());
		glDeleteProgram(shader_programs.dynamic_cube_shadow_program);
		shader_programs.dynamic_cube_shadow_program = 0;
		return false;
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	log_gl_errors("initialize_renderer");

	const GLuint grid_vertex = create_shader(GL_VERTEX_SHADER, GRID_VERTEX_SOURCE);
	if (grid_vertex == 0) {
		return false;
	}
	const GLuint grid_fragment = create_shader(GL_FRAGMENT_SHADER, GRID_FRAGMENT_SOURCE);
	if (grid_fragment == 0) {
		glDeleteShader(grid_vertex);
		return false;
	}

	shader_programs.grid_program = glCreateProgram();
	glAttachShader(shader_programs.grid_program, grid_vertex);
	glAttachShader(shader_programs.grid_program, grid_fragment);
	glLinkProgram(shader_programs.grid_program);
	glDeleteShader(grid_vertex);
	glDeleteShader(grid_fragment);

	glGetProgramiv(shader_programs.grid_program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		GLint log_len = 0;
		glGetProgramiv(shader_programs.grid_program, GL_INFO_LOG_LENGTH, &log_len);
		std::vector<char> buffer(static_cast<std::size_t>(std::max(log_len, 1)));
		glGetProgramInfoLog(shader_programs.grid_program, log_len, NULL, buffer.data());
		debugout(std::string("Grid program link failure: ") + buffer.data());
		glDeleteProgram(shader_programs.grid_program);
		shader_programs.grid_program = 0;
		return false;
	}

	const std::vector<GLfloat> grid_vertices = build_grid_vertices();
	geometry_repository.grid_vertex_count = static_cast<GLsizei>(grid_vertices.size() / 3);
	glGenVertexArrays(1, &geometry_repository.grid_vao);
	glGenBuffers(1, &geometry_repository.grid_buffer);
	glBindVertexArray(geometry_repository.grid_vao);
	glBindBuffer(GL_ARRAY_BUFFER, geometry_repository.grid_buffer);
	glBufferData(GL_ARRAY_BUFFER,
	             sizeof(GLfloat) * grid_vertices.size(),
	             grid_vertices.data(),
	             GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
	glEnableVertexAttribArray(0);

	const GLuint overlay_vertex = create_shader(GL_VERTEX_SHADER, OVERLAY_VERTEX_SOURCE);
	if (overlay_vertex == 0) {
		return false;
	}
	const GLuint overlay_fragment = create_shader(GL_FRAGMENT_SHADER, OVERLAY_FRAGMENT_SOURCE);
	if (overlay_fragment == 0) {
		glDeleteShader(overlay_vertex);
		return false;
	}

	shader_programs.overlay_program = glCreateProgram();
	glAttachShader(shader_programs.overlay_program, overlay_vertex);
	glAttachShader(shader_programs.overlay_program, overlay_fragment);
	glLinkProgram(shader_programs.overlay_program);
	glDeleteShader(overlay_vertex);
	glDeleteShader(overlay_fragment);

	glGetProgramiv(shader_programs.overlay_program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		GLint log_len = 0;
		glGetProgramiv(shader_programs.overlay_program, GL_INFO_LOG_LENGTH, &log_len);
		std::vector<char> buffer(static_cast<std::size_t>(std::max(log_len, 1)));
		glGetProgramInfoLog(shader_programs.overlay_program, log_len, NULL, buffer.data());
		debugout(std::string("Overlay program link failure: ") + buffer.data());
		glDeleteProgram(shader_programs.overlay_program);
		shader_programs.overlay_program = 0;
		return false;
	}

	glGenVertexArrays(1, &geometry_repository.overlay_vao);
	glGenBuffers(1, &geometry_repository.overlay_buffer);
	setup_overlay_buffer_layout(geometry_repository.overlay_vao, geometry_repository.overlay_buffer);
	glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	const GLuint background_vertex = create_shader(GL_VERTEX_SHADER, BACKGROUND_VERTEX_SOURCE);
	if (background_vertex == 0) {
		return false;
	}
	const GLuint background_fragment = create_shader(GL_FRAGMENT_SHADER, BACKGROUND_FRAGMENT_SOURCE);
	if (background_fragment == 0) {
		glDeleteShader(background_vertex);
		return false;
	}

	shader_programs.background_program = glCreateProgram();
	glAttachShader(shader_programs.background_program, background_vertex);
	glAttachShader(shader_programs.background_program, background_fragment);
	glLinkProgram(shader_programs.background_program);
	glDeleteShader(background_vertex);
	glDeleteShader(background_fragment);

	glGetProgramiv(shader_programs.background_program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		GLint log_len = 0;
		glGetProgramiv(shader_programs.background_program, GL_INFO_LOG_LENGTH, &log_len);
		std::vector<char> buffer(static_cast<std::size_t>(std::max(log_len, 1)));
		glGetProgramInfoLog(shader_programs.background_program, log_len, NULL, buffer.data());
		debugout(std::string("Background program link failure: ") + buffer.data());
		glDeleteProgram(shader_programs.background_program);
		shader_programs.background_program = 0;
		return false;
	}
	glGenVertexArrays(1, &geometry_repository.background_vao);

	const GLuint outline_vertex = create_shader(GL_VERTEX_SHADER, OUTLINE_VERTEX_SOURCE);
	if (outline_vertex == 0) {
		return false;
	}
	const GLuint outline_fragment = create_shader(GL_FRAGMENT_SHADER, OUTLINE_FRAGMENT_SOURCE);
	if (outline_fragment == 0) {
		glDeleteShader(outline_vertex);
		return false;
	}

	shader_programs.outline_program = glCreateProgram();
	glAttachShader(shader_programs.outline_program, outline_vertex);
	glAttachShader(shader_programs.outline_program, outline_fragment);
	glLinkProgram(shader_programs.outline_program);
	glDeleteShader(outline_vertex);
	glDeleteShader(outline_fragment);

	glGetProgramiv(shader_programs.outline_program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		GLint log_len = 0;
		glGetProgramiv(shader_programs.outline_program, GL_INFO_LOG_LENGTH, &log_len);
		std::vector<char> buffer(static_cast<std::size_t>(std::max(log_len, 1)));
		glGetProgramInfoLog(shader_programs.outline_program, log_len, NULL, buffer.data());
		debugout(std::string("Outline program link failure: ") + buffer.data());
		glDeleteProgram(shader_programs.outline_program);
		shader_programs.outline_program = 0;
		return false;
	}

	return true;
}

void shutdown_renderer()
{
	PreviewRenderRuntimeEnvironment &runtime = preview_runtime_environment();
	PreviewShaderProgramLibrary &shader_programs = runtime.shader_programs;
	PreviewGeometryRepository &geometry_repository = runtime.geometry_repository;
	runtime.light_buffer.shutdown();
	destroy_shadow_target();
	destroy_preview_target();
	for (std::size_t i = 0; i < geometry_repository.opaque_vaos.size(); ++i) {
		if (geometry_repository.opaque_buffers[i] != 0) {
			glDeleteBuffers(1, &geometry_repository.opaque_buffers[i]);
		}
		if (geometry_repository.opaque_vaos[i] != 0) {
			glDeleteVertexArrays(1, &geometry_repository.opaque_vaos[i]);
		}
	}
	geometry_repository.opaque_buffers.clear();
	geometry_repository.opaque_vaos.clear();
	geometry_repository.opaque_render_counts.clear();
	for (std::size_t i = 0; i < geometry_repository.transparent_vaos.size(); ++i) {
		if (geometry_repository.transparent_buffers[i] != 0) {
			glDeleteBuffers(1, &geometry_repository.transparent_buffers[i]);
		}
		if (geometry_repository.transparent_vaos[i] != 0) {
			glDeleteVertexArrays(1, &geometry_repository.transparent_vaos[i]);
		}
	}
	geometry_repository.transparent_buffers.clear();
	geometry_repository.transparent_vaos.clear();
	geometry_repository.transparent_counts.clear();
	geometry_repository.transparent_material_indices.clear();
	geometry_repository.transparent_centers.clear();
	for (std::size_t i = 0; i < geometry_repository.outline_vaos.size(); ++i) {
		if (geometry_repository.outline_buffers[i] != 0) {
			glDeleteBuffers(1, &geometry_repository.outline_buffers[i]);
		}
		if (geometry_repository.outline_vaos[i] != 0) {
			glDeleteVertexArrays(1, &geometry_repository.outline_vaos[i]);
		}
	}
	geometry_repository.outline_buffers.clear();
	geometry_repository.outline_vaos.clear();
	geometry_repository.outline_counts.clear();
	geometry_repository.outline_colors.clear();
	geometry_repository.outline_widths.clear();

	if (geometry_repository.base_buffer != 0) {
		glDeleteBuffers(1, &geometry_repository.base_buffer);
		geometry_repository.base_buffer = 0;
	}
	if (geometry_repository.base_vao != 0) {
		glDeleteVertexArrays(1, &geometry_repository.base_vao);
		geometry_repository.base_vao = 0;
	}
	if (shader_programs.opaque_program != 0) {
		glDeleteProgram(shader_programs.opaque_program);
		shader_programs.opaque_program = 0;
	}
	if (shader_programs.dynamic_cube_program != 0) {
		glDeleteProgram(shader_programs.dynamic_cube_program);
		shader_programs.dynamic_cube_program = 0;
	}
	if (shader_programs.shadow_program != 0) {
		glDeleteProgram(shader_programs.shadow_program);
		shader_programs.shadow_program = 0;
	}
	if (shader_programs.dynamic_cube_shadow_program != 0) {
		glDeleteProgram(shader_programs.dynamic_cube_shadow_program);
		shader_programs.dynamic_cube_shadow_program = 0;
	}
	geometry_repository.dynamic_cube_draws.clear();
	if (geometry_repository.grid_buffer != 0) {
		glDeleteBuffers(1, &geometry_repository.grid_buffer);
		geometry_repository.grid_buffer = 0;
	}
	if (geometry_repository.grid_vao != 0) {
		glDeleteVertexArrays(1, &geometry_repository.grid_vao);
		geometry_repository.grid_vao = 0;
	}
	if (shader_programs.grid_program != 0) {
		glDeleteProgram(shader_programs.grid_program);
		shader_programs.grid_program = 0;
	}
	geometry_repository.grid_vertex_count = 0;
	if (geometry_repository.overlay_buffer != 0) {
		glDeleteBuffers(1, &geometry_repository.overlay_buffer);
		geometry_repository.overlay_buffer = 0;
	}
	if (geometry_repository.overlay_vao != 0) {
		glDeleteVertexArrays(1, &geometry_repository.overlay_vao);
		geometry_repository.overlay_vao = 0;
	}
	if (shader_programs.overlay_program != 0) {
		glDeleteProgram(shader_programs.overlay_program);
		shader_programs.overlay_program = 0;
	}
	geometry_repository.overlay_vertex_count = 0;
	if (geometry_repository.background_vao != 0) {
		glDeleteVertexArrays(1, &geometry_repository.background_vao);
		geometry_repository.background_vao = 0;
	}
	if (shader_programs.background_program != 0) {
		glDeleteProgram(shader_programs.background_program);
		shader_programs.background_program = 0;
	}
	if (shader_programs.outline_program != 0) {
		glDeleteProgram(shader_programs.outline_program);
		shader_programs.outline_program = 0;
	}
}

void upload_render_buffers(const std::vector<std::vector<GLfloat>> &buffers,
                           const std::vector<int> &counts,
                           std::size_t material_capacity)
{
	PreviewGeometryRepository &geometry_repository = preview_runtime_environment().geometry_repository;
	ensure_geometry_capacity(material_capacity);
	geometry_repository.opaque_render_counts.assign(material_capacity, 0);

	for (std::size_t i = 0; i < material_capacity; ++i) {
		if (i < counts.size()) {
			geometry_repository.opaque_render_counts[i] = counts[i];
		}
		if (i < buffers.size() && !buffers[i].empty()) {
			glBindVertexArray(geometry_repository.opaque_vaos[i]);
			glBindBuffer(GL_ARRAY_BUFFER, geometry_repository.opaque_buffers[i]);
			glBufferData(GL_ARRAY_BUFFER,
			             sizeof(GLfloat) * buffers[i].size(),
			             buffers[i].data(),
			             GL_DYNAMIC_DRAW);
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	log_gl_errors("upload_render_buffers");
}

void upload_transparent_render_batches(const std::vector<PreviewTransparentBatch> &batches)
{
	PreviewGeometryRepository &geometry_repository = preview_runtime_environment().geometry_repository;
	ensure_transparent_batch_capacity(batches.size());
	geometry_repository.transparent_counts.assign(batches.size(), 0);
	geometry_repository.transparent_material_indices.assign(batches.size(), -1);
	geometry_repository.transparent_centers.assign(batches.size(), vec3(0.0f));

	for (std::size_t index = 0; index < batches.size(); ++index) {
		geometry_repository.transparent_material_indices[index] = batches[index].material_index;
		geometry_repository.transparent_centers[index] = batches[index].center;
		geometry_repository.transparent_counts[index] =
			static_cast<GLsizei>(batches[index].vertices.size() / 8u);
		glBindVertexArray(geometry_repository.transparent_vaos[index]);
		glBindBuffer(GL_ARRAY_BUFFER, geometry_repository.transparent_buffers[index]);
		glBufferData(GL_ARRAY_BUFFER,
		             static_cast<GLsizeiptr>(sizeof(GLfloat) * batches[index].vertices.size()),
		             batches[index].vertices.empty() ? nullptr : batches[index].vertices.data(),
		             GL_DYNAMIC_DRAW);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	log_gl_errors("upload_transparent_render_batches");
}

void upload_dynamic_cube_draws(const std::vector<PreviewDynamicCubeDraw> &draws)
{
	preview_runtime_environment().geometry_repository.dynamic_cube_draws = draws;
}

void upload_overlay_lines(const std::vector<PreviewOverlayVertex> &vertices)
{
	PreviewGeometryRepository &geometry_repository = preview_runtime_environment().geometry_repository;
	if (geometry_repository.overlay_vao == 0 || geometry_repository.overlay_buffer == 0) {
		return;
	}

	geometry_repository.overlay_vertex_count = static_cast<GLsizei>(vertices.size());
	glBindVertexArray(geometry_repository.overlay_vao);
	glBindBuffer(GL_ARRAY_BUFFER, geometry_repository.overlay_buffer);
	glBufferData(GL_ARRAY_BUFFER,
	             static_cast<GLsizeiptr>(sizeof(PreviewOverlayVertex) * vertices.size()),
	             vertices.empty() ? nullptr : vertices.data(),
	             GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	log_gl_errors("upload_overlay_lines");
}

void upload_outline_batches(const std::vector<PreviewOutlineBatch> &batches)
{
	PreviewGeometryRepository &geometry_repository = preview_runtime_environment().geometry_repository;
	ensure_outline_capacity(batches.size());
	if (geometry_repository.outline_counts.size() < batches.size()) {
		geometry_repository.outline_counts.resize(batches.size(), 0);
	}
	if (geometry_repository.outline_colors.size() < batches.size()) {
		geometry_repository.outline_colors.resize(batches.size(), vec4(1.0f));
	}
	if (geometry_repository.outline_widths.size() < batches.size()) {
		geometry_repository.outline_widths.resize(batches.size(), 0.03f);
	}

	for (std::size_t index = 0; index < geometry_repository.outline_counts.size(); ++index) {
		geometry_repository.outline_counts[index] = 0;
	}
	for (std::size_t index = 0; index < batches.size(); ++index) {
		geometry_repository.outline_counts[index] =
			static_cast<GLsizei>(batches[index].vertices.size() / 8u);
		geometry_repository.outline_colors[index] = batches[index].color;
		geometry_repository.outline_widths[index] = batches[index].width;
		glBindVertexArray(geometry_repository.outline_vaos[index]);
		glBindBuffer(GL_ARRAY_BUFFER, geometry_repository.outline_buffers[index]);
		glBufferData(GL_ARRAY_BUFFER,
		             static_cast<GLsizeiptr>(sizeof(GLfloat) * batches[index].vertices.size()),
		             batches[index].vertices.empty() ? nullptr : batches[index].vertices.data(),
		             GL_DYNAMIC_DRAW);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	log_gl_errors("upload_outline_batches");
}


void PreviewScenePass::execute(PreviewRenderRuntimeEnvironment &runtime,
                               const CameraMatrices &camera,
	                           const PreviewLightingFrame &lighting_frame,
                               const PreviewRenderSettings &settings,
                               float scale_global,
                               const std::vector<PreviewMaterial> &materials) const
{
	PreviewShaderProgramLibrary &shader_programs = runtime.shader_programs;
	PreviewGeometryRepository &geometry_repository = runtime.geometry_repository;
	PreviewRenderTargetRepository &render_targets = runtime.render_targets;
	glBindFramebuffer(GL_FRAMEBUFFER, render_targets.preview_fbo);
	glViewport(0, 0, render_targets.preview_width, render_targets.preview_height);

	// Apply anti-aliasing settings
	if (settings.anti_aliasing >= 2) { // MSAA modes
		glEnable(GL_MULTISAMPLE);
	} else if (settings.anti_aliasing == 1) { // FXAA mode
		glDisable(GL_MULTISAMPLE);
		// FXAA would be applied in post-processing
	} else {
		glDisable(GL_MULTISAMPLE);
	}

	// Apply render settings immediately for live preview
	// Note: These settings are passed from the UI and take effect instantly

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDisable(GL_CULL_FACE);
	glClearColor(0.75f, 0.81f, 0.90f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	draw_background();
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	draw_grid(camera);
	runtime.light_buffer.bind();

	// Apply bloom settings immediately (framebuffer setup)
	if (settings.bloom_enabled) {
		// Bloom would be applied in post-processing pass
		// For now, we set up the framebuffer for potential bloom
	}

	// Apply ambient occlusion immediately
	// This affects the ambient lighting calculations

	if (shader_programs.opaque_program != 0) {
		glUseProgram(shader_programs.opaque_program);
		set_common_uniforms(shader_programs.opaque_program,
		                    camera,
		                    lighting_frame,
		                    settings.preview_time,
			                    scale_global,
			                    settings.debug_view,
			                    settings);
	}
	if (shader_programs.dynamic_cube_program != 0 && !geometry_repository.dynamic_cube_draws.empty()) {
		glUseProgram(shader_programs.dynamic_cube_program);
		set_common_uniforms(shader_programs.dynamic_cube_program,
		                    camera,
		                    lighting_frame,
		                    settings.preview_time,
			                    scale_global,
			                    settings.debug_view,
			                    settings,
			                    "scene_model");
	}

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	for (std::size_t i = 0; i < geometry_repository.opaque_render_counts.size() && i < materials.size(); ++i) {
		if (materials[i].opacity >= 0.999f) {
			draw_buffer(i, materials, settings.mapping_mode_override);
		}
	}
	draw_dynamic_cube_opaque(settings.simulation_alpha, materials, settings.mapping_mode_override);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	draw_transparent_batches(camera,
	                         scale_global,
	                         settings.simulation_alpha,
	                         materials,
	                         settings.mapping_mode_override);
	draw_outline_batches(camera, scale_global);

	draw_overlay_lines(camera, scale_global);

	glDepthMask(GL_TRUE);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, render_targets.preview_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, render_targets.preview_resolve_fbo);
	glBlitFramebuffer(0,
	                  0,
	                  render_targets.preview_width,
	                  render_targets.preview_height,
	                  0,
	                  0,
	                  render_targets.preview_width,
	                  render_targets.preview_height,
	                  GL_COLOR_BUFFER_BIT,
	                  GL_NEAREST);

	// Clean up cubemap binding after use
	if (settings.cubemap_enabled && settings.cubemap_texture != 0) {
		glActiveTexture(GL_TEXTURE15);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		glActiveTexture(GL_TEXTURE0);
	}
}

void PreviewResolvePass::execute(PreviewRenderRuntimeEnvironment &runtime) const
{
	PreviewRenderTargetRepository &render_targets = runtime.render_targets;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, render_targets.preview_color);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void render_scene_to_preview(const PreviewFrameRequest &frame_request)
{
	PreviewRenderRuntimeEnvironment &runtime = preview_runtime_environment();
	PreviewShaderProgramLibrary &shader_programs = runtime.shader_programs;
	PreviewRenderTargetRepository &render_targets = runtime.render_targets;
	if (frame_request.materials == nullptr ||
	    frame_request.lights == nullptr ||
	    shader_programs.opaque_program == 0 ||
	    !ensure_preview_target(frame_request.camera.width, frame_request.camera.height) ||
	    !ensure_shadow_target()) {
		return;
	}

	const CameraMatrices camera = build_camera_matrices(render_targets.preview_width,
	                                                    render_targets.preview_height,
	                                                    frame_request.camera.camera_distance,
	                                                    frame_request.camera.angle_view,
	                                                    frame_request.camera.elevation_view,
	                                                    frame_request.camera.roll_view,
	                                                    frame_request.camera.vertical_field_of_view_radians,
	                                                    frame_request.camera.target,
	                                                    frame_request.camera.scale_global);
	const PreviewLightCameraContext light_camera = build_preview_light_camera_context(
		camera, frame_request.camera.scale_global);
	const PreviewLightingFrame lighting_frame = PreviewLightingFrameFactory().build(
		*frame_request.lights, light_camera);
	runtime.light_buffer.upload(lighting_frame.gpu_light_records);
	static const PreviewShadowMapPass shadow_map_pass;
	static const PreviewScenePass preview_scene_pass;
	static const PreviewResolvePass preview_resolve_pass;

	if (frame_request.settings.shadows) {
		shadow_map_pass.execute(runtime,
		                        lighting_frame,
		                        frame_request.camera.scale_global,
		                        frame_request.settings.simulation_alpha,
		                        *frame_request.materials);
	}
	preview_scene_pass.execute(runtime,
	                           camera,
	                           lighting_frame,
	                           frame_request.settings,
	                           frame_request.camera.scale_global,
	                           *frame_request.materials);
	preview_resolve_pass.execute(runtime);
	log_gl_errors("render_scene_to_preview");
}


GLuint generate_procedural_cubemap(int resolution, float intensity)
{
	if (resolution <= 0) resolution = 512;
	if (intensity <= 0.0f) intensity = 1.0f;

	// Validate resolution limits
	GLint max_cube_map_resolution;
	glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &max_cube_map_resolution);
	if (resolution > max_cube_map_resolution) {
		resolution = max_cube_map_resolution;
	}

	// Save current OpenGL state
	GLint current_texture = 0;
	glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &current_texture);

	GLuint cubemap_texture = 0;
	glGenTextures(1, &cubemap_texture);

	// Check for texture generation errors
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		debugout("generate_procedural_cubemap: glGenTextures error - " + std::to_string(error));
		return 0;
	}

	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture);

	// Check for binding errors
	error = glGetError();
	if (error != GL_NO_ERROR) {
		debugout("generate_procedural_cubemap: glBindTexture error - " + std::to_string(error));
		glDeleteTextures(1, &cubemap_texture);
		glBindTexture(GL_TEXTURE_CUBE_MAP, current_texture);
		return 0;
	}

	// Generate 6 faces for the cubemap
	for (int face = 0; face < 6; ++face) {
		std::vector<GLfloat> face_data(resolution * resolution * 4);

		for (int y = 0; y < resolution; ++y) {
			for (int x = 0; x < resolution; ++x) {
				// Normalize coordinates to [-1, 1]
				float u = 2.0f * (x + 0.5f) / resolution - 1.0f;
				float v = 2.0f * (y + 0.5f) / resolution - 1.0f;

				// Calculate direction vector for this face
				glm::vec3 dir(0.0f, 0.0f, 0.0f);
				switch (face) {
					case 0: dir = glm::vec3(1.0f, -v, -u); break;  // GL_TEXTURE_CUBE_MAP_POSITIVE_X
					case 1: dir = glm::vec3(-1.0f, -v, u); break;  // GL_TEXTURE_CUBE_MAP_NEGATIVE_X
					case 2: dir = glm::vec3(u, 1.0f, v); break;     // GL_TEXTURE_CUBE_MAP_POSITIVE_Y
					case 3: dir = glm::vec3(u, -1.0f, -v); break;   // GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
					case 4: dir = glm::vec3(u, -v, 1.0f); break;    // GL_TEXTURE_CUBE_MAP_POSITIVE_Z
					case 5: dir = glm::vec3(-u, -v, -1.0f); break;  // GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
				}

				dir = glm::normalize(dir);

				// Generate procedural sky color
				glm::vec3 sky_color(0.75f, 0.81f, 0.90f);  // Base sky color
				glm::vec3 horizon_color(0.85f, 0.88, 0.92);
				glm::vec3 zenith_color(0.55f, 0.68, 0.95);

				// Calculate height factor (0 = horizon, 1 = zenith)
				float height_factor = glm::max(0.0f, dir.y);

				// Gradient from horizon to zenith
				glm::vec3 color = glm::mix(horizon_color, zenith_color, height_factor * height_factor);

				// Add sun
				glm::vec3 sun_dir = glm::normalize(glm::vec3(0.5f, 0.8f, 0.3));
				float sun_dot = glm::max(0.0f, glm::dot(dir, sun_dir));
				float sun_intensity = glm::pow(sun_dot, 32.0f) * intensity;

				// Sun glow
				float sun_glow = glm::pow(sun_dot, 4.0f) * 0.5f * intensity;
				color += glm::vec3(1.0f, 0.95f, 0.8) * sun_intensity;
				color += glm::vec3(1.0f, 0.9f, 0.7f) * sun_glow;

				// Add atmospheric haze near horizon
				float haze_factor = 1.0f - glm::abs(dir.y);
				haze_factor = glm::pow(haze_factor, 3.0f);
				color = glm::mix(color, horizon_color, haze_factor * 0.3f);

				// Add subtle ground reflection
				if (dir.y < 0.0f) {
					glm::vec3 ground_color(0.15f, 0.18f, 0.12f);
					float ground_blend = glm::pow(-dir.y, 0.5f);
					color = glm::mix(color, ground_color, ground_blend * 0.5f);
				}

				// Clamp and store
				color = glm::clamp(color, glm::vec3(0.0f), glm::vec3(1.0f));

				int pixel_index = (y * resolution + x) * 4;
				face_data[pixel_index + 0] = color.r;
				face_data[pixel_index + 1] = color.g;
				face_data[pixel_index + 2] = color.b;
				face_data[pixel_index + 3] = 1.0f;
			}
		}

		// Upload face data with error checking
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
				           0,
				           GL_RGBA16F,
				           resolution,
				           resolution,
				           0,
				           GL_RGBA,
				           GL_FLOAT,
				           face_data.data());

		// Check for upload errors
		error = glGetError();
		if (error != GL_NO_ERROR) {
			debugout("generate_procedural_cubemap: glTexImage2D error for face " +
			        std::to_string(face) + " - " + std::to_string(error));

			// Try fallback to GL_RGBA8 format
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
			           0,
			           GL_RGBA8,
			           resolution,
			           resolution,
			           0,
			           GL_RGBA,
			           GL_UNSIGNED_BYTE,
			           face_data.data());

			error = glGetError();
			if (error != GL_NO_ERROR) {
				debugout("generate_procedural_cubemap: Fallback also failed for face " +
				        std::to_string(face) + " - " + std::to_string(error));
				glDeleteTextures(1, &cubemap_texture);
				glBindTexture(GL_TEXTURE_CUBE_MAP, current_texture);
				return 0;
			}
		}
	}

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	// Generate mipmaps with error checking
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		debugout("generate_procedural_cubemap: glGenerateMipmap error - " + std::to_string(error));
		// Continue anyway, mipmaps aren't critical
	}

	// Restore previous OpenGL state
	glBindTexture(GL_TEXTURE_CUBE_MAP, current_texture);

	// Make sure we don't leave texture unit 15 bound (used for cubemap environment mapping)
	glActiveTexture(GL_TEXTURE15);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glActiveTexture(GL_TEXTURE0);

	// Final error check
	GLenum final_error = glGetError();
	if (final_error != GL_NO_ERROR) {
		debugout("generate_procedural_cubemap: Final error check - 0x" + std::to_string(final_error));
	}

	log_gl_errors("generate_procedural_cubemap");

	debugout("Generated cubemap successfully: " + std::to_string(resolution) + "x" + std::to_string(resolution));

	return cubemap_texture;
}

GLuint generate_enhanced_procedural_cubemap(int resolution, float intensity)
{
	if (resolution <= 0) resolution = 512;
	if (intensity <= 0.0f) intensity = 1.0f;

	// Validate resolution limits
	GLint max_cube_map_resolution;
	glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &max_cube_map_resolution);
	if (resolution > max_cube_map_resolution) {
		resolution = max_cube_map_resolution;
		debugout("Resolution limited to GPU maximum: " + std::to_string(resolution));
	}

	// Save current OpenGL state
	GLint current_texture = 0;
	glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &current_texture);

	GLuint cubemap_texture = 0;
	glGenTextures(1, &cubemap_texture);

	// Check for texture generation errors
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		debugout("generate_enhanced_cubemap: glGenTextures error 0x" + std::to_string(error));
		return 0;
	}

	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture);

	// Check for binding errors
	error = glGetError();
	if (error != GL_NO_ERROR) {
		debugout("generate_enhanced_cubemap: glBindTexture error 0x" + std::to_string(error));
		glDeleteTextures(1, &cubemap_texture);
		glBindTexture(GL_TEXTURE_CUBE_MAP, current_texture);
		return 0;
	}

	// Enhanced atmospheric parameters
	glm::vec3 sky_zenith(0.42f, 0.62f, 0.92f);    // Rich blue
	glm::vec3 sky_horizon(0.78f, 0.84f, 0.94f);  // Light blue-white
	glm::vec3 ground_near(0.22f, 0.25f, 0.18f);  // Dark earth
	glm::vec3 ground_far(0.12f, 0.13f, 0.11f);   // Distant ground
	glm::vec3 sun_color(1.0f, 0.95f, 0.85f);     // Warm sunlight
	glm::vec3 sunset_color(1.0f, 0.55f, 0.25f);   // Sunset orange

	// Improved sun position
	glm::vec3 sun_dir = glm::normalize(glm::vec3(0.7f, 0.4f, 0.9f));

	// Generate 6 faces with enhanced atmospheric rendering
	for (int face = 0; face < 6; ++face) {
		std::vector<GLfloat> face_data(resolution * resolution * 4);

		for (int y = 0; y < resolution; ++y) {
			for (int x = 0; x < resolution; ++x) {
				// Normalize coordinates to [-1, 1]
				float u = 2.0f * (x + 0.5f) / resolution - 1.0f;
				float v = 2.0f * (y + 0.5f) / resolution - 1.0f;

				// Calculate direction vector for this face
				glm::vec3 dir(0.0f, 0.0f, 0.0f);
				switch (face) {
					case 0: dir = glm::vec3(1.0f, -v, -u); break;
					case 1: dir = glm::vec3(-1.0f, -v, u); break;
					case 2: dir = glm::vec3(u, 1.0f, v); break;
					case 3: dir = glm::vec3(u, -1.0f, -v); break;
					case 4: dir = glm::vec3(u, -v, 1.0f); break;
					case 5: dir = glm::vec3(-u, -v, -1.0f); break;
				}

				dir = glm::normalize(dir);

				// Enhanced atmospheric calculations
				float height_factor = glm::max(0.0f, dir.y);
				float atmospheric_depth = 1.0f - glm::abs(dir.y);
				float horizon_distance = pow(1.0f - height_factor, 3.0f);

				 // Base sky gradient with exponential falloff
				glm::vec3 sky_color = glm::mix(sky_horizon, sky_zenith,
				    pow(height_factor, 0.5f));

				// Rayleigh scattering (makes sky blue)
				float rayleigh = pow(horizon_distance, 1.5f) * 0.4f;
				sky_color = glm::mix(sky_color, glm::vec3(0.55f, 0.65f, 0.95f), rayleigh);

				// Mie scattering (haze near horizon)
				float mie = pow(horizon_distance, 4.0f) * 0.3f;
				sky_color = glm::mix(sky_color, glm::vec3(0.92f, 0.88f, 0.82f), mie);

				// Enhanced multi-layer sun
				float sun_dot = glm::max(0.0f, glm::dot(dir, sun_dir));

				// Sun core (intense center)
				float sun_core = pow(sun_dot, 512.0f) * intensity * 2.0f;

				// Sun glow (immediate corona)
				float sun_glow = pow(sun_dot, 32.0f) * 0.7f * intensity;

				// Sun corona (outer glow)
				float sun_corona = pow(sun_dot, 8.0f) * 0.4f * intensity;

				// Sunset color transition near horizon
				float sunset_factor = pow(horizon_distance, 2.0f);
				glm::vec3 current_sun_color = glm::mix(sun_color, sunset_color, sunset_factor * 0.8f);

				// Apply all sun layers
				sky_color += current_sun_color * sun_core;
				sky_color += current_sun_color * sun_glow;
				sky_color += current_sun_color * sun_corona;

				// Atmospheric perspective
				float perspective = pow(atmospheric_depth, 1.5f);
				sky_color = glm::mix(sky_color, sky_horizon * 0.8f, perspective * 0.3f);

				// Ground plane with realistic depth
				if (dir.y < 0.0f) {
					float ground_depth = glm::min(-dir.y * 2.0f, 1.0f);
					glm::vec3 ground_color = glm::mix(ground_near, ground_far, ground_depth);

					// Ground atmospheric perspective
					float ground_fade = pow(-dir.y, 0.6f);
					sky_color = glm::mix(sky_color, ground_color, ground_fade * 0.85f);

					// Subtle ground variation
					float variation = sin(dir.x * 20.0f) * sin(dir.z * 20.0f) * 0.03f;
					sky_color *= 1.0f + variation;
				}

				// Subtle high-altitude clouds
				if (height_factor > 0.3f) {
					float cloud_noise = sin(dir.x * 12.0f + 2.0f) * sin(dir.z * 8.0f + 1.5f);
					float clouds = glm::max(0.0f, cloud_noise) * pow(height_factor - 0.3f, 2.0f) * 0.1f;
					sky_color = glm::mix(sky_color, glm::vec3(1.0f), clouds);
				}

				// Tone mapping with HDR clamping
				sky_color = glm::clamp(sky_color, glm::vec3(0.0f), glm::vec3(1.2f));
				sky_color = sky_color / (sky_color + glm::vec3(1.0f)); // Simple tone map

				int pixel_index = (y * resolution + x) * 4;
				face_data[pixel_index + 0] = sky_color.r;
				face_data[pixel_index + 1] = sky_color.g;
				face_data[pixel_index + 2] = sky_color.b;
				face_data[pixel_index + 3] = 1.0f;
			}
		}

		// Upload face data with comprehensive error checking
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
				   0,
				   GL_RGBA16F,
				   resolution,
				   resolution,
				   0,
				   GL_RGBA,
				   GL_FLOAT,
				   face_data.data());

		// Check for upload errors with fallback
		error = glGetError();
		if (error != GL_NO_ERROR) {
			debugout("Enhanced cubemap face " + std::to_string(face) + " upload error 0x" + std::to_string(error));

			// Fallback to GL_RGBA8 format
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
					   0,
					   GL_RGBA8,
					   resolution,
					   resolution,
					   0,
					   GL_RGBA,
					   GL_UNSIGNED_BYTE,
					   face_data.data());

			error = glGetError();
			if (error != GL_NO_ERROR) {
				debugout("Fallback also failed for face " + std::to_string(face));
				glDeleteTextures(1, &cubemap_texture);
				glBindTexture(GL_TEXTURE_CUBE_MAP, current_texture);
				return 0;
			}
		}
	}

	// Set texture parameters for maximum quality
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	// Generate mipmaps for smooth reflections
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	error = glGetError();
	if (error != GL_NO_ERROR) {
		debugout("Enhanced cubemap mipmap generation warning 0x" + std::to_string(error));
	}

	// Restore previous OpenGL state completely
	glBindTexture(GL_TEXTURE_CUBE_MAP, current_texture);

	// Ensure texture unit 15 is clean
	glActiveTexture(GL_TEXTURE15);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glActiveTexture(GL_TEXTURE0);

	// Final validation
	GLenum final_error = glGetError();
	if (final_error != GL_NO_ERROR) {
		debugout("Enhanced cubemap final error check 0x" + std::to_string(final_error));
	}

	log_gl_errors("generate_enhanced_procedural_cubemap");

	debugout("Generated enhanced cubemap successfully: " + std::to_string(resolution) + "x" + std::to_string(resolution));

	return cubemap_texture;
}


GLuint preview_texture_id()
{
	return preview_runtime_environment().render_targets.preview_color;
}

bool read_preview_rgb_pixels(std::vector<std::uint8_t> *pixels,
	                         int *width,
	                         int *height)
{
	if (pixels == nullptr || width == nullptr || height == nullptr) {
		return false;
	}
	const PreviewRenderTargetRepository &render_targets =
		preview_runtime_environment().render_targets;
	if (render_targets.preview_color == 0 ||
	    render_targets.preview_width <= 0 ||
	    render_targets.preview_height <= 0) {
		return false;
	}

	GLint previous_texture = 0;
	GLint previous_pack_alignment = 0;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture);
	glGetIntegerv(GL_PACK_ALIGNMENT, &previous_pack_alignment);
	glBindTexture(GL_TEXTURE_2D, render_targets.preview_color);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	std::vector<std::uint8_t> bottom_left_pixels(
		static_cast<std::size_t>(render_targets.preview_width) *
		static_cast<std::size_t>(render_targets.preview_height) * 3u);
	glGetTexImage(GL_TEXTURE_2D,
	              0,
	              GL_RGB,
	              GL_UNSIGNED_BYTE,
	              bottom_left_pixels.data());
	const GLenum capture_error = glGetError();
	glPixelStorei(GL_PACK_ALIGNMENT, previous_pack_alignment);
	glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previous_texture));
	if (capture_error != GL_NO_ERROR) {
		return false;
	}

	pixels->resize(bottom_left_pixels.size());
	const std::size_t row_size =
		static_cast<std::size_t>(render_targets.preview_width) * 3u;
	for (int row = 0; row < render_targets.preview_height; ++row) {
		const std::size_t source_offset =
			static_cast<std::size_t>(render_targets.preview_height - row - 1) * row_size;
		const std::size_t destination_offset =
			static_cast<std::size_t>(row) * row_size;
		std::copy_n(bottom_left_pixels.data() + source_offset,
		            row_size,
		            pixels->data() + destination_offset);
	}
	*width = render_targets.preview_width;
	*height = render_targets.preview_height;
	return true;
}

void draw_box(glm::vec3,
              glm::vec3,
              glm::vec3,
              int,
              float,
              float)
{
	// The new frontend renders uploaded scene geometry batches directly.
	// This legacy immediate draw hook is kept only to satisfy older Context code paths.
}

} // namespace imgui_render_internal

namespace {

class DefaultPreviewCubemapGenerator : public PreviewCubemapGenerator {
public:
	GLuint generateSkyCubemap(int resolution, float intensity) override
	{
		return imgui_render_internal::generate_procedural_cubemap(resolution, intensity);
	}

	GLuint generateEnhancedSkyCubemap(int resolution, float intensity) override
	{
		return imgui_render_internal::generate_enhanced_procedural_cubemap(resolution, intensity);
	}
};

class OpenGlPreviewRenderer : public PreviewRenderer {
public:
	bool initialize() override
	{
		return imgui_render_internal::initialize_renderer();
	}

	void shutdown() override
	{
		imgui_render_internal::shutdown_renderer();
	}

	void uploadOpaqueBatches(const std::vector<std::vector<GLfloat>> &buffers,
	                         const std::vector<int> &counts,
	                         std::size_t material_capacity) override
	{
		imgui_render_internal::upload_render_buffers(buffers, counts, material_capacity);
	}

	void uploadTransparentBatches(const std::vector<PreviewTransparentBatch> &batches) override
	{
		imgui_render_internal::upload_transparent_render_batches(batches);
	}

	void uploadDynamicCubeDraws(const std::vector<PreviewDynamicCubeDraw> &draws) override
	{
		imgui_render_internal::upload_dynamic_cube_draws(draws);
	}

	void uploadOverlayLines(const std::vector<PreviewOverlayVertex> &vertices) override
	{
		imgui_render_internal::upload_overlay_lines(vertices);
	}

	void uploadOutlineBatches(const std::vector<PreviewOutlineBatch> &batches) override
	{
		imgui_render_internal::upload_outline_batches(batches);
	}

	void render(const PreviewCameraState &camera,
	            const PreviewRenderSettings &settings,
	            const std::vector<PreviewMaterial> &materials,
	            const PreviewLightCollection &lights) override
	{
		imgui_render_internal::PreviewFrameRequest frame_request;
		frame_request.camera = camera;
		frame_request.settings = settings;
		frame_request.materials = &materials;
		frame_request.lights = &lights;
		imgui_render_internal::render_scene_to_preview(frame_request);
	}

	GLuint previewTextureId() const override
	{
		return imgui_render_internal::preview_texture_id();
	}
};

OpenGlPreviewRenderer g_preview_renderer;
DefaultPreviewCubemapGenerator g_preview_cubemap_generator;

} // namespace

PreviewRenderer &preview_renderer()
{
	return g_preview_renderer;
}

PreviewCubemapGenerator &preview_cubemap_generator()
{
	return g_preview_cubemap_generator;
}

std::array<GLfloat, 36 * 8> build_base_vertex_data()
{
	return imgui_render_internal::build_base_vertex_data();
}

bool initialize_renderer()
{
	return preview_renderer().initialize();
}

void shutdown_renderer()
{
	preview_renderer().shutdown();
}

void upload_render_buffers(const std::vector<std::vector<GLfloat>> &buffers,
                           const std::vector<int> &counts,
                           std::size_t material_capacity)
{
	preview_renderer().uploadOpaqueBatches(buffers, counts, material_capacity);
}

void upload_transparent_render_batches(const std::vector<PreviewTransparentBatch> &batches)
{
	preview_renderer().uploadTransparentBatches(batches);
}

void upload_dynamic_cube_draws(const std::vector<PreviewDynamicCubeDraw> &draws)
{
	preview_renderer().uploadDynamicCubeDraws(draws);
}

void upload_overlay_lines(const std::vector<PreviewOverlayVertex> &vertices)
{
	preview_renderer().uploadOverlayLines(vertices);
}

void upload_outline_batches(const std::vector<PreviewOutlineBatch> &batches)
{
	preview_renderer().uploadOutlineBatches(batches);
}

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
	                             PreviewTextureDebugView debug_view,
                             int mapping_mode_override,
                             bool lens_flare,
                             bool shadows,
                             int anti_aliasing,
                             bool cubemap_enabled,
                             GLuint cubemap_texture,
                             float cubemap_intensity,
                             bool bloom_enabled,
                             float bloom_threshold,
                             float bloom_intensity,
                             bool ambient_occlusion,
                             float exposure,
                             float gamma)
{
	PreviewCameraState camera;
	camera.width = width;
	camera.height = height;
	camera.scale_global = scale_global;
	camera.camera_distance = camera_distance;
	camera.angle_view = angle_view;
	camera.elevation_view = elevation_view;
	camera.roll_view = roll_view;
	camera.vertical_field_of_view_radians = vertical_field_of_view_radians;
	camera.target = glm::vec3(target_x, target_y, target_z);

	PreviewRenderSettings settings;
	settings.simulation_alpha = simulation_alpha;
	settings.preview_time = preview_time;
	settings.debug_view = debug_view;
	settings.mapping_mode_override = mapping_mode_override;
	settings.lens_flare = lens_flare;
	settings.shadows = shadows;
	settings.anti_aliasing = anti_aliasing;
	settings.cubemap_enabled = cubemap_enabled;
	settings.cubemap_texture = cubemap_texture;
	settings.cubemap_intensity = cubemap_intensity;
	settings.bloom_enabled = bloom_enabled;
	settings.bloom_threshold = bloom_threshold;
	settings.bloom_intensity = bloom_intensity;
	settings.ambient_occlusion = ambient_occlusion;
	settings.exposure = exposure;
	settings.gamma = gamma;

	preview_renderer().render(camera, settings, materials, lights);
}

GLuint generate_procedural_cubemap(int resolution, float intensity)
{
	return preview_cubemap_generator().generateSkyCubemap(resolution, intensity);
}

GLuint generate_enhanced_procedural_cubemap(int resolution, float intensity)
{
	return preview_cubemap_generator().generateEnhancedSkyCubemap(resolution, intensity);
}

GLuint preview_texture_id()
{
	return preview_renderer().previewTextureId();
}

bool read_preview_rgb_pixels(std::vector<std::uint8_t> *pixels,
	                         int *width,
	                         int *height)
{
	return imgui_render_internal::read_preview_rgb_pixels(pixels, width, height);
}

void draw_box(glm::vec3 size,
              glm::vec3 position,
              glm::vec3 anchor,
              int material_index,
              float texscale,
              float alpha)
{
	imgui_render_internal::draw_box(size, position, anchor, material_index, texscale, alpha);
}
