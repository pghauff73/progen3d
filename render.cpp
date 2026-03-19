#include "render.h"

#include <array>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using glm::lookAt;
using glm::mat4;
using glm::perspective;
using glm::rotate;
using glm::scale;
using glm::translate;
using glm::vec3;

extern std::string grammar_filename;
extern GtkTextBuffer *mybuffer;
extern GtkWidget *gl_area;
extern std::vector<int> texture_list;
extern std::vector<float> texture_list_alpha;
extern float scale_global;
extern float angle_view;
extern float elevation_view;
extern double mousex;
extern double mousey;
extern bool mouse_dragging;
extern bool play;
extern std::atomic<bool> regeneration_in_progress;

int tex_count[50] = {};

void setup();
void txt_inserted(GtkTextBuffer *textbuffer,
                  GtkTextIter *location,
                  gchar *text,
                  gint len,
                  gpointer user_data);
void start_async_regeneration(bool save_current_file);

namespace {

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
"uniform mat4 model;\n"
"uniform mat4 view;\n"
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

const GLfloat vertex_data[] = {
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

long current_frame = 0.0;
long delta_time = 0.0;
GDateTime *last_frame = nullptr;
GLuint position_buffer = 0;
GLuint position_buffer1[50] = {};
GLuint program = 0;
GLuint vao = 0;
GLuint vao1[50] = {};
int glarea_width = 1;
int glarea_height = 1;
int render_texture_capacity = 0;
mat4 model = mat4(1.0f);

GLuint create_shader(int type)
{
  GLuint shader = glCreateShader(type);
  int status;
  if (type == GL_FRAGMENT_SHADER) {
    glShaderSource(shader, 1, &FRAGMENT_SOURCE, NULL);
  }
  if (type == GL_VERTEX_SHADER) {
    glShaderSource(shader, 1, &VERTEX_SOURCE, NULL);
  }
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status == GL_FALSE) {
    int log_len;
    char *buffer;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
    buffer = (char*)g_malloc(log_len + 1);
    glGetShaderInfoLog(shader, log_len, NULL, buffer);
    g_warning("Compile failure in %s shader:\n%s",
              type == GL_VERTEX_SHADER ? "vertex" : "fragment",
              buffer);
    g_free(buffer);
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

void draw_box_internal(float angle_cube,
                       vec3 scale_vec,
                       vec3 position_vec,
                       vec3,
                       int tex_index,
                       float texscale)
{
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_list[tex_index]);
  glUseProgram(program);
  glUniform1i(glGetUniformLocation(program, "texture1"), 0);

  position_vec = vec3(position_vec.x * scale_global,
                      position_vec.y * scale_global,
                      position_vec.z * scale_global);

  model = glm::mat4(1.0f);
  model = rotate(model, angle_cube, vec3(0, 1, 0));
  model = translate(model, position_vec);
  vec3 scale_vec2 = vec3(scale_vec.x * scale_global,
                         scale_vec.y * scale_global,
                         scale_vec.z * scale_global);
  model = scale(model, scale_vec2);

  glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &model[0][0]);

  float angle_radians = angle_view * M_PI / 180.0f;
  float elevation_radians = elevation_view * M_PI / 180.0f;
  float radius = 5.0f;
  vec3 position = vec3(radius * cos(elevation_radians) * cos(angle_radians),
                       radius * sin(elevation_radians),
                       radius * cos(elevation_radians) * sin(angle_radians));

  mat4 view = lookAt(position, vec3(0.0f, 0.0f, 0.0f), vec3(0, 1, 0));
  glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, &view[0][0]);

  mat4 projection = perspective(43.0,
                                double(glarea_width) / double(glarea_height),
                                0.01,
                                100.0);
  glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, &projection[0][0]);

  vec3 lightposition = vec3(1.1f, 15.7f, -1.3f);
  glUniform3fv(glGetUniformLocation(program, "lightposition"), 1, &lightposition[0]);

  glm::vec4 ambientproduct = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
  glm::vec4 diffuseproduct = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
  glm::vec4 specularproduct = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
  glUniform4fv(glGetUniformLocation(program, "ambientproduct"), 1, &ambientproduct[0]);
  glUniform4fv(glGetUniformLocation(program, "diffuseproduct"), 1, &diffuseproduct[0]);
  glUniform4fv(glGetUniformLocation(program, "specularproduct"), 1, &specularproduct[0]);
  glUniform1f(glGetUniformLocation(program, "shinyness"), 128.0f);
  glUniform1f(glGetUniformLocation(program, "alpha"), texscale);

  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glUseProgram(0);
}

