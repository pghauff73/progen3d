#include "SceneLayerCapture.h"

#include "Context.h"
#include "spatial/model/SpatialBuildingModel.h"

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <stdexcept>
#include <utility>

namespace {

struct LayerVertex {
	float x;
	float y;
	float z;
	std::uint32_t object_id;
};

const char *layer_vertex_shader = R"glsl(
#version 460 core
layout(location = 0) in vec3 position;
layout(location = 1) in uint object_id;
uniform mat4 view;
uniform mat4 projection;
uniform float scene_scale;
out float camera_depth;
flat out uint label;

void main()
{
    vec4 camera_position = view * vec4(position * scene_scale, 1.0);
    camera_depth = -camera_position.z / scene_scale;
    label = object_id;
    gl_Position = projection * camera_position;
}
)glsl";

const char *layer_fragment_shader = R"glsl(
#version 460 core
in float camera_depth;
flat in uint label;
layout(location = 0) out float linear_depth;
layout(location = 1) out uint object_id;

void main()
{
    linear_depth = camera_depth;
    object_id = label;
}
)glsl";

GLuint compile_shader(GLenum type, const char *source)
{
	const GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);
	GLint compiled = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (compiled == GL_TRUE) {
		return shader;
	}
	std::array<char, 2048> error{};
	glGetShaderInfoLog(shader, static_cast<GLsizei>(error.size()), nullptr, error.data());
	glDeleteShader(shader);
	throw std::runtime_error(std::string("Layer shader compilation failed: ") + error.data());
}

GLuint create_layer_program()
{
	const GLuint vertex = compile_shader(GL_VERTEX_SHADER, layer_vertex_shader);
	GLuint fragment = 0;
	try {
		fragment = compile_shader(GL_FRAGMENT_SHADER, layer_fragment_shader);
	} catch (...) {
		glDeleteShader(vertex);
		throw;
	}
	const GLuint program = glCreateProgram();
	glAttachShader(program, vertex);
	glAttachShader(program, fragment);
	glLinkProgram(program);
	glDeleteShader(vertex);
	glDeleteShader(fragment);
	GLint linked = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (linked == GL_TRUE) {
		return program;
	}
	std::array<char, 2048> error{};
	glGetProgramInfoLog(program, static_cast<GLsizei>(error.size()), nullptr, error.data());
	glDeleteProgram(program);
	throw std::runtime_error(std::string("Layer shader link failed: ") + error.data());
}

std::vector<std::uint32_t> assign_object_ids(const SceneGenerationContext &scene,
                                              std::vector<SceneMaskLabel> &labels)
{
	std::vector<std::uint32_t> ids(scene.primitive_instances.size(), 0);
	labels.emplace_back();
	const SpatialBuildingModel *model = scene.spatialBuildingModel().get();
	if (model != nullptr) {
		std::map<std::string, std::uint32_t> named_ids;
		for (const auto &object : model->objects().objects()) {
			named_ids.emplace(object.identity().objectId().value(), 0);
		}
		for (auto &[name, id] : named_ids) {
			id = static_cast<std::uint32_t>(labels.size());
			labels.push_back({name, std::nullopt});
		}
		for (const auto &binding : model->geometryBindings()) {
			const std::size_t index = binding.primitiveInstanceIndex();
			if (index < ids.size() && ids[index] == 0) {
				ids[index] = named_ids.at(binding.objectId().value());
			}
		}
	}
	return ids;
}

GLuint attach_layer(GLuint framebuffer, GLenum attachment, GLenum format, int width, int height)
{
	GLuint buffer = 0;
	glCreateRenderbuffers(1, &buffer);
	glNamedRenderbufferStorage(buffer, format, width, height);
	glNamedFramebufferRenderbuffer(framebuffer, attachment, GL_RENDERBUFFER, buffer);
	return buffer;
}

void append_layer_vertices(std::vector<LayerVertex> &layers, const std::vector<GLfloat> &vertices,
                           std::uint32_t object_id)
{
	for (std::size_t vertex = 0; vertex < vertices.size(); vertex += 8) {
		layers.push_back({vertices[vertex], vertices[vertex + 1], vertices[vertex + 2], object_id});
	}
}

