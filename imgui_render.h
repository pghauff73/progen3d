#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include <epoxy/gl.h>

std::array<GLfloat, 36 * 8> build_base_vertex_data();
bool initialize_renderer();
void shutdown_renderer();
void upload_render_buffers(const std::vector<std::vector<GLfloat>> &buffers,
                           const std::vector<int> &counts,
                           std::size_t texture_capacity);
void render_scene_to_preview(int width,
                             int height,
                             const std::vector<int> &textures,
                             const std::vector<float> &texture_alphas,
                             float scale_global,
                             float angle_view,
                             float elevation_view);
GLuint preview_texture_id();
