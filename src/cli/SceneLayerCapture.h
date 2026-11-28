#pragma once

#include "imgui_render.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class SceneGenerationContext;

struct SceneMaskLabel {
	std::string object;
	std::optional<std::size_t> primitive_index;
};

struct CapturedSceneLayers {
	std::vector<float> linear_depth;
	std::vector<std::uint32_t> object_ids;
};

class SceneLayerCapture
{
public:
	SceneLayerCapture() = default;
	SceneLayerCapture(const SceneLayerCapture &) = delete;
	SceneLayerCapture &operator=(const SceneLayerCapture &) = delete;
	~SceneLayerCapture();

	void initialize(const SceneGenerationContext &scene, const std::vector<PreviewMaterial> &materials,
	                int width, int height, bool track_movable = false);
	bool updatePhysics(const SceneGenerationContext &scene);
	CapturedSceneLayers capture(const PreviewCameraState &camera, bool include_object_ids) const;
	const std::vector<SceneMaskLabel> &labels() const { return labels_; }
	std::vector<SceneMaskLabel> takeLabels();

private:
	struct MovableVertexRange {
		std::size_t instance_index;
		std::size_t vertex_offset;
		std::size_t vertex_count;
		std::uint32_t object_id;
	};

	int width_ = 0;
	int height_ = 0;
	GLuint framebuffer_ = 0;
	GLuint depth_surface_ = 0;
	GLuint linear_depth_ = 0;
	GLuint object_ids_ = 0;
	GLuint program_ = 0;
	GLuint vertex_array_ = 0;
	GLuint vertex_buffer_ = 0;
	GLsizei vertex_count_ = 0;
	std::vector<SceneMaskLabel> labels_;
	bool has_physics_layout_ = false;
	std::vector<bool> removed_instances_;
	std::vector<MovableVertexRange> movable_ranges_;
};