template <typename Pixel>
void flip_rows(std::vector<Pixel> &pixels, int width, int height)
{
	for (int row = 0; row < height / 2; ++row) {
		const auto top = pixels.begin() + static_cast<std::size_t>(row) * width;
		const auto bottom = pixels.begin() + static_cast<std::size_t>(height - row - 1) * width;
		std::swap_ranges(top, top + width, bottom);
	}
}

}

SceneLayerCapture::~SceneLayerCapture()
{
	glDeleteVertexArrays(1, &vertex_array_);
	glDeleteBuffers(1, &vertex_buffer_);
	glDeleteFramebuffers(1, &framebuffer_);
	glDeleteRenderbuffers(1, &depth_surface_);
	glDeleteRenderbuffers(1, &linear_depth_);
	glDeleteRenderbuffers(1, &object_ids_);
	glDeleteProgram(program_);
}

void SceneLayerCapture::initialize(const SceneGenerationContext &scene,
                                   const std::vector<PreviewMaterial> &materials, int width, int height,
                                   bool track_movable)
{
	if (program_ != 0 && (width != width_ || height != height_)) {
		throw std::runtime_error("Layer capture dimensions changed after initialization.");
	}
	width_ = width;
	height_ = height;
	labels_.clear();
	const std::vector<std::uint32_t> ids = assign_object_ids(scene, labels_);
	const auto cube_vertices = build_base_vertex_data();
	std::vector<LayerVertex> layer_vertices;
	std::vector<GLfloat> instance_vertices;
	removed_instances_.clear();
	movable_ranges_.clear();
	if (track_movable) removed_instances_.reserve(scene.primitive_instances.size());
	for (std::size_t index = 0; index < scene.primitive_instances.size(); ++index) {
		const PrimitiveInstance &instance = scene.primitive_instances[index];
		if (track_movable) removed_instances_.push_back(instance.removed);
		if (instance.removed || instance.material_index < 0 ||
		    static_cast<std::size_t>(instance.material_index) >= materials.size() ||
		    materials[instance.material_index].opacity < 0.02f) {
			continue;
		}
		instance_vertices.clear();
		scene.buildInstanceBuffer(instance, cube_vertices.data(), &instance_vertices);
		if (instance_vertices.empty()) {
			continue;
		}
		std::uint32_t id = ids[index];
		if (id == 0) {
			id = static_cast<std::uint32_t>(labels_.size());
			labels_.push_back({"", index});
		}
		if (track_movable && !instance.immovable) {
			movable_ranges_.push_back({index, layer_vertices.size(), instance_vertices.size() / 8, id});
		}
		append_layer_vertices(layer_vertices, instance_vertices, id);
	}
	if (layer_vertices.size() > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max())) {
		throw std::runtime_error("The scene has too many vertices for the layer pass.");
	}

	if (program_ == 0) {
		program_ = create_layer_program();
		glCreateFramebuffers(1, &framebuffer_);
		depth_surface_ = attach_layer(framebuffer_, GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT24, width, height);
		linear_depth_ = attach_layer(framebuffer_, GL_COLOR_ATTACHMENT0, GL_R32F, width, height);
		object_ids_ = attach_layer(framebuffer_, GL_COLOR_ATTACHMENT1, GL_R32UI, width, height);
		const GLenum attachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
		glNamedFramebufferDrawBuffers(framebuffer_, 2, attachments);
		if (glCheckNamedFramebufferStatus(framebuffer_, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			throw std::runtime_error("Cannot create the depth and object ID framebuffer.");
		}

		glCreateVertexArrays(1, &vertex_array_);
		glCreateBuffers(1, &vertex_buffer_);
		glVertexArrayVertexBuffer(vertex_array_, 0, vertex_buffer_, 0, sizeof(LayerVertex));
		glEnableVertexArrayAttrib(vertex_array_, 0);
		glVertexArrayAttribFormat(vertex_array_, 0, 3, GL_FLOAT, GL_FALSE, offsetof(LayerVertex, x));
		glVertexArrayAttribBinding(vertex_array_, 0, 0);
		glEnableVertexArrayAttrib(vertex_array_, 1);
		glVertexArrayAttribIFormat(vertex_array_, 1, 1, GL_UNSIGNED_INT, offsetof(LayerVertex, object_id));
		glVertexArrayAttribBinding(vertex_array_, 1, 0);
	}
	glNamedBufferData(vertex_buffer_, static_cast<GLsizeiptr>(layer_vertices.size() * sizeof(LayerVertex)),
	                  layer_vertices.data(), GL_STATIC_DRAW);
	vertex_count_ = static_cast<GLsizei>(layer_vertices.size());
	if (glGetError() != GL_NO_ERROR) {
		throw std::runtime_error("Cannot upload geometry for depth and object ID capture.");
	}
	has_physics_layout_ = track_movable;
}

