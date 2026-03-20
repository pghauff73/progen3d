#include "imgui_render.h"

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

void debugout(const std::string &message);

namespace {

struct CameraMatrices {
	mat4 view{1.0f};
	mat4 projection{1.0f};
	vec3 position{0.0f, 0.0f, 0.0f};
};

const GLchar *VERTEX_SOURCE =
"#version 330\n"
"in vec3 position;\n"
"in vec3 normal;\n"
"in vec2 texture;\n"
"out vec3 v_normal;\n"
"out vec3 v_view_dir;\n"
"out vec3 v_light_dir;\n"
"out vec2 v_tex_coord;\n"
"out vec3 v_view_pos;\n"
"uniform mat4 projection;\n"
"uniform mat4 view;\n"
"uniform mat4 model;\n"
"uniform vec3 lightposition;\n"
"void main(){\n"
"    v_tex_coord = texture;\n"
"    vec4 world_pos = model * vec4(position, 1.0);\n"
"    vec4 view_pos = view * world_pos;\n"
"    mat3 normal_matrix = transpose(inverse(mat3(view * model)));\n"
"    v_normal = normalize(normal_matrix * normal);\n"
"    v_view_dir = normalize(-view_pos.xyz);\n"
"    v_light_dir = normalize((view * vec4(lightposition, 1.0)).xyz - view_pos.xyz);\n"
"    v_view_pos = view_pos.xyz;\n"
"    gl_Position = projection * view_pos;\n"
"}\n";

const GLchar *FRAGMENT_SOURCE =
"#version 330\n"
"in vec3 v_normal;\n"
"in vec3 v_view_dir;\n"
"in vec3 v_light_dir;\n"
"in vec2 v_tex_coord;\n"
"in vec3 v_view_pos;\n"
"uniform vec4 ambientproduct, diffuseproduct, specularproduct ;\n"
"uniform vec3 lightposition ;\n"
"uniform float shinyness ;\n"
"uniform sampler2D texture1;\n"
"uniform float alpha;\n"
"out vec4 fragColor;\n"
"vec3 fresnel_schlick(float cosTheta, vec3 F0){\n"
"    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);\n"
"}\n"
"void main(){\n"
"    vec3 N = normalize(v_normal);\n"
"    vec3 V = normalize(v_view_dir);\n"
"    vec3 L = normalize(v_light_dir);\n"
"    vec3 H = normalize(L + V);\n"
"    vec4 texel = texture(texture1, v_tex_coord);\n"
"    if(texel.a * alpha < 0.05) discard;\n"
"    vec3 albedo = pow(texel.rgb, vec3(2.2));\n"
"    float ndotl = max(dot(N, L), 0.0);\n"
"    float ndotv = max(dot(N, V), 0.0);\n"
"    float ndoth = max(dot(N, H), 0.0);\n"
"    float vdoth = max(dot(V, H), 0.0);\n"
"    vec3 hemi = mix(vec3(0.82, 0.85, 0.90), vec3(0.42, 0.44, 0.48), N.y * 0.5 + 0.5);\n"
"    vec3 ambient = albedo * hemi * ambientproduct.rgb * 0.7;\n"
"    vec3 diffuse = albedo * diffuseproduct.rgb * ndotl;\n"
"    vec3 F0 = mix(vec3(0.04), clamp(specularproduct.rgb, 0.0, 1.0), 0.35);\n"
"    vec3 fresnel = fresnel_schlick(vdoth, F0);\n"
"    float spec_power = max(8.0, shinyness);\n"
"    float specular_term = pow(ndoth, spec_power) * ndotl;\n"
"    vec3 specular = fresnel * specularproduct.rgb * specular_term * 0.35;\n"
"    float rim = pow(1.0 - ndotv, 3.0) * 0.18;\n"
"    vec3 color = ambient + diffuse + specular + rim * albedo;\n"
"    color = color / (color + vec3(1.0));\n"
"    color = pow(color, vec3(1.0 / 2.2));\n"
"    fragColor = vec4(color, texel.a * alpha);\n"
"}\n";

const GLchar *GRID_VERTEX_SOURCE =
"#version 330\n"
"layout (location = 0) in vec3 position;\n"
"uniform mat4 projection;\n"
"uniform mat4 view;\n"
"void main(){\n"
"    gl_Position = projection * view * vec4(position, 1.0);\n"
"}\n";

const GLchar *GRID_FRAGMENT_SOURCE =
"#version 330\n"
"uniform vec4 grid_color;\n"
"out vec4 fragColor;\n"
"void main(){\n"
"    fragColor = grid_color;\n"
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

GLuint g_program = 0;
GLuint g_base_vao = 0;
GLuint g_base_buffer = 0;
std::vector<GLuint> g_geometry_vaos;
std::vector<GLuint> g_geometry_buffers;
std::vector<int> g_render_counts;
GLuint g_grid_program = 0;
GLuint g_grid_vao = 0;
GLuint g_grid_buffer = 0;
GLsizei g_grid_vertex_count = 0;

GLuint g_preview_fbo = 0;
GLuint g_preview_color = 0;
GLuint g_preview_depth = 0;
int g_preview_width = 0;
int g_preview_height = 0;

CameraMatrices build_camera_matrices(int width,
                                     int height,
                                     float angle_view,
                                     float elevation_view)
{
	const float angle_radians = angle_view * static_cast<float>(M_PI) / 180.0f;
	const float elevation_radians = elevation_view * static_cast<float>(M_PI) / 180.0f;
	const float radius = 5.0f;

	CameraMatrices camera;
	camera.position =
		vec3(radius * std::cos(elevation_radians) * std::cos(angle_radians),
		     radius * std::sin(elevation_radians),
		     radius * std::cos(elevation_radians) * std::sin(angle_radians));
	camera.view = lookAt(camera.position, vec3(0.0f, 0.0f, 0.0f), vec3(0, 1, 0));
	camera.projection = perspective(43.0,
	                                static_cast<double>(width) / static_cast<double>(height),
	                                0.01,
	                                100.0);
	return camera;
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

void ensure_geometry_capacity(std::size_t texture_capacity)
{
	const std::size_t current_capacity = g_geometry_vaos.size();
	if (current_capacity >= texture_capacity) {
		if (g_render_counts.size() < texture_capacity) {
			g_render_counts.resize(texture_capacity, 0);
		}
		return;
	}

	g_geometry_vaos.resize(texture_capacity, 0);
	g_geometry_buffers.resize(texture_capacity, 0);
	g_render_counts.resize(texture_capacity, 0);

	for (std::size_t i = current_capacity; i < texture_capacity; ++i) {
		glGenVertexArrays(1, &g_geometry_vaos[i]);
		glGenBuffers(1, &g_geometry_buffers[i]);
		setup_buffer_layout(g_geometry_vaos[i], g_geometry_buffers[i]);
		glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void destroy_preview_target()
{
	if (g_preview_depth != 0) {
		glDeleteRenderbuffers(1, &g_preview_depth);
		g_preview_depth = 0;
	}
	if (g_preview_color != 0) {
		glDeleteTextures(1, &g_preview_color);
		g_preview_color = 0;
	}
	if (g_preview_fbo != 0) {
		glDeleteFramebuffers(1, &g_preview_fbo);
		g_preview_fbo = 0;
	}
	g_preview_width = 0;
	g_preview_height = 0;
}

bool ensure_preview_target(int width, int height)
{
	width = std::max(width, 1);
	height = std::max(height, 1);
	if (width == g_preview_width && height == g_preview_height && g_preview_fbo != 0) {
		return true;
	}

	destroy_preview_target();

	glGenFramebuffers(1, &g_preview_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, g_preview_fbo);

	glGenTextures(1, &g_preview_color);
	glBindTexture(GL_TEXTURE_2D, g_preview_color);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_preview_color, 0);

	glGenRenderbuffers(1, &g_preview_depth);
	glBindRenderbuffer(GL_RENDERBUFFER, g_preview_depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, g_preview_depth);

	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		debugout("Preview framebuffer is incomplete");
		destroy_preview_target();
		return false;
	}

	g_preview_width = width;
	g_preview_height = height;
	return true;
}

void set_common_uniforms(GLuint program,
                         int width,
                         int height,
                         float scale_global,
                         float angle_view,
                         float elevation_view)
{
	mat4 model = mat4(1.0f);
	model = scale(model, vec3(scale_global, scale_global, scale_global));
	glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &model[0][0]);

	const CameraMatrices camera = build_camera_matrices(width, height, angle_view, elevation_view);
	glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, &camera.view[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, &camera.projection[0][0]);

	const vec3 lightposition = vec3(1.1f * scale_global, -60.7f * scale_global, -1.3f * scale_global);
	glUniform3fv(glGetUniformLocation(program, "lightposition"), 1, &lightposition[0]);

	const glm::vec4 ambientproduct = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
	const glm::vec4 diffuseproduct = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
	const glm::vec4 specularproduct = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
	glUniform4fv(glGetUniformLocation(program, "ambientproduct"), 1, &ambientproduct[0]);
	glUniform4fv(glGetUniformLocation(program, "diffuseproduct"), 1, &diffuseproduct[0]);
	glUniform4fv(glGetUniformLocation(program, "specularproduct"), 1, &specularproduct[0]);
	glUniform1f(glGetUniformLocation(program, "shinyness"), 16.0f);
}

void draw_grid(int width, int height, float angle_view, float elevation_view)
{
	if (g_grid_program == 0 || g_grid_vao == 0 || g_grid_vertex_count <= 0) {
		return;
	}

	const CameraMatrices camera = build_camera_matrices(width, height, angle_view, elevation_view);
	glUseProgram(g_grid_program);
	glUniformMatrix4fv(glGetUniformLocation(g_grid_program, "view"), 1, GL_FALSE, &camera.view[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(g_grid_program, "projection"), 1, GL_FALSE, &camera.projection[0][0]);
	glUniform4f(glGetUniformLocation(g_grid_program, "grid_color"), 0.36f, 0.39f, 0.46f, 1.0f);
	glBindVertexArray(g_grid_vao);
	glDrawArrays(GL_LINES, 0, g_grid_vertex_count);
}

void draw_buffer(std::size_t index,
                 const std::vector<int> &textures,
                 const std::vector<float> &texture_alphas)
{
	if (index >= g_geometry_vaos.size() ||
	    index >= g_geometry_buffers.size() ||
	    index >= g_render_counts.size() ||
	    index >= textures.size() ||
	    index >= texture_alphas.size() ||
	    g_render_counts[index] <= 0) {
		return;
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(textures[index]));
	glUniform1i(glGetUniformLocation(g_program, "texture1"), 0);
	glUniform1f(glGetUniformLocation(g_program, "alpha"), texture_alphas[index]);

	glBindVertexArray(g_geometry_vaos[index]);
	glDrawArrays(GL_TRIANGLES, 0, 36 * g_render_counts[index]);
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
	if (g_program != 0) {
		return true;
	}

	auto base_vertices = build_base_vertex_data();
	glGenVertexArrays(1, &g_base_vao);
	glGenBuffers(1, &g_base_buffer);
	setup_buffer_layout(g_base_vao, g_base_buffer);
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

	g_program = glCreateProgram();
	glAttachShader(g_program, vertex);
	glAttachShader(g_program, fragment);
	glLinkProgram(g_program);
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	GLint status = GL_FALSE;
	glGetProgramiv(g_program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		GLint log_len = 0;
		glGetProgramiv(g_program, GL_INFO_LOG_LENGTH, &log_len);
		std::vector<char> buffer(static_cast<std::size_t>(std::max(log_len, 1)));
		glGetProgramInfoLog(g_program, log_len, NULL, buffer.data());
		debugout(std::string("Program link failure: ") + buffer.data());
		glDeleteProgram(g_program);
		g_program = 0;
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

	g_grid_program = glCreateProgram();
	glAttachShader(g_grid_program, grid_vertex);
	glAttachShader(g_grid_program, grid_fragment);
	glLinkProgram(g_grid_program);
	glDeleteShader(grid_vertex);
	glDeleteShader(grid_fragment);

	glGetProgramiv(g_grid_program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		GLint log_len = 0;
		glGetProgramiv(g_grid_program, GL_INFO_LOG_LENGTH, &log_len);
		std::vector<char> buffer(static_cast<std::size_t>(std::max(log_len, 1)));
		glGetProgramInfoLog(g_grid_program, log_len, NULL, buffer.data());
		debugout(std::string("Grid program link failure: ") + buffer.data());
		glDeleteProgram(g_grid_program);
		g_grid_program = 0;
		return false;
	}

	const std::vector<GLfloat> grid_vertices = build_grid_vertices();
	g_grid_vertex_count = static_cast<GLsizei>(grid_vertices.size() / 3);
	glGenVertexArrays(1, &g_grid_vao);
	glGenBuffers(1, &g_grid_buffer);
	glBindVertexArray(g_grid_vao);
	glBindBuffer(GL_ARRAY_BUFFER, g_grid_buffer);
	glBufferData(GL_ARRAY_BUFFER,
	             sizeof(GLfloat) * grid_vertices.size(),
	             grid_vertices.data(),
	             GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
	glEnableVertexAttribArray(0);

	return true;
}

void shutdown_renderer()
{
	destroy_preview_target();
	for (std::size_t i = 0; i < g_geometry_vaos.size(); ++i) {
		if (g_geometry_buffers[i] != 0) {
			glDeleteBuffers(1, &g_geometry_buffers[i]);
		}
		if (g_geometry_vaos[i] != 0) {
			glDeleteVertexArrays(1, &g_geometry_vaos[i]);
		}
	}
	g_geometry_buffers.clear();
	g_geometry_vaos.clear();
	g_render_counts.clear();

	if (g_base_buffer != 0) {
		glDeleteBuffers(1, &g_base_buffer);
		g_base_buffer = 0;
	}
	if (g_base_vao != 0) {
		glDeleteVertexArrays(1, &g_base_vao);
		g_base_vao = 0;
	}
	if (g_program != 0) {
		glDeleteProgram(g_program);
		g_program = 0;
	}
	if (g_grid_buffer != 0) {
		glDeleteBuffers(1, &g_grid_buffer);
		g_grid_buffer = 0;
	}
	if (g_grid_vao != 0) {
		glDeleteVertexArrays(1, &g_grid_vao);
		g_grid_vao = 0;
	}
	if (g_grid_program != 0) {
		glDeleteProgram(g_grid_program);
		g_grid_program = 0;
	}
	g_grid_vertex_count = 0;
}

void upload_render_buffers(const std::vector<std::vector<GLfloat>> &buffers,
                           const std::vector<int> &counts,
                           std::size_t texture_capacity)
{
	ensure_geometry_capacity(texture_capacity);
	g_render_counts.assign(texture_capacity, 0);

	for (std::size_t i = 0; i < texture_capacity; ++i) {
		if (i < counts.size()) {
			g_render_counts[i] = counts[i];
		}
		if (i < buffers.size() && !buffers[i].empty()) {
			glBindVertexArray(g_geometry_vaos[i]);
			glBindBuffer(GL_ARRAY_BUFFER, g_geometry_buffers[i]);
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

void render_scene_to_preview(int width,
                             int height,
                             const std::vector<int> &textures,
                             const std::vector<float> &texture_alphas,
                             float scale_global,
                             float angle_view,
                             float elevation_view)
{
	if (g_program == 0 || !ensure_preview_target(width, height)) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, g_preview_fbo);
	glViewport(0, 0, g_preview_width, g_preview_height);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDisable(GL_CULL_FACE);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	draw_grid(g_preview_width, g_preview_height, angle_view, elevation_view);

	glUseProgram(g_program);
	set_common_uniforms(g_program, g_preview_width, g_preview_height, scale_global, angle_view, elevation_view);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	for (std::size_t i = 0; i < g_render_counts.size() && i < texture_alphas.size(); ++i) {
		if (texture_alphas[i] >= 0.999f) {
			draw_buffer(i, textures, texture_alphas);
		}
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	for (std::size_t i = 0; i < g_render_counts.size() && i < texture_alphas.size(); ++i) {
		if (texture_alphas[i] < 0.999f) {
			draw_buffer(i, textures, texture_alphas);
		}
	}

	glDepthMask(GL_TRUE);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	log_gl_errors("render_scene_to_preview");
}

GLuint preview_texture_id()
{
	return g_preview_color;
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