void draw_buffer(int tex_index)
{
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_list[tex_index]);
  glUseProgram(program);
  glUniform1i(glGetUniformLocation(program, "texture1"), 0);
  glDepthFunc(GL_LESS);

  model = glm::mat4(1.0f);
  vec3 scale_vec2 = vec3(scale_global, scale_global, scale_global);
  model = scale(model, scale_vec2);
  glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &model[0][0]);

  float angle_radians = angle_view * M_PI / 180.0f;
  float elevation_radians = elevation_view * M_PI / 180.0f;
  float radius = 5.0f;
  vec3 position = vec3(radius * cos(elevation_radians) * cos(angle_radians),
                       radius * sin(elevation_radians),
                       radius * cos(elevation_radians) * sin(angle_radians));

  mat4 view = lookAt(position, vec3(0.0f, 0.0f, 0.0f), vec3(0, 1, 0));
  glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, &view[0][0]);

  mat4 projection = perspective(43.0,
                                double(glarea_width) / double(glarea_height),
                                0.01,
                                100.0);
  glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, &projection[0][0]);

  vec3 lightposition = vec3(1.1f * scale_global, -60.7f * scale_global, -1.3f * scale_global);
  glUniform3fv(glGetUniformLocation(program, "lightposition"), 1, &lightposition[0]);

  glm::vec4 ambientproduct = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
  glm::vec4 diffuseproduct = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
  glm::vec4 specularproduct = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
  glUniform4fv(glGetUniformLocation(program, "ambientproduct"), 1, &ambientproduct[0]);
  glUniform4fv(glGetUniformLocation(program, "diffuseproduct"), 1, &diffuseproduct[0]);
  glUniform4fv(glGetUniformLocation(program, "specularproduct"), 1, &specularproduct[0]);
  glUniform1f(glGetUniformLocation(program, "shinyness"), 16.0f);
  glUniform1f(glGetUniformLocation(program, "alpha"), texture_list_alpha[tex_index]);

  glBindVertexArray(vao1[tex_index]);
  glDrawArrays(GL_TRIANGLES, 0, 36 * tex_count[tex_index]);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glUseProgram(0);
}

}

void draw_box(float angle_cube,
              glm::vec3 scale_vec,
              glm::vec3 position_vec,
              glm::vec3 pos,
              int tex_index,
              float texscale)
{
  (void)pos;
  draw_box_internal(angle_cube, scale_vec, position_vec, glm::vec3(0.0f), tex_index, texscale);
}

std::array<GLfloat, 36 * 8> build_base_vertex_data()
{
  std::array<GLfloat, 36 * 8> mydata{};
  for (int i = 0; i < 36 * 8; i++) {
    mydata[i] = vertex_data[i];
  }
  for (int i = 0; i < 36; i++) {
    for (int j = 0; j < 8; j++) {
      if (j == 0 || j == 2) mydata[i * 8 + j] *= 0.5f;
      if (j == 1 && mydata[i * 8 + j] < 0) mydata[i * 8 + j] = 0.0f;
    }
  }
  return mydata;
}

void initialize_render_texture_buffers()
{
  if (gl_area == NULL) return;
  gtk_gl_area_make_current(GTK_GL_AREA(gl_area));
  if (gtk_gl_area_get_error(GTK_GL_AREA(gl_area)) != NULL) return;
  int target_capacity = std::min<int>(texture_list.size(), 50);
  for (int i = render_texture_capacity; i < target_capacity; ++i) {
    glGenVertexArrays(1, &vao1[i]);
    glBindVertexArray(vao1[i]);
    glGenBuffers(1, &position_buffer1[i]);
    glBindBuffer(GL_ARRAY_BUFFER, position_buffer1[i]);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  render_texture_capacity = target_capacity;
}

void upload_render_buffers(const std::vector<std::vector<GLfloat>> &buffers,
                           const std::vector<int> &counts)
{
  if (gl_area == NULL) return;
  gtk_gl_area_make_current(GTK_GL_AREA(gl_area));
  if (gtk_gl_area_get_error(GTK_GL_AREA(gl_area)) != NULL) return;
  initialize_render_texture_buffers();
  for (int i = 0; i < 50; i++) {
    tex_count[i] = 0;
  }
  for (int i = 0; i < texture_list.size() && i < 50; i++) {
    if (i < counts.size()) tex_count[i] = counts[i];
    if (tex_count[i] > 0 && i < buffers.size() && !buffers[i].empty()) {
      glBindVertexArray(vao1[i]);
      glBindBuffer(GL_ARRAY_BUFFER, position_buffer1[i]);
      glBufferData(GL_ARRAY_BUFFER,
                   sizeof(GLfloat) * buffers[i].size(),
                   buffers[i].data(),
                   GL_DYNAMIC_DRAW);
    }
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  gtk_gl_area_queue_render(GTK_GL_AREA(gl_area));
}

gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer)
{
  if (event->button == 1) {
    mouse_dragging = true;
    mousex = event->x;
    mousey = event->y;
    return TRUE;
  }
  return FALSE;
}

gboolean on_button_release(GtkWidget *, GdkEventButton *event, gpointer)
{
  if (event->button == 1) {
    mouse_dragging = false;
    return TRUE;
  }
  return FALSE;
}

gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer)
{
  if (mouse_dragging) {
    double dx = event->x - mousex;
    double dy = event->y - mousey;
    angle_view += dx * 0.5;
    elevation_view += dy * 0.5;
    if (elevation_view > 89.0) elevation_view = 89.0;
    if (elevation_view < -89.0) elevation_view = -89.0;
    while (angle_view >= 360.0) angle_view -= 360.0;
    while (angle_view < 0.0) angle_view += 360.0;
    mousex = event->x;
    mousey = event->y;
    gtk_gl_area_queue_render(GTK_GL_AREA(widget));
    return TRUE;
  }
  return FALSE;
}

