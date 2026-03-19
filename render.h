#pragma once

#include <array>
#include <vector>

#include <epoxy/gl.h>
#include <gtk/gtk.h>
#include <glm/glm.hpp>

extern int tex_count[50];

std::array<GLfloat, 36 * 8> build_base_vertex_data();
void initialize_render_texture_buffers();
void upload_render_buffers(const std::vector<std::vector<GLfloat>> &buffers,
                           const std::vector<int> &counts);
void draw_box(float angle_cube,
              glm::vec3 scale_vec,
              glm::vec3 position_vec,
              glm::vec3 pos,
              int tex_index,
              float texscale);

gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data);
gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data);
gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data);
void realize(GtkWidget *widget);
void unrealize(GtkWidget *widget);
void resize_glarea(GtkGLArea *area, int width, int height, gpointer user_data);
gboolean render(GtkGLArea *area, GdkGLContext *context);