bool SceneLayerCapture::updatePhysics(const SceneGenerationContext &scene)
{
	const auto &instances = scene.primitive_instances;
	if (!has_physics_layout_ || instances.size() != removed_instances_.size()) return false;
	for (std::size_t index = 0; index < instances.size(); ++index) {
		if (instances[index].removed != removed_instances_[index]) return false;
	}
	const auto cube_vertices = build_base_vertex_data();
	std::vector<GLfloat> instance_vertices;
	std::vector<LayerVertex> layer_vertices;
	for (const MovableVertexRange &range : movable_ranges_) {
		instance_vertices.clear();
		scene.buildInstanceBuffer(instances[range.instance_index], cube_vertices.data(), &instance_vertices);
		if (instance_vertices.size() != range.vertex_count * 8) return false;
		layer_vertices.clear();
		append_layer_vertices(layer_vertices, instance_vertices, range.object_id);
		glNamedBufferSubData(vertex_buffer_, static_cast<GLintptr>(range.vertex_offset * sizeof(LayerVertex)),
		                     static_cast<GLsizeiptr>(layer_vertices.size() * sizeof(LayerVertex)),
		                     layer_vertices.data());
	}
	if (glGetError() != GL_NO_ERROR) {
		throw std::runtime_error("Cannot update moving geometry for depth and object ID capture.");
	}
	return true;
}

CapturedSceneLayers SceneLayerCapture::capture(const PreviewCameraState &camera, bool include_object_ids) const
{
	if (camera.width != width_ || camera.height != height_) {
		throw std::runtime_error("Layer capture requires a camera at the framebuffer dimensions.");
	}
	const std::size_t pixel_count = static_cast<std::size_t>(camera.width) * camera.height;
	CapturedSceneLayers layers;
	layers.linear_depth.resize(pixel_count);
	if (include_object_ids) {
		layers.object_ids.resize(pixel_count);
	}

	const auto matrices = preview_camera_matrices(camera);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
	glViewport(0, 0, camera.width, camera.height);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	const GLfloat depth_clear = 1.0f;
	const GLfloat empty_depth[4]{};
	const GLuint empty_label[4]{};
	glClearBufferfv(GL_DEPTH, 0, &depth_clear);
	glClearBufferfv(GL_COLOR, 0, empty_depth);
	glClearBufferuiv(GL_COLOR, 1, empty_label);
	glUseProgram(program_);
	glUniformMatrix4fv(glGetUniformLocation(program_, "view"), 1, GL_FALSE, &matrices.view[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(program_, "projection"), 1, GL_FALSE, &matrices.projection[0][0]);
	glUniform1f(glGetUniformLocation(program_, "scene_scale"), camera.scale_global);
	glBindVertexArray(vertex_array_);
	glDrawArrays(GL_TRIANGLES, 0, vertex_count_);

	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glReadPixels(0, 0, camera.width, camera.height, GL_RED, GL_FLOAT, layers.linear_depth.data());
	if (include_object_ids) {
		glReadBuffer(GL_COLOR_ATTACHMENT1);
		glReadPixels(0, 0, camera.width, camera.height, GL_RED_INTEGER, GL_UNSIGNED_INT, layers.object_ids.data());
		flip_rows(layers.object_ids, camera.width, camera.height);
	}
	glBindVertexArray(0);
	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	if (glGetError() != GL_NO_ERROR) {
		throw std::runtime_error("Cannot read the depth and object ID layers.");
	}
	flip_rows(layers.linear_depth, camera.width, camera.height);
	return layers;
}

std::vector<SceneMaskLabel> SceneLayerCapture::takeLabels()
{
	return std::move(labels_);
}