void realize(GtkWidget *widget)
{
  gtk_gl_area_make_current(GTK_GL_AREA(widget));
  if (gtk_gl_area_get_error(GTK_GL_AREA(widget)) != NULL) return;
  gtk_gl_area_attach_buffers(GTK_GL_AREA(widget));
  gtk_gl_area_set_auto_render(GTK_GL_AREA(widget), FALSE);

  setup();
  initialize_render_texture_buffers();

  std::ifstream fin(grammar_filename.c_str());
  std::string text;
  if (fin.is_open()) {
    std::string line;
    while (std::getline(fin, line)) {
      text += line + "\n";
    }
    fin.close();
  }
  g_signal_handlers_block_by_func(mybuffer, (gpointer)txt_inserted, NULL);
  gtk_text_buffer_set_text(mybuffer, text.c_str(), text.length());
  g_signal_handlers_unblock_by_func(mybuffer, (gpointer)txt_inserted, NULL);

  auto mydata = build_base_vertex_data();
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glGenBuffers(1, &position_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mydata.size(), mydata.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  GLuint vertex = create_shader(GL_VERTEX_SHADER);
  if (vertex == 0) return;
  GLuint fragment = create_shader(GL_FRAGMENT_SHADER);
  if (fragment == 0) {
    glDeleteShader(vertex);
    return;
  }

  int status;
  program = glCreateProgram();
  glAttachShader(program, vertex);
  glAttachShader(program, fragment);
  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status == GL_FALSE) {
    int log_len;
    char *buffer;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);
    buffer = (char*)g_malloc(log_len + 1);
    glGetProgramInfoLog(program, log_len, NULL, buffer);
    g_warning("Linking failure:\n%s", buffer);
    g_free(buffer);
    glDeleteProgram(program);
    program = 0;
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return;
  }

  glDetachShader(program, vertex);
  glDetachShader(program, fragment);
  glDeleteShader(vertex);
  glDeleteShader(fragment);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glViewport(0, 0,
             gtk_widget_get_allocated_width(widget),
             gtk_widget_get_allocated_height(widget));

  last_frame = g_date_time_new_now_local();
  current_frame = g_date_time_get_microsecond(last_frame);
  if (!text.empty()) {
    start_async_regeneration(false);
  }
}

void unrealize(GtkWidget *widget)
{
  gtk_gl_area_make_current(GTK_GL_AREA(widget));
  if (gtk_gl_area_get_error(GTK_GL_AREA(widget)) != NULL) return;
  for (int i = 0; i < render_texture_capacity; i++) {
    if (position_buffer1[i] != 0) glDeleteBuffers(1, &position_buffer1[i]);
    if (vao1[i] != 0) glDeleteVertexArrays(1, &vao1[i]);
    position_buffer1[i] = 0;
    vao1[i] = 0;
  }
  render_texture_capacity = 0;
  if (position_buffer != 0) glDeleteBuffers(1, &position_buffer);
  if (vao != 0) glDeleteVertexArrays(1, &vao);
  if (program != 0) glDeleteProgram(program);
  position_buffer = 0;
  vao = 0;
  program = 0;
}

void resize_glarea(GtkGLArea *area, int width, int height, gpointer)
{
  glarea_width = std::max(width, 1);
  glarea_height = std::max(height, 1);
  gtk_gl_area_make_current(area);
  if (gtk_gl_area_get_error(area) == NULL) {
    glViewport(0, 0, glarea_width, glarea_height);
  }
}

gboolean render(GtkGLArea *area, GdkGLContext *context)
{
  gdk_gl_context_make_current(context);
  GDateTime *date_time = g_date_time_new_now_local();
  if (last_frame == nullptr) {
    last_frame = g_date_time_ref(date_time);
  }
  current_frame = g_date_time_get_microsecond(date_time);
  delta_time = g_date_time_difference(date_time, last_frame) / 1000;
  g_date_time_unref(last_frame);
  last_frame = date_time;

  if (gtk_gl_area_get_error(area) != NULL) return FALSE;
  if (regeneration_in_progress.load()) return TRUE;
  if (play) angle_view += (float)delta_time / 1000.0f;
  if (angle_view > 360.0f) angle_view = 0.0f;

  glDisable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glClearColor(1.0, 1.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
  for (int i = 0; i < texture_list.size(); i++) {
    if (tex_count[i] > 0 && texture_list_alpha[i] >= 0.999f) {
      draw_buffer(i);
    }
  }

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_FALSE);
  for (int i = 0; i < texture_list.size(); i++) {
    if (tex_count[i] > 0 && texture_list_alpha[i] < 0.999f) {
      draw_buffer(i);
    }
  }
  glDepthMask(GL_TRUE);

  if (play) {
    gtk_gl_area_queue_render(area);
  }
  return TRUE;
}
