#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <list>
#include <string>
#include <unordered_map>
#include <stack>
#include <vector>
#include <filesystem>
#include <glob.h>
#include <random>
#include <atomic>
#include <array>
#include <memory>
#include <mutex>
#include <streambuf>


#include <stdio.h>
#include <bits/stdc++.h>
#include <termios.h>
#include <cstdlib>
#include <ctime>
#include <algorithm>



#include <epoxy/gl.h>
#include <GL/gl.h>
#include <GL/glu.h>

	

#include <stdio.h> /*standard i/o*/
#include <unistd.h> /*FOR USING FORK for at a time send and receive messages*/ 
#include <malloc.h> /*FOR MEMORY ALLOCATION */
#include <string.h> /*using fgets funtions for geting input from user*/
#include <sys/socket.h> /*for creating sockets*/
#include <resolv.h> /*server to find out the runner's IP address*/ 
#include <netdb.h> /*definitions for network database operations */
#include <openssl/ssl.h> /*using openssl function's and certificates and configuring them*/
#include <openssl/err.h> /* helps in finding out openssl errors*/
#include <unistd.h>  /*FOR USING FORK for at a time send and receive messages*/ 



#define FAIL    -1 /*for error output == -1 */
#define BUFFER  1024  /*buffer for reading messages*/




std::string hostname="xxx.xxx.xxx.xxx";
std::string portnum="pppp";

#include <gtk/gtk.h>
#if __has_include(<gtksourceview/gtksource.h>)
#include <gtksourceview/gtksource.h>
#define HAVE_GTKSOURCEVIEW 1
#else
#define HAVE_GTKSOURCEVIEW 0
#endif
// Project headers required by main.cpp
#include "grammar.h" // defines Grammar, Rule, Token, Variable
#include "Context.h" // ensure Context is fully defined for grammar->context uses
#include "Solution.h" // declares Noise2d
// Make sure GLM and stb_image types are available before using them
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "stbimage/stb_image.h"
#include "render.h"

// Forward / global state used across the UI and other translation units.
// Some of these were accidentally removed during edits; re-declare them here.
Grammar *grammar = nullptr;
std::string grammar_filename = "test.grammar";

// GTK buffers/widgets referenced throughout main.cpp
GtkTextBuffer *mybuffer = nullptr;
GtkTextBuffer *errorbuffer = nullptr;
GtkWidget *window = nullptr;
GtkWidget *view = nullptr;
GtkWidget *view2 = nullptr;
GtkWidget *viewerror = nullptr;
GtkWidget *gl_area = nullptr;
GtkWidget *root_shell = nullptr;
GtkWidget *top_toolbar_box = nullptr;
GtkWidget *layout3 = nullptr;
GtkWidget *layout4 = nullptr;
GtkWidget *layout = nullptr;
GtkWidget *texture_scrollwin = nullptr;
GtkWidget *file_label = nullptr;
GtkWidget *rulescombo = nullptr;
GtkWidget *openbutton = nullptr;
GtkWidget *resetviewbutton = nullptr;
GtkWidget *scale_slider_main = nullptr;
GtkWidget *grammar_mode_button = nullptr;
GtkWidget *rules_mode_button = nullptr;
GtkWidget *inspector_mode_button = nullptr;
GtkWidget *sketch_mode_button = nullptr;
GtkWidget *rule_summary_label = nullptr;
GtkWidget *rule_columns_label = nullptr;
GtkWidget *section_banner_labels[8];
int section_banner_count = 0;

// RNG used by Grammar.cpp (Grammar.cpp declares extern std::mt19937_64 rng;)
std::mt19937_64 rng;

// variable lists are defined in Grammar.cpp; declare externs so main.cpp can use them
extern std::vector<Variable *> variable_list;
extern std::vector<Variable *> full_variable_list;

std::vector<int> texture_list;
std::vector<float> texture_list_alpha;
std::vector<std::string> texture_filenames;
bool loggedin=false;

bool play=false;

// Common UI/global variables that are referenced in several places but
// weren't declared earlier after edits.
std::string username;
std::string password;
const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 900;
std::string appname = "Progen3d";
// View is now controlled by mouse

// Simple aliases for the GTK text tag variables used by the highlighter code
GtkTextTag *tag = nullptr;
GtkTextTag *tag2 = nullptr;
GtkTextTag *boldtag = nullptr;

// Mouse and camera control variables
float scale_global = 1.0f;
float angle_view = 0.0f;
float elevation_view = 0.0f;
double mousex = 0;
double mousey = 0;
bool mouse_dragging = false;
std::thread::id main_thread_id;
std::atomic<bool> regeneration_in_progress{false};

void display( void );
GtkWidget *labels[10000];
GtkWidget *ruleslabel[1000];
bool setup_textures=true;
bool setup_vars=true;
bool setup_rules=true;
GtkWidget *frames[140];
GtkBox *token_box[140];
GtkComboBoxText *combo1[140],*combo2[140],*combo3[140];
GtkComboBoxText *token_label[140];
GtkWidget *token_button[140];
GtkWidget *token_button_add[140];
GtkWidget *token_button_add_movedown[140];
GtkWidget *token_button_add_moveup[140];
int img_counter=0;
GtkWidget *button_remove_img[50];
GtkWidget *button_moveup_img[50];
GtkWidget *button_movedown_img[50];
GtkWidget *scale_slider_alpha[50];
GtkWidget *layout2;
GtkWidget *genlabel1;
GtkWidget *genlabel2;
GtkWidget *gencombo1;
GtkWidget *gencombo2;
GtkWidget *genlabelR1;
GtkWidget *gencomboR1;
GtkWidget *genlabelG1;
GtkWidget *gencomboG1;
GtkWidget *genlabelB1;
GtkWidget *gencomboB1;
 GtkWidget *entry1,*entry2;
 
 GtkWidget *box6,*box,*paned,*vpaned,*box3,*box4,*box5,*box8,*box7,*box9,*box10,*box11;
 
GtkActionBar *actionbar,*actionbar3,*actionbar7,*actionbar5,*actionbar6;
  GtkWidget *runbutton,*newbutton;
 GtkWidget *savebutton;
 GtkWidget *plybutton;
  GtkWidget *playbutton3;
  GtkWidget *stopbutton3;
  GtkWidget *texture_filechooser;
	GtkWidget *scale_slider3;
	GtkWidget *scale_slider4;

 GtkActionBar *actionbar2,*actionbar4;

char *rulenames[1000];

//GtkWidget *images[30];

std::vector<GtkWidget *> images;

int frame_counter=0;
bool init_buffer=false;
GtkAdjustment * vadj;

  

GtkFileChooserNative *native;
GtkFileChooser *chooser;
void generate_widgets();
void activate_add(GtkWidget *item) ;
void update_file_label();
void activate_show_grammar(GtkButton *item);
void activate_show_rules(GtkButton *item);
void activate_show_inspector(GtkButton *item);
void activate_show_sketch(GtkButton *item);
void activate(GtkButton *item);
void save_to_file(const char *filename);
void errorout(std::string error_str);
void debugout(const std::string &message);
void debugstate(const std::string &key, const std::string &message);
void upload_fulltext(std::string fulltext);
std::vector<std::string> breakup_into_lines(std::string input,std::string delimiter);
void start_async_regeneration(bool save_current_file);
void txt_inserted (GtkTextBuffer *textbuffer,
               GtkTextIter   *location,
               gchar         *text,
               gint           len,
               gpointer       user_data);
void txt_changed (GtkTextBuffer *textbuffer,gpointer       user_data);
void update_texture_layout_size();
static void on_texture_scrollwin_size_allocate(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data);
static void add_css_class(GtkWidget *widget, const char *css_class);
static void install_app_css();

static void show_main_workspace(){
	if(box6!=NULL && gtk_widget_get_parent(box6)!=NULL){
		gtk_widget_hide(box6);
	}
	if(root_shell!=NULL && paned!=NULL && gtk_widget_get_parent(paned)==NULL){
		gtk_box_pack_start(GTK_BOX(root_shell), (GtkWidget *)paned, 1, 1, 0);
	}
	if(window!=NULL){
		gtk_widget_show_all(GTK_WIDGET(window));
		gtk_window_present(GTK_WINDOW(window));
	}
}

static void add_css_class(GtkWidget *widget, const char *css_class){
	if(widget==NULL || css_class==NULL)return;
	gtk_style_context_add_class(gtk_widget_get_style_context(widget), css_class);
}

static void install_app_css(){
	static bool installed = false;
	if(installed)return;

	GtkCssProvider *provider = gtk_css_provider_new();
	const char *css = R"CSS(
.progen-window {
  background: #ddd0bc;
  color: #241c16;
}

.shell-header {
  background: #1f1a17;
  border: 1px solid #322922;
  border-radius: 22px;
  padding: 18px 22px;
}

.brand-kicker {
  color: #cab49a;
  font-size: 11px;
  font-weight: 700;
  letter-spacing: 1.5px;
}

.brand-title {
  color: #fff6ea;
  font-size: 24px;
  font-weight: 800;
}

.brand-note {
  color: rgba(255, 246, 234, 0.76);
}

.header-actions button,
.preview-actions button {
  background-image: none;
  background: #f6eadb;
  border: 1px solid #cfb99b;
  border-radius: 10px;
  box-shadow: none;
  color: #33271d;
  min-height: 34px;
  padding: 7px 14px;
}

.header-actions button:hover,
.preview-actions button:hover {
  background: #efdfca;
}

.header-actions button:active,
.preview-actions button:active {
  background: #e4d0b8;
}

.primary-action {
  background: #2e8752;
  border-color: #23663d;
  color: #ffffff;
  font-weight: 700;
}

.primary-action:hover {
  background: #379860;
}

.primary-action:active {
  background: #276f45;
}

.status-pill {
  background: #efe1cb;
  border: 1px solid #c8b294;
  border-radius: 999px;
  color: #433427;
  font-weight: 600;
  padding: 8px 14px;
}

.preview-pane,
.workspace-pane {
  background: transparent;
}

.column-kicker {
  color: #7a6754;
  font-size: 11px;
  font-weight: 700;
  letter-spacing: 1.3px;
}

.column-title {
  color: #2d2219;
  font-size: 18px;
  font-weight: 800;
}

.column-subtitle {
  color: #7f6d5a;
}

.surface-frame > border,
.viewport-frame > border {
  background: #fff9f0;
  border: 1px solid #d1bea7;
  border-radius: 18px;
}

.viewport-frame > border {
  background: #f4ecde;
  border-radius: 14px;
}

.panel-body {
  background: transparent;
  padding: 0;
}

.panel-label {
  color: #5d4b39;
  font-weight: 700;
  letter-spacing: 0.4px;
}

.control-label {
  color: #6a5643;
  font-weight: 700;
}

.editor-view,
.console-view {
  background: #fffdf8;
  color: #2f241b;
}

.console-view {
  background: #f4ecdf;
}
)CSS";

	GError *error = NULL;
	gtk_css_provider_load_from_data(provider, css, -1, &error);
	if(error!=NULL){
		debugout(std::string("install_app_css: ") + error->message);
		g_error_free(error);
		g_object_unref(provider);
		return;
	}

	GdkScreen *screen = gdk_screen_get_default();
	if(screen!=NULL){
		gtk_style_context_add_provider_for_screen(
			screen,
			GTK_STYLE_PROVIDER(provider),
			GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		installed = true;
	}
	g_object_unref(provider);
}

struct RegenerationResult {
	Grammar *grammar = nullptr;
	std::vector<std::vector<GLfloat>> vertex_buffers;
	std::vector<int> counts;
	unsigned long long texture_binding_version = 0;
	std::string error;
	std::string temp_filename;
	bool save_current_file = false;
	std::string save_filename;
};

std::vector<GdkPixbuf *> pixbuf;
GtkWidget *drawing_area;
int last_mouse_x = 0;
int last_mouse_y = 0;
void txt_changed (GtkTextBuffer *textbuffer,gpointer       user_data);

using glm::mat4;
using glm::vec3;
using glm::lookAt;
using glm::perspective;
using glm::rotate;
using glm::translate;
using glm::scale;

static std::mutex console_mutex;
static std::vector<std::string> pending_console_messages;
static bool console_flush_scheduled = false;
static std::mutex debug_state_mutex;
static std::unordered_map<std::string, std::string> debug_state_messages;
static std::mutex texture_binding_mutex;
static unsigned long long texture_binding_version = 0;
static void queue_console_message(std::string message);
static void flush_pending_console_messages();
static std::vector<int> snapshot_texture_bindings(unsigned long long *version_out = nullptr);
static unsigned long long current_texture_binding_version();
static void note_texture_binding_changed(const std::string &reason);

class GuiConsoleStreamBuf : public std::streambuf {
public:
	explicit GuiConsoleStreamBuf(std::string prefix = "")
		: prefix_(std::move(prefix)) {}

	~GuiConsoleStreamBuf() override {
		sync();
	}

protected:
	int overflow(int ch) override {
		if(ch == traits_type::eof()){
			return traits_type::not_eof(ch);
		}
		if(ch == '\r'){
			return ch;
		}
		if(ch == '\n'){
			flush_buffer(false);
			return ch;
		}
		buffer_.push_back(static_cast<char>(ch));
		return ch;
	}

	int sync() override {
		flush_buffer(true);
		return 0;
	}

private:
	void flush_buffer(bool flush_partial) {
		if(buffer_.empty() && !flush_partial){
			queue_console_message(prefix_);
			return;
		}
		if(buffer_.empty()){
			return;
		}
		queue_console_message(prefix_ + buffer_);
		buffer_.clear();
	}

	std::string prefix_;
	std::string buffer_;
};

static GuiConsoleStreamBuf gui_stdout_stream;
static GuiConsoleStreamBuf gui_stderr_stream;

static void install_console_stream_redirects(){
	static bool installed = false;
	if(installed){
		return;
	}
	std::cout.rdbuf(&gui_stdout_stream);
	std::cerr.rdbuf(&gui_stderr_stream);
	std::clog.rdbuf(&gui_stderr_stream);
	installed = true;
}

static void append_text_to_console(const std::string &text){
	if(errorbuffer==NULL)return;
	bool should_follow = true;
	if(viewerror!=NULL){
		GtkAdjustment *vadj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(viewerror));
		if(vadj!=NULL){
			double value = gtk_adjustment_get_value(vadj);
			double page_size = gtk_adjustment_get_page_size(vadj);
			double upper = gtk_adjustment_get_upper(vadj);
			should_follow = value + page_size >= upper - 8.0;
		}
	}
	GtkTextIter end;
	gtk_text_buffer_get_end_iter(errorbuffer,&end);
	gtk_text_buffer_insert(errorbuffer,&end,text.c_str(),text.length());
	if(viewerror!=NULL && should_follow){
		gtk_text_buffer_get_end_iter(errorbuffer,&end);
		GtkTextMark* mark = gtk_text_buffer_create_mark(errorbuffer, NULL, &end, 1);
		gtk_text_view_scroll_to_mark((GtkTextView *)viewerror, mark, 0.0, 0, 0.0, 1.0);
		gtk_text_buffer_delete_mark (errorbuffer, mark);
	}
}

static gboolean console_append_idle(gpointer){
	flush_pending_console_messages();
	return G_SOURCE_REMOVE;
}

static void flush_pending_console_messages(){
	if(errorbuffer==NULL){
		return;
	}
	std::vector<std::string> pending_messages;
	{
		std::lock_guard<std::mutex> lock(console_mutex);
		if(pending_console_messages.empty()){
			console_flush_scheduled = false;
			return;
		}
		pending_messages.swap(pending_console_messages);
		console_flush_scheduled = false;
	}
	for(const std::string &message : pending_messages){
		append_text_to_console(message);
	}
}

static void queue_console_message(std::string message){
	if(message.empty() || message.back()!='\n'){
		message.push_back('\n');
	}
	auto current = std::this_thread::get_id();
	bool on_main_thread = (main_thread_id == std::thread::id() || current == main_thread_id);
	bool schedule_idle = false;
	{
		std::lock_guard<std::mutex> lock(console_mutex);
		pending_console_messages.push_back(std::move(message));
		if(errorbuffer!=NULL && !on_main_thread && !console_flush_scheduled){
			console_flush_scheduled = true;
			schedule_idle = true;
		}
	}
	if(errorbuffer==NULL){
		return;
	}
	if(on_main_thread){
		flush_pending_console_messages();
		return;
	}
	if(schedule_idle){
		g_idle_add(console_append_idle, NULL);
	}
}

static void set_regeneration_ui_state(bool busy){
	if(runbutton!=NULL)gtk_widget_set_sensitive(runbutton, !busy);
	if(openbutton!=NULL)gtk_widget_set_sensitive(openbutton, !busy);
	if(newbutton!=NULL)gtk_widget_set_sensitive(newbutton, !busy);
	if(savebutton!=NULL)gtk_widget_set_sensitive(savebutton, !busy);
	if(plybutton!=NULL)gtk_widget_set_sensitive(plybutton, !busy);
	if(resetviewbutton!=NULL)gtk_widget_set_sensitive(resetviewbutton, !busy);
	if(texture_filechooser!=NULL)gtk_widget_set_sensitive(texture_filechooser, !busy);
	if(texture_scrollwin!=NULL)gtk_widget_set_sensitive(texture_scrollwin, !busy);
	if(file_label!=NULL && busy){
		gtk_label_set_text(GTK_LABEL(file_label), "Regenerating...");
	}
}


static gboolean finalize_regeneration_idle(gpointer data){
	std::unique_ptr<RegenerationResult> result(static_cast<RegenerationResult *>(data));
	regeneration_in_progress = false;
	set_regeneration_ui_state(false);

	if(!result->temp_filename.empty()){
		std::error_code ec;
		std::filesystem::remove(result->temp_filename, ec);
	}

	if(!result->error.empty() || result->grammar == nullptr){
		std::string failure_text = result->error.empty() ? "Regeneration failed: unknown error" : ("Regeneration failed: " + result->error);
		debugout(failure_text);
		update_file_label();
		if(!result->error.empty())errorout(result->error);
		if(gl_area!=NULL)gtk_gl_area_queue_render(GTK_GL_AREA(gl_area));
		return G_SOURCE_REMOVE;
	}

	if(current_texture_binding_version() != result->texture_binding_version){
		debugout("finalize_regeneration_idle: discarding stale regeneration result because texture bindings changed");
		if(result->grammar!=NULL){
			delete result->grammar;
			result->grammar = NULL;
		}
		start_async_regeneration(result->save_current_file);
		return G_SOURCE_REMOVE;
	}

	Grammar *old_grammar = grammar;
	grammar = result->grammar;
	update_file_label();
	upload_render_buffers(result->vertex_buffers, result->counts);
	if(result->save_current_file){
		save_to_file(result->save_filename.c_str());
	}
	std::stringstream success_msg;
	success_msg<<"Regeneration succeeded: vertex_buffers="<<result->vertex_buffers.size()
	           <<", counts="<<result->counts.size();
	debugout(success_msg.str());
	if(old_grammar!=NULL){
		delete old_grammar;
	}
	return G_SOURCE_REMOVE;
}

static std::vector<int> snapshot_texture_bindings(unsigned long long *version_out){
	std::lock_guard<std::mutex> lock(texture_binding_mutex);
	if(version_out!=NULL)*version_out = texture_binding_version;
	return texture_list;
}

static unsigned long long current_texture_binding_version(){
	std::lock_guard<std::mutex> lock(texture_binding_mutex);
	return texture_binding_version;
}

static void note_texture_binding_changed(const std::string &reason){
	unsigned long long version = 0;
	{
		std::lock_guard<std::mutex> lock(texture_binding_mutex);
		version = ++texture_binding_version;
	}
	debugout("texture bindings updated: " + reason + ", version=" + std::to_string(version));
}

void start_async_regeneration(bool save_current_file){
	if(regeneration_in_progress.exchange(true)){
		errorout("Regeneration already in progress");
		return;
	}
	if(mybuffer==NULL){
		regeneration_in_progress = false;
		return;
	}

	play=false;
	GtkTextIter start, end;
	gtk_text_buffer_get_start_iter(mybuffer, &start);
	gtk_text_buffer_get_end_iter(mybuffer, &end);
	std::string fulltext = (std::string)gtk_text_buffer_get_text(mybuffer, &start, &end, FALSE);
	std::stringstream debug_msg;
	debug_msg<<"start_async_regeneration: save_current_file="<<(save_current_file ? "true" : "false")
	         <<", text_len="<<fulltext.size();
	debugout(debug_msg.str());
	std::string save_filename_snapshot = grammar_filename;
	unsigned long long texture_binding_version_snapshot = 0;
	std::vector<int> texture_list_snapshot = snapshot_texture_bindings(&texture_binding_version_snapshot);
	set_regeneration_ui_state(true);

	std::thread([fulltext,
	            save_current_file,
	            save_filename_snapshot,
	            texture_list_snapshot,
	            texture_binding_version_snapshot]() {
		std::unique_ptr<RegenerationResult> result(new RegenerationResult());
		result->save_current_file = save_current_file;
		result->save_filename = save_filename_snapshot;
		result->texture_binding_version = texture_binding_version_snapshot;
		try{
			auto temp_path = std::filesystem::temp_directory_path() /
				("progen3d-regeneration-" +
				 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
				 ".grammar");
			result->temp_filename = temp_path.string();
			std::ofstream fout(result->temp_filename);
			fout << fulltext;
			fout.close();

			if(loggedin==true){
				upload_fulltext(fulltext);
			}

			Grammar *new_grammar = new Grammar(result->temp_filename);
			new_grammar->lines = breakup_into_lines(fulltext,"\n");
			if(!new_grammar->lines.empty()){
				new_grammar->Reread();
			}
			new_grammar->addContext();
			for(size_t i=0;i<texture_list_snapshot.size();i++){
				new_grammar->context->loadTexture(texture_list_snapshot[i]);
			}
			new_grammar->context->genPrimitives();
			new_grammar->generateGeometry();

			auto mydata = build_base_vertex_data();
			new_grammar->context->buildTextureBuffers(mydata.data(),
			                                         texture_list_snapshot.size(),
			                                         &result->vertex_buffers,
			                                         &result->counts);
			result->grammar = new_grammar;
		}
		catch(...){
			result->error = "Error regenerating grammar";
		}
		g_idle_add(finalize_regeneration_idle, result.release());
	}).detach();
}






void errorout(std::string error_str){
	queue_console_message(error_str);
}

void debugout(const std::string &message){
	queue_console_message(std::string("[DEBUG] ") + message);
}

void debugstate(const std::string &key, const std::string &message){
	std::lock_guard<std::mutex> lock(debug_state_mutex);
	auto it = debug_state_messages.find(key);
	if(it != debug_state_messages.end() && it->second == message){
		return;
	}
	debug_state_messages[key] = message;
	debugout(message);
}

static void log_widget_allocation_state(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data){
	const char *name = static_cast<const char *>(user_data);
	std::stringstream ss;
	ss<<"gui: "<<name
	  <<" size="<<allocation->width<<"x"<<allocation->height
	  <<" visible="<<(gtk_widget_get_visible(widget) ? "true" : "false")
	  <<" realized="<<(gtk_widget_get_realized(widget) ? "true" : "false")
	  <<" mapped="<<(gtk_widget_get_mapped(widget) ? "true" : "false");
	debugstate(std::string("widget:") + name, ss.str());
}

namespace {
const char *kGuiDebugInstalledKey = "progen3d-gui-debug-installed";
thread_local bool gui_event_log_in_progress = false;
}

static bool begin_gui_event_log(){
	if(gui_event_log_in_progress){
		return false;
	}
	gui_event_log_in_progress = true;
	return true;
}

static void end_gui_event_log(){
	gui_event_log_in_progress = false;
}

static std::string widget_debug_name(GtkWidget *widget){
	if(widget == NULL){
		return "null";
	}

	std::stringstream ss;
	ss << G_OBJECT_TYPE_NAME(widget);
	const char *widget_name = gtk_widget_get_name(widget);
	if(widget_name != NULL && widget_name[0] != '\0'){
		ss << "[" << widget_name << "]";
	}
	ss << "@" << widget;
	return ss.str();
}

static const char *gdk_event_type_name(GdkEventType type){
	switch(type){
		case GDK_NOTHING: return "nothing";
		case GDK_DELETE: return "delete";
		case GDK_DESTROY: return "destroy";
		case GDK_EXPOSE: return "expose";
		case GDK_MOTION_NOTIFY: return "motion-notify";
		case GDK_BUTTON_PRESS: return "button-press";
		case GDK_2BUTTON_PRESS: return "double-button-press";
		case GDK_3BUTTON_PRESS: return "triple-button-press";
		case GDK_BUTTON_RELEASE: return "button-release";
		case GDK_KEY_PRESS: return "key-press";
		case GDK_KEY_RELEASE: return "key-release";
		case GDK_ENTER_NOTIFY: return "enter-notify";
		case GDK_LEAVE_NOTIFY: return "leave-notify";
		case GDK_FOCUS_CHANGE: return "focus-change";
		case GDK_CONFIGURE: return "configure";
		case GDK_MAP: return "map";
		case GDK_UNMAP: return "unmap";
		case GDK_PROPERTY_NOTIFY: return "property-notify";
		case GDK_SELECTION_CLEAR: return "selection-clear";
		case GDK_SELECTION_REQUEST: return "selection-request";
		case GDK_SELECTION_NOTIFY: return "selection-notify";
		case GDK_PROXIMITY_IN: return "proximity-in";
		case GDK_PROXIMITY_OUT: return "proximity-out";
		case GDK_DRAG_ENTER: return "drag-enter";
		case GDK_DRAG_LEAVE: return "drag-leave";
		case GDK_DRAG_MOTION: return "drag-motion";
		case GDK_DRAG_STATUS: return "drag-status";
		case GDK_DROP_START: return "drop-start";
		case GDK_DROP_FINISHED: return "drop-finished";
		case GDK_CLIENT_EVENT: return "client-event";
		case GDK_VISIBILITY_NOTIFY: return "visibility-notify";
		case GDK_SCROLL: return "scroll";
		case GDK_WINDOW_STATE: return "window-state";
		case GDK_SETTING: return "setting";
		case GDK_OWNER_CHANGE: return "owner-change";
		case GDK_GRAB_BROKEN: return "grab-broken";
		case GDK_DAMAGE: return "damage";
		case GDK_TOUCH_BEGIN: return "touch-begin";
		case GDK_TOUCH_UPDATE: return "touch-update";
		case GDK_TOUCH_END: return "touch-end";
		case GDK_TOUCH_CANCEL: return "touch-cancel";
		case GDK_TOUCHPAD_SWIPE: return "touchpad-swipe";
		case GDK_TOUCHPAD_PINCH: return "touchpad-pinch";
		case GDK_PAD_BUTTON_PRESS: return "pad-button-press";
		case GDK_PAD_BUTTON_RELEASE: return "pad-button-release";
		case GDK_PAD_RING: return "pad-ring";
		case GDK_PAD_STRIP: return "pad-strip";
		case GDK_PAD_GROUP_MODE: return "pad-group-mode";
		default: return "other";
	}
}

static void emit_gui_debug_message(const std::string &message){
	if(!begin_gui_event_log()){
		return;
	}
	debugout(message);
	end_gui_event_log();
}

static void log_widget_signal_event(GtkWidget *widget, const char *signal_name){
	std::stringstream ss;
	ss << "gui-signal: " << signal_name
	   << " widget=" << widget_debug_name(widget);
	emit_gui_debug_message(ss.str());
}

static void log_widget_signal(GtkWidget *widget, gpointer user_data){
	log_widget_signal_event(widget, static_cast<const char *>(user_data));
}

static gboolean log_widget_gdk_event(GtkWidget *widget, GdkEvent *event, gpointer user_data){
	std::stringstream ss;
	ss << "gui-event: phase=" << static_cast<const char *>(user_data)
	   << " type=" << gdk_event_type_name(event->type)
	   << " widget=" << widget_debug_name(widget);

	GtkWidget *event_widget = gtk_get_event_widget(event);
	if(event_widget != NULL && event_widget != widget){
		ss << " event_widget=" << widget_debug_name(event_widget);
	}

	switch(event->type){
		case GDK_BUTTON_PRESS:
		case GDK_2BUTTON_PRESS:
		case GDK_3BUTTON_PRESS:
		case GDK_BUTTON_RELEASE:
			ss << " button=" << event->button.button
			   << " x=" << event->button.x
			   << " y=" << event->button.y;
			break;
		case GDK_MOTION_NOTIFY:
			ss << " x=" << event->motion.x
			   << " y=" << event->motion.y;
			break;
		case GDK_SCROLL:
			ss << " x=" << event->scroll.x
			   << " y=" << event->scroll.y
			   << " delta_x=" << event->scroll.delta_x
			   << " delta_y=" << event->scroll.delta_y
			   << " direction=" << event->scroll.direction;
			break;
		case GDK_KEY_PRESS:
		case GDK_KEY_RELEASE:
			ss << " keyval=" << event->key.keyval
			   << " hardware_keycode=" << event->key.hardware_keycode
			   << " state=" << event->key.state;
			break;
		case GDK_FOCUS_CHANGE:
			ss << " in=" << (event->focus_change.in ? "true" : "false");
			break;
		case GDK_CONFIGURE:
			ss << " x=" << event->configure.x
			   << " y=" << event->configure.y
			   << " width=" << event->configure.width
			   << " height=" << event->configure.height;
			break;
		case GDK_ENTER_NOTIFY:
		case GDK_LEAVE_NOTIFY:
			ss << " x=" << event->crossing.x
			   << " y=" << event->crossing.y
			   << " mode=" << event->crossing.mode
			   << " detail=" << event->crossing.detail;
			break;
		case GDK_WINDOW_STATE:
			ss << " changed_mask=" << event->window_state.changed_mask
			   << " new_state=" << event->window_state.new_window_state;
			break;
		default:
			break;
	}

	emit_gui_debug_message(ss.str());
	return FALSE;
}

static void log_widget_gdk_event_after(GtkWidget *widget, GdkEvent *event, gpointer user_data){
	log_widget_gdk_event(widget, event, user_data);
}

static void log_widget_size_allocate(GtkWidget *widget, GdkRectangle *allocation, gpointer){
	std::stringstream ss;
	ss << "gui-signal: size-allocate"
	   << " widget=" << widget_debug_name(widget)
	   << " width=" << allocation->width
	   << " height=" << allocation->height;
	emit_gui_debug_message(ss.str());
}

static void instrument_widget_tree(GtkWidget *widget);

static void on_debug_container_add(GtkContainer *container, GtkWidget *child, gpointer){
	std::stringstream ss;
	ss << "gui-signal: add"
	   << " container=" << widget_debug_name(GTK_WIDGET(container))
	   << " child=" << widget_debug_name(child);
	emit_gui_debug_message(ss.str());
	instrument_widget_tree(child);
}

static void on_debug_container_remove(GtkContainer *container, GtkWidget *child, gpointer){
	std::stringstream ss;
	ss << "gui-signal: remove"
	   << " container=" << widget_debug_name(GTK_WIDGET(container))
	   << " child=" << widget_debug_name(child);
	emit_gui_debug_message(ss.str());
}

static void instrument_widget_tree(GtkWidget *widget){
	if(widget == NULL){
		return;
	}
	if(g_object_get_data(G_OBJECT(widget), kGuiDebugInstalledKey) != NULL){
		return;
	}

	g_object_set_data(G_OBJECT(widget), kGuiDebugInstalledKey, GINT_TO_POINTER(1));
	g_signal_connect(widget, "event", G_CALLBACK(log_widget_gdk_event), (gpointer)"before");
	g_signal_connect(widget, "event-after", G_CALLBACK(log_widget_gdk_event_after), (gpointer)"after");
	g_signal_connect(widget, "show", G_CALLBACK(log_widget_signal), (gpointer)"show");
	g_signal_connect(widget, "hide", G_CALLBACK(log_widget_signal), (gpointer)"hide");
	g_signal_connect(widget, "map", G_CALLBACK(log_widget_signal), (gpointer)"map");
	g_signal_connect(widget, "unmap", G_CALLBACK(log_widget_signal), (gpointer)"unmap");
	g_signal_connect(widget, "realize", G_CALLBACK(log_widget_signal), (gpointer)"realize");
	g_signal_connect(widget, "unrealize", G_CALLBACK(log_widget_signal), (gpointer)"unrealize");
	g_signal_connect(widget, "destroy", G_CALLBACK(log_widget_signal), (gpointer)"destroy");
	g_signal_connect(widget, "size-allocate", G_CALLBACK(log_widget_size_allocate), NULL);

	if(GTK_IS_CONTAINER(widget)){
		g_signal_connect(widget, "add", G_CALLBACK(on_debug_container_add), NULL);
		g_signal_connect(widget, "remove", G_CALLBACK(on_debug_container_remove), NULL);
		gtk_container_forall(GTK_CONTAINER(widget),
		                     [](GtkWidget *child, gpointer){
			                     instrument_widget_tree(child);
		                     },
		                     NULL);
	}
}

static void install_gui_event_logging(GtkWidget *root_widget){
	if(root_widget == NULL){
		return;
	}

	gtk_widget_add_events(root_widget, GDK_ALL_EVENTS_MASK);
	instrument_widget_tree(root_widget);
	debugout("gui-event logging installed");
}

std::vector<std::string> breakup_into_lines(std::string input,std::string delimiter){
		std::vector<std::string> output;
	
		int pos=-1;
		
		while( (pos = input.find(delimiter))!=-1){
	
			std::string str=input.substr(0, pos);
			
			input.erase(0, pos + delimiter.length());
		
		    output.push_back(str);
		}
		output.push_back(input);
		
		
		
		return output;
}

static GdkPixbuf *load_texture_preview_pixbuf(const std::string &filename, int width = 128, int height = 128){
	GError *error = NULL;
	GdkPixbuf *loaded = gdk_pixbuf_new_from_file(filename.c_str(), &error);
	if(loaded == NULL){
		if(error != NULL){
			errorout(std::string("Failed to load texture preview: ") + error->message);
			g_error_free(error);
		}
		else{
			errorout(std::string("Failed to load texture preview: ") + filename);
		}
		return NULL;
	}

	GdkPixbuf *scaled = gdk_pixbuf_scale_simple(loaded, width, height, GDK_INTERP_BILINEAR);
	g_object_unref(loaded);
	if(scaled == NULL){
		errorout(std::string("Failed to scale texture preview: ") + filename);
	}
	return scaled;
}

static bool append_texture_preview_image(const std::string &filename){
	GdkPixbuf *preview = load_texture_preview_pixbuf(filename);
	if(preview == NULL)return false;
	pixbuf.push_back(preview);
	images.push_back(gtk_image_new_from_pixbuf(preview));
	return true;
}


// The project previously contained a JavaScript-style tokenizer class here
// (EnhancedTokenizer). That accidental JS code was removed because the
// C++ project uses the Grammar.* implementation for tokenization and
// parsing. If a C++ tokenizer is required in future, implement it here.



mat4 model = mat4(1.0);

namespace {
constexpr int kTextureGridMargin = 16;
constexpr int kTextureGridGap = 16;
constexpr int kTextureCardWidth = 208;
constexpr int kTextureCardHeight = 144;
constexpr int kTextureButtonOffsetX = 168;
}

static int texture_grid_available_width(){
	int available_width = 0;
	if(texture_scrollwin != NULL){
		available_width = gtk_widget_get_allocated_width(texture_scrollwin);
	}
	if(available_width <= 0){
		available_width = 720;
	}
	return available_width;
}

static int texture_grid_column_count(){
	int inner_width = std::max(kTextureCardWidth + kTextureGridMargin * 2,
	                           texture_grid_available_width() - 24);
	int usable_width = std::max(kTextureCardWidth,
	                            inner_width - kTextureGridMargin * 2 + kTextureGridGap);
	return std::max(1, usable_width / (kTextureCardWidth + kTextureGridGap));
}

static void relayout_texture_items(){
	if(layout2 == NULL)return;
	int columns = texture_grid_column_count();
	for(int i=0;i<img_counter;i++){
		int row = i / columns;
		int column = i % columns;
		int base_x = kTextureGridMargin + column * (kTextureCardWidth + kTextureGridGap);
		int base_y = kTextureGridMargin + row * (kTextureCardHeight + kTextureGridGap);
		if(i < images.size() && images[i] != NULL && gtk_widget_get_parent(images[i]) != NULL){
			gtk_layout_move(GTK_LAYOUT(layout2), images[i], base_x, base_y);
		}
		if(button_remove_img[i] != NULL && gtk_widget_get_parent(button_remove_img[i]) != NULL){
			gtk_layout_move(GTK_LAYOUT(layout2), button_remove_img[i], base_x + kTextureButtonOffsetX, base_y);
		}
		if(button_moveup_img[i] != NULL && gtk_widget_get_parent(button_moveup_img[i]) != NULL){
			gtk_layout_move(GTK_LAYOUT(layout2), button_moveup_img[i], base_x + kTextureButtonOffsetX, base_y + 30);
		}
		if(button_movedown_img[i] != NULL && gtk_widget_get_parent(button_movedown_img[i]) != NULL){
			gtk_layout_move(GTK_LAYOUT(layout2), button_movedown_img[i], base_x + kTextureButtonOffsetX, base_y + 60);
		}
	}
}

void update_texture_layout_size(){
	if(layout2==NULL)return;
	int column_count = texture_grid_column_count();
	int row_count = std::max(1, (img_counter + column_count - 1) / column_count);
	int layout_width = std::max(texture_grid_available_width(),
	                            kTextureGridMargin * 2 + column_count * kTextureCardWidth +
	                            std::max(0, column_count - 1) * kTextureGridGap);
	int layout_height = kTextureGridMargin * 2 + row_count * kTextureCardHeight +
		std::max(0, row_count - 1) * kTextureGridGap;
	gtk_layout_set_size(GTK_LAYOUT(layout2), layout_width, layout_height);
	relayout_texture_items();
}

static void on_texture_scrollwin_size_allocate(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data){
	update_texture_layout_size();
}



int OpenConnection(const char *hostname, int port)

{
	   int sd;
	struct hostent *host;
	struct sockaddr_in addr;   /*creating the sockets*/

	if ( (host = gethostbyname(hostname)) == NULL )
	{
		perror(hostname);
		abort();
	}
	sd = socket(PF_INET, SOCK_STREAM, 0);   /* setting the connection as tcp it creates endpoint for connection */
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = *(long*)(host->h_addr);
	if ( connect(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )  /*initiate a connection on a socket*/
	{
		close(sd);
		//perror(hostname);
		//abort();
		return -1;
	}
return sd;
}

SSL_CTX* InitCTX(void)     /*creating and setting up ssl context structure*/
{
    const SSL_METHOD *method;
	SSL_CTX *ctx;
	
	OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
	SSL_load_error_strings();   /* Bring in and register error messages */
	method = SSLv23_client_method();//TLSv1_2_client_method();  /* Create new client-method instance */
	ctx = SSL_CTX_new(method);   /* Create new context */
	if ( ctx == NULL )
	{
		//ERR_print_errors_fp(stderr);
		//abort();
		return NULL;
	}

return ctx;
}
void LoadCertificates(SSL_CTX* ctx, std::string CertFile, std::string KeyFile)   /* to load a certificate into an SSL_CTX structure*/
{	/* set the local certificate from CertFile */
	if ( SSL_CTX_use_certificate_file(ctx, CertFile.c_str(), SSL_FILETYPE_PEM) <= 0 )	{
		std::stringstream ss;
			ss<< "use certificate failed "<<CertFile<<std::endl;
			errorout(ss.str());
			//exit(0);
	}	
	/* set the private key from KeyFile (may be the same as CertFile) */
	//std::string passphrase="########";
	//SSL_CTX_set_default_passwd_cb_userdata(ctx,(void *)passphrase.c_str());
	//if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile.c_str(), SSL_FILETYPE_PEM) <= 0 )	{	
		//std::cout<< "use privatekey failed"<<std::endl;
	//exit(0);
	//}	/* verify private key */
	//if ( !SSL_CTX_check_private_key(ctx) )	{
	//		std::cout<< "Private key does not match the public certificate"<<std::endl;
	//	//	exit(0);
	
	//}

}
void ShowCerts(SSL* ssl)  /*show the ceritficates to server and match them but here we are not using any client certificate*/

{   X509 *cert;
	char *line;
	
	cert = SSL_get_peer_certificate(ssl); /* get the server's certificate */
	if ( cert != NULL )
	{	
		std::stringstream ss;
		ss<<"Server certificates:"<<std::endl;
		line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
		ss<<"Subject: "<<line<<std::endl;
		free(line);       /* free the malloc'ed string */
		line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
		ss<<"Issuer: "<<line<<std::endl;
		free(line);       /* free the malloc'ed string */
		X509_free(cert);     /* free the malloc'ed certificate copy */
		errorout(ss.str());
	}
	else
		errorout("Info: No client certificates configured.");

}


int generateTextureFromBuf(unsigned char *buf)
{
	GLuint mTexture;
    glGenTextures(1 , &mTexture);
    glBindTexture(GL_TEXTURE_2D, mTexture);// Bind our 2D texture so that following set up will be applied

    //Set texture wrapping parameter
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

    //Set texture Filtering parameter
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);


    //Generate the image
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB , 128 , 128, 0, GL_RGB, GL_UNSIGNED_BYTE, buf);
    //glGenerateMipmap(GL_TEXTURE_2D);
    
    
    
    
    glBindTexture(GL_TEXTURE_2D,0); //Unbind 2D textures
    
    return mTexture;

}


int generateTexture(const char * filename)
{
	GLuint mTexture;
    glGenTextures(1 , &mTexture);
    glBindTexture(GL_TEXTURE_2D, mTexture);// Bind our 2D texture so that following set up will be applied

    //Set texture wrapping parameter
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

    //Set texture Filtering parameter
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

    //Load the image
    int picWidth,picHeight,n;
    unsigned char* image = stbi_load(filename, &picWidth, &picHeight, &n,STBI_rgb_alpha);
    if (image == NULL ) {
		std::stringstream ss;
        ss<<"Failed to load image: "<<stbi_failure_reason()<<std::endl;
        errorout(ss.str());
    }
    //Generate the image
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA , picWidth , picHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    //glGenerateMipmap(GL_TEXTURE_2D);
    
    //texture_data_list.push_back(image);
    
   // std::cout<<"Image START#####"<<picWidth<<":"<<picHeight<<":"<<picWidth*picHeight*3<<"#####################"<<std::endl;
   // for(int i=0;i<picWidth*picHeight*3;i++)std::cout<<(unsigned int)image[i]<<",";
   // std::cout<<"Image END##########################"<<std::endl;

    stbi_image_free(image);// Free the reference to the image
    glBindTexture(GL_TEXTURE_2D,0); //Unbind 2D textures
    
    return mTexture;

}



std::vector<std::string> globVector(const std::string& pattern){
	try{
    glob_t glob_result;
    memset(&glob_result, 0, sizeof(glob_result));
    
     int return_value,count=0;
    while((return_value=glob(pattern.c_str(),GLOB_TILDE,NULL,&glob_result))!=0){
		 
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		if(count++>30){
			throw(1);
			
		}
	}
    
    
    std::vector<std::string> files;
    for(unsigned int i=0;i<glob_result.gl_pathc;++i){
        files.push_back(std::string(glob_result.gl_pathv[i]));
    }
    globfree(&glob_result);
    return files;
    
}catch(...){
	errorout("glob error");
	std::vector<std::string> out;
	return out;
}
}


void   setup(){
  
    
    errorout("setup");

    
   

	srand(time(NULL));
    //std::cout << std::fixed << std::showpoint;
	std::cout.precision(2);
	
	errorout("Read files...");
	
	
if(loggedin==true){
	SSL_CTX *ctx;
	int client;
	SSL *ssl;
	char buf[65536];
	char input[BUFFER];
	int bytes;
	std::string writeout;
	

ctx = InitCTX();
LoadCertificates(ctx, "mycert.pem", "mycertkey.pem");
	client= OpenConnection(hostname.c_str(), atoi(portnum.c_str()));   /*converting ascii port to interger */
	ssl = SSL_new(ctx);      /* create new SSL connection state */
	SSL_set_fd(ssl, client);    /* attach the socket descriptor */
	if ( SSL_connect(ssl) != FAIL )   /* perform the connection */
	{    
	
		std::stringstream ss;
		ss<<"Connected with encryption "<< SSL_get_cipher(ssl)<<std::endl;
		errorout(ss.str());
		ShowCerts(ssl);
		/* get any certs */


	
			
			
			SSL_write(ssl, username.c_str(), strlen(username.c_str()));   /* encrypt & send message */
			bytes = SSL_read(ssl, buf, sizeof(buf)); /* get request */
			SSL_write(ssl, password.c_str(), strlen(password.c_str()));   /* encrypt & send message */
		
			bytes = SSL_read(ssl, buf, sizeof(buf)); /* get request */
			buf[bytes]=0;
			std::string OK=buf;
			//if(OK=="DOWN"){
				/*writeout="TEXTURE";
				std::cout<<writeout<<std::endl;
				SSL_write(ssl, writeout.c_str(), strlen(writeout.c_str()));
				img_counter=0;
				
				while(1){
					
					
					unsigned char ubuf[65536];
					int counter=0;
					
					int dec1,dec2;
					std::string bufferstr="";
					for(int i=0;i<20;i++){
					bytes = SSL_read(ssl, buf ,sizeof(buf));
						if(bytes>0){
							buf[bytes]=0;
						}
						bufferstr+=buf;
					}
					if(bufferstr=="OK" || bufferstr.size()<30 )break;
					
					std::vector<std::string> strdata=breakup_into_lines(bufferstr,",");
					for(int i=0;i<strdata.size();i++){
						//std::cout<<strdata[i]<<" ";
						ubuf[i]=(uint8_t)atoi(strdata[i].c_str());
					}
					if(strdata.size()<2)break;
					std::cout<<strdata.size()<<std::endl;
					
					int texid;
					
					if(strdata.size()>=128*128*3-15 && strdata.size()<=128*128*3+1 ){
						texid=generateTextureFromBuf((unsigned char *)ubuf);
						texture_list.push_back(texid);

						pixsbuffer[img_counter]=(unsigned char *)malloc(128*128*3);
						memcpy(pixsbuffer[img_counter],ubuf,128*128*3);
						
						pixbufs[img_counter]=gdk_pixbuf_new_from_data((const guchar*)pixsbuffer[img_counter],GDK_COLORSPACE_RGB,FALSE,8,128,128,128 * 3,NULL,NULL);
						img_counter++;
						gtk_widget_queue_draw_area(drawing_area2,0,0, draw2width,draw2height);
					}
					
				}*/
			//}
			 if(OK=="UP"){
								std::ifstream fin(grammar_filename.c_str());
								if (!fin.is_open()) {
									std::cout << "E: Could not open file " << grammar_filename << std::endl;
								}
								std::string fulltext;
								std::string line;
								while(std::getline(fin, line) ) {
								  fulltext+=line;
								}
								fin.close();
								SSL_write(ssl, fulltext.c_str(), strlen(fulltext.c_str())); 
								
							
					
			}
		
	}
	SSL_free(ssl);        /* release connection state */
	close(client);         /* close socket */
	SSL_CTX_free(ctx);        /* release */
	
}
else{
	std::vector<std::string> files = globVector("/snap/progen3d/current/textures/*.png");
	texture_filenames=files;
	std::stringstream ss;
	
	ss<<"files: "<<files.size()<<std::endl;
	errorout(ss.str());

	
	
	
	int texid;
	
	int counter=0;
    for (int i=0;i<files.size();i++){
		//std::cout<<counter;
		counter++;
		texid=generateTexture(files[i].c_str());
		texture_list.push_back(texid);
		texture_list_alpha.push_back(1.0);
		append_texture_preview_image(files[i]);
		
	//pixbufs[img_counter] = gdk_pixbuf_new_from_file(files[i].c_str(), &err);
	img_counter++;
	    
	}
	if(!texture_list.empty()){
		note_texture_binding_changed("initial texture load");
	}
}
	/*std::vector<std::string> files2 = globVector("./*.jpg");
	
	
	std::cout<<"files: "<<files2.size()<<std::endl;
	
	
    for (int i=0;i<files2.size();i++){
		std::cout<<counter;
		counter++;
		GLuint texid=generateTexture(files2[i].c_str());
		texture_list.push_back(texid);
	
	}*/
	
	initialize_render_texture_buffers();

	update_file_label();
	init_buffer=false;



}
  
  
  





















//             MAIN



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////





  void save_to_file(const char *filename){
   std::ofstream fout(filename);
    if (!fout.is_open()) {
        std::cerr << "E: Could not open file for saving" << filename << std::endl;
        return;
    }
    
    for(int i=0;i<grammar->lines.size();i++){
		fout<<grammar->lines[i]<<std::endl;
	}	
fout.close();
}

void update_file_label(){
	if(file_label==NULL)return;
	if(grammar!=NULL){
		gtk_label_set_text(
			(GtkLabel *)file_label,
			("File: "+grammar_filename+"    Tokens: "+std::to_string(grammar->tokens_new.size())).c_str()
		);
	}
	else {
		gtk_label_set_text((GtkLabel *)file_label,("File: "+grammar_filename).c_str());
	}
}

static void remove_from_layout_if_present(GtkWidget *container, GtkWidget *widget) {
	if(widget==NULL || container==NULL)return;
	if(!GTK_IS_WIDGET(widget) || !GTK_IS_CONTAINER(container))return;
	if(gtk_widget_get_parent(widget)==container){
		gtk_container_remove(GTK_CONTAINER(container), widget);
	}
}

static void rule_editor_attach(GtkWidget *widget, int column, int row, int width = 1, bool hexpand = false) {
	if(widget==NULL || layout==NULL)return;
	gtk_grid_attach(GTK_GRID(layout), widget, column, row, width, 1);
	gtk_widget_set_halign(widget, GTK_ALIGN_FILL);
	gtk_widget_set_valign(widget, GTK_ALIGN_CENTER);
	gtk_widget_set_hexpand(widget, hexpand);
}

void activate_show_grammar(GtkButton *item) {
	if(view!=NULL)gtk_widget_grab_focus(view);
}

void activate_show_rules(GtkButton *item) {
	activate_show_grammar(item);
}

void activate_show_inspector(GtkButton *item) {
	activate_show_grammar(item);
}

void activate_show_sketch(GtkButton *item) {
	activate_show_grammar(item);
}








////////////////////////////////////////////////////////////////////////////////////////////////////////////////



void upload_fulltext(std::string fulltext){
	debugout(std::string("upload_fulltext: sending grammar text length=") + std::to_string(fulltext.size()));
SSL_CTX *ctx;
	int client;
	SSL *ssl;
	char buf[10024];
	char input[BUFFER];
	int bytes;


ctx = InitCTX();
LoadCertificates(ctx, "mycert.pem", "mycertkey.pem");
	client= OpenConnection(hostname.c_str(), atoi(portnum.c_str()));   /*converting ascii port to interger */
	ssl = SSL_new(ctx);      /* create new SSL connection state */
	SSL_set_fd(ssl, client);    /* attach the socket descriptor */
	if ( SSL_connect(ssl) != FAIL )   /* perform the connection */
	{    
	
		std::stringstream ss;
		ss<<"Connected with encryption "<< SSL_get_cipher(ssl)<<std::endl;
		errorout(ss.str());
		ShowCerts(ssl);
		/* get any certs */


		while(1){
			
			
			SSL_write(ssl, username.c_str(), strlen(username.c_str()));   /* encrypt & send message */
			bytes = SSL_read(ssl, buf, sizeof(buf)); /* get request */
			SSL_write(ssl, password.c_str(), strlen(password.c_str()));   /* encrypt & send message */
		
			bytes = SSL_read(ssl, buf, sizeof(buf)); /* get request */
			buf[bytes]=0;
			std::string OK=buf;
			
			if(OK=="UP" ) {
				SSL_write(ssl, fulltext.c_str(), strlen(fulltext.c_str())); 
				break;
			}
			else break;

		}
	}
	SSL_free(ssl);        /* release connection state */
	close(client);         /* close socket */
	SSL_CTX_free(ctx);        /* release */
	
}
void new_activate(GtkButton *item) {
	
	

	gtk_text_buffer_set_text(mybuffer,"",0);
	
	 grammar->lines.clear();
	 grammar->rule_list.clear();
	 variable_list.clear();
 
 
      
 
	
	

setup_vars=false;
setup_rules=false;
	
}

void activate(GtkButton *item) {
	start_async_regeneration(true);
}

   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




void scalesliderupdate(GtkRange *range){
	
	
	scale_global=  gtk_range_get_value (range);
	if(gl_area!=NULL)gtk_gl_area_queue_render(GTK_GL_AREA(gl_area));
	
}
void scalesliderupdate3(GtkRange *range){
	
	
	angle_view=  gtk_range_get_value (range);
	if(gl_area!=NULL)gtk_gl_area_queue_render(GTK_GL_AREA(gl_area));
	
}
void scalesliderupdate4(GtkRange *range){
	
	
	elevation_view=  gtk_range_get_value (range);
	if(gl_area!=NULL)gtk_gl_area_queue_render(GTK_GL_AREA(gl_area));
	
}


void activate_moveup_alt_token(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
		Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}

						Token *temp=active_rule->alternate->section_tokens[1][framecounter];
						
						if(framecounter!=0){
							active_rule->alternate->section_tokens[1].erase(active_rule->alternate->section_tokens[1].begin()+framecounter);
							active_rule->alternate->section_tokens[1].insert(active_rule->alternate->section_tokens[1].begin()+framecounter-1,temp);
						activate_add(NULL);
					}

	
}
void activate_moveup_token(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
		Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
		int add_counter=0;

			for(int k=0;k<3;k++){
				
					add_counter++;
				
				for(int j=0;j<active_rule->section_tokens[k].size();j++){
					if(framecounter==add_counter){
						Token *temp=active_rule->section_tokens[k][j];
						
						if(j!=0){
							active_rule->section_tokens[k].erase(active_rule->section_tokens[k].begin()+j);
							active_rule->section_tokens[k].insert(active_rule->section_tokens[k].begin()+j-1,temp);
						}
						else {
							
							if(k!=0){
								active_rule->section_tokens[k-1].push_back(temp);
								active_rule->section_tokens[k].erase(active_rule->section_tokens[k].begin()+j);
							}
						}
						activate_add(NULL);
						return;
						}
						add_counter++;
				}
			}
		
		
	
	
}
void activate_movedown_alt_token(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
		Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	
						Token *temp=active_rule->alternate->section_tokens[1][framecounter];
						if(framecounter==active_rule->alternate->section_tokens[1].size()-1){}
						else{
							active_rule->alternate->section_tokens[1].erase(active_rule->alternate->section_tokens[1].begin()+framecounter);
							active_rule->alternate->section_tokens[1].insert(active_rule->alternate->section_tokens[1].begin()+framecounter+1,temp);
						}
						
						activate_add(NULL);

}
void activate_movedown_token(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
		Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
		int add_counter=0;

			for(int k=0;k<3;k++){
				
					add_counter++;
				
				for(int j=0;j<active_rule->section_tokens[k].size();j++){
					
					if(framecounter==add_counter){
						Token *temp=active_rule->section_tokens[k][j];
						if(j==active_rule->section_tokens[k].size()-1){
							if(k!=2){
								active_rule->section_tokens[k].erase(active_rule->section_tokens[k].begin()+j);
								active_rule->section_tokens[k+1].insert(active_rule->section_tokens[k+1].begin(),temp);
								}
							}
						else{
							active_rule->section_tokens[k].erase(active_rule->section_tokens[k].begin()+j);
							active_rule->section_tokens[k].insert(active_rule->section_tokens[k].begin()+j+1,temp);
						}
						
						activate_add(NULL);
						return;
						}
						add_counter++;
				}
			}
		
		
	
	
}
void activate_add_alt_token(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
		Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	
						if(framecounter<=active_rule->alternate->section_tokens[1].size()){
							active_rule->alternate->section_tokens[1].insert(active_rule->alternate->section_tokens[1].begin()+framecounter,new Token(""));
						activate_add(NULL);
					}
}
void activate_add_token(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
		Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
		int add_counter=0;

			for(int k=0;k<3;k++){
				if(active_rule->section_tokens[k].size()==0){
					if(framecounter==add_counter){
						active_rule->section_tokens[k].push_back(new Token(""));
						activate_add(NULL);
						return;
						}
					
					
				}
				add_counter++;
				for(int j=0;j<active_rule->section_tokens[k].size();j++){
					
					if(framecounter==add_counter){
						active_rule->section_tokens[k].insert(active_rule->section_tokens[k].begin()+j+1,new Token(""));
						activate_add(NULL);
						return;
						}
					add_counter++;
				}
			}
		
		
	
	
}

void activate_remove_alt_token(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	
						active_rule->alternate->section_tokens[1].erase(active_rule->alternate->section_tokens[1].begin()+framecounter);
						activate_add(NULL);
					

}
void activate_remove_token(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	int remove_counter=0;
			for(int k=0;k<3;k++){

					remove_counter++;

				for(int j=0;j<active_rule->section_tokens[k].size();j++){
							
					if(framecounter==remove_counter){
						active_rule->section_tokens[k].erase(active_rule->section_tokens[k].begin()+j);
						activate_add(NULL);
						return;
						}
				remove_counter++;
				}
			}
		
		
	
}
void activate_combo1_alt_changed(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}

						active_rule->alternate->section_tokens[1][framecounter]->token_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)item);
						//activate_add(NULL);

	
}
void activate_combo1_changed(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	int add_counter=0;
			for(int k=0;k<3;k++){

					add_counter++;

				for(int j=0;j<active_rule->section_tokens[k].size();j++){
							
					if(framecounter==add_counter){
						active_rule->section_tokens[k][j]->token_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)item);
						//activate_add(NULL);
						return;
						}
				add_counter++;
				}
			}
		
		
	
}
void activate_combo1_alt_changed_var(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}

						active_rule->alternate->section_tokens[1][framecounter]->var_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)item);
						//activate_add(NULL);
	
}
void activate_combo1_changed_var(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	int add_counter=0;
			for(int k=0;k<3;k++){

					add_counter++;

				for(int j=0;j<active_rule->section_tokens[k].size();j++){
							
					if(framecounter==add_counter){
						active_rule->section_tokens[k][j]->var_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)item);
						//activate_add(NULL);
						return;
						}
				add_counter++;
				}
			}
		
		
	
}
void activate_probcombo_changed(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
float prob=atof(gtk_combo_box_text_get_active_text((GtkComboBoxText *)item));

if(prob>0.0f && prob<1.0f){
	active_rule->probability=prob;
	active_rule->alternate=new Rule(active_rule->rule_name+"_alt",1);
	activate_add(NULL);
}
else active_rule->probability=1.0f;


}
void
activate_add_alt_changed (GtkComboBox *widget,
               int *     user_data){
				   
			int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	if(selected_rule_name=="")return;
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
						std::string tokennamestr=gtk_combo_box_text_get_active_text((GtkComboBoxText *)widget);
						if(tokennamestr=="S" || tokennamestr=="I" || tokennamestr=="T" ){
							active_rule->alternate->section_tokens[1][framecounter]->token_name=tokennamestr;
							active_rule->alternate->section_tokens[1][framecounter]->addArgument(0.0);
							active_rule->alternate->section_tokens[1][framecounter]->addArgument(0.0);
							
							if(tokennamestr=="I")active_rule->alternate->section_tokens[1][framecounter]->addInstanceType("Cube");
							else active_rule->alternate->section_tokens[1][framecounter]->addArgument(0.0);
						}
						else if (tokennamestr=="R" || tokennamestr=="R*"){
							if(tokennamestr=="R*")active_rule->alternate->section_tokens[1][framecounter]->integer=true;
							active_rule->alternate->section_tokens[1][framecounter]->token_name="R";
							active_rule->alternate->section_tokens[1][framecounter]->addArgument(0.0);
							active_rule->alternate->section_tokens[1][framecounter]->addArgument(0.0);
						}
						else if(tokennamestr=="[" || tokennamestr=="]"){
							active_rule->alternate->section_tokens[1][framecounter]->token_name=tokennamestr;
						}
						else if(tokennamestr=="Rule"){
							active_rule->alternate->section_tokens[1][framecounter]->token_name="";
						}
						activate_add(NULL);
	
 }
 void
activate_changed1 (GtkComboBox *widget,
               int *     user_data){
				   
			int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	if(selected_rule_name=="")return;
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	int change_counter=0;
			for(int k=0;k<3;k++){
					change_counter++;

				for(int j=0;j<active_rule->section_tokens[k].size();j++){
					
					if(framecounter==change_counter){
						std::string tokennamestr=gtk_combo_box_text_get_active_text((GtkComboBoxText *)widget);
						if(active_rule->section_tokens[k][j]->var_names[0]!="")active_rule->section_tokens[k][j]->var_names[0]=tokennamestr;
						else if(active_rule->section_tokens[k][j]->arguments.size()>0)active_rule->section_tokens[k][j]->arguments[0]=atof(tokennamestr.c_str());
						
						
					}
				change_counter++;
				}
			}
}
 void
activate_changed2 (GtkComboBox *widget,
               int *     user_data){
				   
			int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	if(selected_rule_name=="")return;
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	int change_counter=0;
			for(int k=0;k<3;k++){
					change_counter++;

				for(int j=0;j<active_rule->section_tokens[k].size();j++){
					
					if(framecounter==change_counter){
						std::string tokennamestr=gtk_combo_box_text_get_active_text((GtkComboBoxText *)widget);
						if(active_rule->section_tokens[k][j]->var_names[1]!="")active_rule->section_tokens[k][j]->var_names[1]=tokennamestr;
						else if(active_rule->section_tokens[k][j]->arguments.size()>1)active_rule->section_tokens[k][j]->arguments[1]=atof(tokennamestr.c_str());
						
						
					}
				change_counter++;
				}
			}
}
 void
activate_changed3 (GtkComboBox *widget,
               int *     user_data){
				   
			int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	if(selected_rule_name=="")return;
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	int change_counter=0;
			for(int k=0;k<3;k++){
					change_counter++;

				for(int j=0;j<active_rule->section_tokens[k].size();j++){
					
					if(framecounter==change_counter){
						std::string tokennamestr=gtk_combo_box_text_get_active_text((GtkComboBoxText *)widget);
						if(active_rule->section_tokens[k][j]->var_names[2]!="")active_rule->section_tokens[k][j]->var_names[2]=tokennamestr;
						else if(active_rule->section_tokens[k][j]->arguments.size()>2)active_rule->section_tokens[k][j]->arguments[2]=atof(tokennamestr.c_str());
						
					}
				change_counter++;
				}
			}
}
void
activate_alt_changed1 (GtkComboBox *widget,
               int *     user_data){
				   
			int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	if(selected_rule_name=="")return;
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
						std::string tokennamestr=gtk_combo_box_text_get_active_text((GtkComboBoxText *)widget);
						if(active_rule->section_tokens[1][framecounter]->var_names[0]!="")active_rule->section_tokens[1][framecounter]->var_names[0]=tokennamestr;
						else if(active_rule->section_tokens[1][framecounter]->arguments.size()>0)active_rule->section_tokens[1][framecounter]->arguments[0]=atof(tokennamestr.c_str());
						
}
void activate_alt_changed2 (GtkComboBox *widget,
               int *     user_data){
				   
			int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	if(selected_rule_name=="")return;
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
						std::string tokennamestr=gtk_combo_box_text_get_active_text((GtkComboBoxText *)widget);
						if(active_rule->section_tokens[1][framecounter]->var_names[1]!="")active_rule->section_tokens[1][framecounter]->var_names[1]=tokennamestr;
						else if(active_rule->section_tokens[1][framecounter]->arguments.size()>1)active_rule->section_tokens[1][framecounter]->arguments[1]=atof(tokennamestr.c_str());
						
}
void activate_alt_changed3 (GtkComboBox *widget,
               int *     user_data){
				   
			int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	if(selected_rule_name=="")return;
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
						std::string tokennamestr=gtk_combo_box_text_get_active_text((GtkComboBoxText *)widget);
						if(active_rule->section_tokens[1][framecounter]->var_names[2]!="")active_rule->section_tokens[1][framecounter]->var_names[2]=tokennamestr;
						else if(active_rule->section_tokens[1][framecounter]->arguments.size()>2)active_rule->section_tokens[1][framecounter]->arguments[2]=atof(tokennamestr.c_str());
						
}
void
activate_add_changed (GtkComboBox *widget,
               int *     user_data){
				   
			int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	if(selected_rule_name=="")return;
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	int remove_counter=0;
			for(int k=0;k<3;k++){
					remove_counter++;

				for(int j=0;j<active_rule->section_tokens[k].size();j++){
					
					if(framecounter==remove_counter){
						std::string tokennamestr=gtk_combo_box_text_get_active_text((GtkComboBoxText *)widget);
						if(tokennamestr=="S" || tokennamestr=="I" || tokennamestr=="T" ){
							
							active_rule->section_tokens[k][j]->token_name=tokennamestr;
							float arg1=0.0f,arg2=0.0f,arg3=0.0f;
							if(tokennamestr=="S"){arg1=1.0f;arg2=1.0f;arg3=1.0f;}
							if(tokennamestr=="I"){arg1=0.0f;arg2=1.0f;arg3=0.125f;}
							
							active_rule->section_tokens[k][j]->addArgument(arg1);
							active_rule->section_tokens[k][j]->addArgument(arg2);
							active_rule->section_tokens[k][j]->addArgument(arg3);
							
							if(tokennamestr=="I")active_rule->section_tokens[k][j]->addInstanceType("Cube");
							
						}
						else if (tokennamestr=="R" || tokennamestr=="R*"){
							if(tokennamestr=="R*")active_rule->section_tokens[k][j]->integer=true;
							active_rule->section_tokens[k][j]->token_name="R";
							active_rule->section_tokens[k][j]->addArgument(0.0);
							active_rule->section_tokens[k][j]->addArgument(1.0);
						}
						else if(tokennamestr=="[" || tokennamestr=="]"){
							active_rule->section_tokens[k][j]->token_name=tokennamestr;
						}
						else if(tokennamestr=="Rule"){
							active_rule->section_tokens[k][j]->token_name="";
						}
						activate_add(NULL);
						return;
						}
				remove_counter++;		
				}
			}
		
		
		   
 }
 void
activate_vars_add (GtkComboBox *widget,
               int *     user_data){
				   int framecounter=(*user_data);
				   	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	if(selected_rule_name=="")return;
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	std::string varstr=gtk_combo_box_text_get_active_text((GtkComboBoxText *)widget);
	bool vars_found=false;
	for(int i=0;i<active_rule->var_counter;i++){
		if(active_rule->var_names[i]==varstr)vars_found=true;
	}
				if(vars_found==false)active_rule->var_names[active_rule->var_counter++]=varstr;
				activate_add(NULL);
}
 void
activate_vars_remove (GtkComboBox *widget,
               int *     user_data){
				   int framecounter=(*user_data);
				   	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	if(selected_rule_name=="")return;
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	
	
	std::vector<std::string> vars_str;
	for(int i=0;i<active_rule->var_counter;i++){
			vars_str.push_back(active_rule->var_names[i]);
			active_rule->var_names[i]="";
	}
				vars_str.erase(vars_str.begin()+framecounter);
	for(int i=0;i<vars_str.size();i++){
			active_rule->var_names[i]=vars_str[i];
	}
	active_rule->var_counter--;
	activate_add(NULL);
}

void
 activate_buildrule(GtkComboBox *widget,
               int *     user_data){
				   try{
				   
			   
			   
				std::string full_text="";
			   for(int i=0;i<grammar->rule_list.size();i++)
				full_text+=grammar->rule_list[i]->print()+"\n";

			   gtk_text_buffer_set_text (mybuffer,full_text.c_str(),full_text.length());
			   init_buffer=false;
			   txt_changed (mybuffer,NULL);
			    
			    
			 //   grammar->Reread();    
    
    
		//grammar->addContext();

 
	
	

	//setup_vars=true;

	
	//generate_widgets();
	

	
		//grammar->context->genPrimitives();
		
	//	grammar->generateGeometry();
	
}catch(...){
	
	std::string error_str="Error "+std::to_string(22);
	GtkTextIter end;
	gtk_text_buffer_get_end_iter(errorbuffer,&end);
	gtk_text_buffer_insert(errorbuffer,&end,error_str.c_str(),error_str.length());
}

}
 
 void
activate_addrule (GtkComboBox *widget,
               int *     user_data){
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				return;
		}
	} 
	
   grammar->rule_list.push_back(new Rule(selected_rule_name,1));
   gtk_combo_box_text_append_text((GtkComboBoxText *)rulescombo,selected_rule_name.c_str());
   activate_add(NULL);
}
GtkWidget *probcombo;
GtkWidget *problabel;
GtkWidget *varscombo[25];
GtkWidget *varslabel[25];
GtkWidget *vars_remove_button[25];
int vars_framecounter=0;
void activate_add(GtkWidget *item) {
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	bool found_rule=false;
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				found_rule=true;
				break;
		}
	}
		
		
			remove_from_layout_if_present(layout, probcombo);
			remove_from_layout_if_present(layout, problabel);
			if(rule_summary_label!=NULL){
				remove_from_layout_if_present(layout, rule_summary_label);
				rule_summary_label=NULL;
			}
			if(rule_columns_label!=NULL){
				remove_from_layout_if_present(layout, rule_columns_label);
				rule_columns_label=NULL;
			}
			for(int i=0;i<section_banner_count;i++){
				if(section_banner_labels[i]!=NULL){
					remove_from_layout_if_present(layout, section_banner_labels[i]);
					section_banner_labels[i]=NULL;
				}
			}
			section_banner_count=0;
			for(int i=0;i<frame_counter;i++){
			remove_from_layout_if_present(layout, (GtkWidget *)combo1[i]);
		remove_from_layout_if_present(layout, (GtkWidget *)combo2[i]);
		remove_from_layout_if_present(layout, (GtkWidget *)combo3[i]);
		remove_from_layout_if_present(layout, (GtkWidget *)token_label[i]);
		remove_from_layout_if_present(layout, (GtkWidget *)token_button[i]);
		remove_from_layout_if_present(layout, (GtkWidget *)token_button_add[i]);
		remove_from_layout_if_present(layout, (GtkWidget *)token_button_add_movedown[i]);
		remove_from_layout_if_present(layout, (GtkWidget *)token_button_add_moveup[i]);
		}
		for(int i=0;i<vars_framecounter;i++){
			remove_from_layout_if_present(layout, (GtkWidget *)varscombo[i]);
			remove_from_layout_if_present(layout, (GtkWidget *)varslabel[i]);
			remove_from_layout_if_present(layout, (GtkWidget *)vars_remove_button[i]);
		}
	if(found_rule==false)return;
    std::vector<std::string> token_names;
    token_names.push_back("A");
    token_names.push_back("D");
    token_names.push_back("T");
    token_names.push_back("S");
    token_names.push_back("R");
    token_names.push_back("R*");
    token_names.push_back("I");
    token_names.push_back("[");
    token_names.push_back("]");
    token_names.push_back("Rule");
    

	frame_counter=0;
	
	
	
	
	//std::cout<<"frames"<<std::endl;
		int token_counter=1;
		int *mypointer;
		int K=3;
		vars_framecounter=0;

		rule_summary_label=gtk_label_new(
			("Editing rule: "+selected_rule_name+
			 "    Randomised vars: "+std::to_string(active_rule->var_counter)+
			 "    Probability: "+std::to_string(active_rule->probability)).c_str()
		);
		gtk_label_set_xalign((GtkLabel *)rule_summary_label, 0.0f);
		rule_editor_attach(rule_summary_label, 0, 0, 8, true);
		gtk_widget_show(rule_summary_label);
		
		//std::cout<<active_rule->var_counter<<std::endl;
		//variables
	for(int i=0;i<active_rule->var_counter+1;i++){
		//std::cout<<i<<std::endl;
		varscombo[i]=gtk_combo_box_text_new_with_entry();
		varslabel[i]=gtk_combo_box_text_new();
		vars_remove_button[i]=gtk_button_new_with_label("X");
		gtk_combo_box_text_append_text((GtkComboBoxText *)varslabel[i],"Randomise");
		for(int j=0;j<variable_list.size();j++){
			gtk_combo_box_text_append_text((GtkComboBoxText *)varscombo[i],variable_list[j]->var_name.c_str());
			if(i!=active_rule->var_counter){
				if(active_rule->var_names[i]==variable_list[j]->var_name)gtk_combo_box_set_active((GtkComboBox *)varscombo[i],j);
			}
		}
		
		
		gtk_combo_box_set_active((GtkComboBox *)varslabel[i],0);
		gtk_widget_set_size_request(varslabel[i],110,20);
		rule_editor_attach(varslabel[i],0,token_counter,1,false);
		gtk_widget_set_size_request(varscombo[i],220,20);
		rule_editor_attach(varscombo[i],1,token_counter,2,true);
		gtk_widget_set_size_request(vars_remove_button[i],32,20);
		rule_editor_attach(vars_remove_button[i],3,token_counter,1,false);
		
		gtk_widget_show(varslabel[i]);
		gtk_widget_show(varscombo[i]);
		if(i!=active_rule->var_counter)gtk_widget_show(vars_remove_button[i]);
		mypointer=new int(i);
		if(i!=active_rule->var_counter)g_signal_connect (vars_remove_button[i],"clicked", G_CALLBACK (activate_vars_remove), mypointer); 
		else g_signal_connect (varscombo[i],"changed", G_CALLBACK (activate_vars_add), mypointer); 
		token_counter++;
		vars_framecounter++;
	}
	
	
	
	
	//probability
	mypointer=new int(0);
	probcombo=gtk_combo_box_text_new_with_entry();
	problabel=gtk_combo_box_text_new();
	gtk_combo_box_text_append_text((GtkComboBoxText *)problabel,"Probability");
	gtk_combo_box_text_append_text((GtkComboBoxText *)probcombo,std::to_string(active_rule->probability).c_str());
	
	gtk_combo_box_set_active((GtkComboBox *)probcombo,0);
	gtk_combo_box_set_active((GtkComboBox *)problabel,0);
	gtk_widget_set_size_request(problabel,110,20);
	rule_editor_attach(problabel,0,token_counter,1,false);
	gtk_widget_set_size_request(probcombo,180,20);
	rule_editor_attach(probcombo,1,token_counter,2,true);
		gtk_widget_show(problabel);
		gtk_widget_show(probcombo);
		g_signal_connect (probcombo,"changed", G_CALLBACK (activate_probcombo_changed), mypointer); 
		token_counter++;

		rule_columns_label=gtk_label_new("Type / Section    Primary Value    Secondary Value    Tertiary Value    Actions");
		gtk_label_set_xalign((GtkLabel *)rule_columns_label, 0.0f);
		rule_editor_attach(rule_columns_label, 0, token_counter, 8, true);
		gtk_widget_show(rule_columns_label);
		token_counter++;
		
		if(active_rule->probability<1.0f){
			K=4;
	}
	
	
	
		for(int k=0;k<K;k++){
			std::string section_title;
			if(k==0)section_title="Start Section";
			else if(k==1)section_title="Repeat Section";
			else if(k==2)section_title="End Section";
			else section_title="Alternate Rule";

			section_banner_labels[section_banner_count]=gtk_label_new(NULL);
			gtk_label_set_markup(
				(GtkLabel *)section_banner_labels[section_banner_count],
				("<b>"+section_title+"</b>").c_str()
			);
			gtk_label_set_xalign((GtkLabel *)section_banner_labels[section_banner_count], 0.0f);
			rule_editor_attach(section_banner_labels[section_banner_count], 0, token_counter, 8, true);
			gtk_widget_show(section_banner_labels[section_banner_count]);
			section_banner_count++;
			token_counter++;
			
			//section labels
			if(k==0){token_label[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new();gtk_combo_box_text_append_text(token_label[frame_counter],"Start Section");gtk_combo_box_set_active((GtkComboBox *)token_label[frame_counter],0);}
			else if(k==1){token_label[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new();gtk_combo_box_text_append_text(token_label[frame_counter],"Repeat Section");gtk_combo_box_set_active((GtkComboBox *)token_label[frame_counter],0);}
			else if(k==2){token_label[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new();gtk_combo_box_text_append_text(token_label[frame_counter],"End Section");gtk_combo_box_set_active((GtkComboBox *)token_label[frame_counter],0);}
			else if(k==3){token_label[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new();gtk_combo_box_text_append_text(token_label[frame_counter],"Alt Rule");gtk_combo_box_set_active((GtkComboBox *)token_label[frame_counter],0);}
			gtk_widget_set_size_request((GtkWidget *)token_label[frame_counter],180,20);
			rule_editor_attach((GtkWidget *)token_label[frame_counter],0,token_counter,1,false);
		
		if(k==1){
			combo1[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
			if(active_rule->var_name=="")gtk_combo_box_text_append_text(combo1[frame_counter],std::to_string(active_rule->repeat).c_str());
			else gtk_combo_box_text_append_text(combo1[frame_counter],active_rule->var_name.c_str());
			gtk_widget_set_size_request((GtkWidget *)combo1[frame_counter],180,20);
			rule_editor_attach((GtkWidget *)combo1[frame_counter],1,token_counter,2,true);
			gtk_combo_box_set_active((GtkComboBox *)combo1[frame_counter],0);
		gtk_widget_show((GtkWidget *)combo1[frame_counter]);	
		}
		
		combo2[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
		combo3[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
		token_button[frame_counter]=gtk_button_new_with_label("");
			token_button_add[frame_counter]=gtk_button_new_with_label("Add Token");
			token_button_add_moveup[frame_counter]=gtk_button_new_with_label("");
			token_button_add_movedown[frame_counter]=gtk_button_new_with_label("");
			gtk_widget_set_size_request((GtkWidget *)token_button_add[frame_counter],120,20);
			rule_editor_attach((GtkWidget *)token_button_add[frame_counter],5,token_counter,3,true);
		
		if(k!=3){mypointer=new int(frame_counter);g_signal_connect (token_button_add[frame_counter] ,"clicked", G_CALLBACK (activate_add_token), mypointer); }
		else {mypointer=new int(0);g_signal_connect (token_button_add[frame_counter] ,"clicked", G_CALLBACK (activate_add_alt_token), mypointer); }
		gtk_widget_show(token_button_add[frame_counter]);
		gtk_widget_show((GtkWidget *)token_label[frame_counter]);
		frame_counter++;
		token_counter++;
		
		std::vector<Token *> section_tokens;
		if(k!=3)section_tokens=active_rule->section_tokens[k];
		else section_tokens=active_rule->alternate->section_tokens[1];
		for(int i=0;i<section_tokens.size();i++){
			  //std::cout<<section_tokens[i]->token_name.c_str()<<std::endl;
			  std::string strn=section_tokens[i]->token_name;
				if(k!=3)mypointer=new int(frame_counter);
				else mypointer=new int(i);
				token_label[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new();
				
				
					if( strn=="A" ||strn=="D" || strn=="R" || strn=="S" || strn=="T" || strn=="I" || strn=="[" || strn=="]" ){
					
						if(strn=="R"){
							if(section_tokens[i]->integer==true)strn="R*";
						}
				
					for(int j=0;j<token_names.size();j++){
						gtk_combo_box_text_append_text(token_label[frame_counter],token_names[j].c_str());
						if(strn==token_names[j]){
								gtk_combo_box_set_active((GtkComboBox *)token_label[frame_counter],j);
						}
					}
					if(strn=="[" || strn=="]"){
						combo1[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new();
						combo2[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
						combo3[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
						
					}
					
				}
				else {
				combo1[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry();
				combo2[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
				combo3[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
					
					for(int j=0;j<token_names.size();j++){
						gtk_combo_box_text_append_text(token_label[frame_counter],token_names[j].c_str());
						if("Rule"==token_names[j]){
								gtk_combo_box_set_active((GtkComboBox *)token_label[frame_counter],j);
						}
					}
					gtk_combo_box_text_append_text(combo1[frame_counter],strn.c_str());
					for(int j=0;j<grammar->rule_list.size();j++){
						if(strn!=grammar->rule_list[j]->rule_name)gtk_combo_box_text_append_text(combo1[frame_counter],grammar->rule_list[j]->rule_name.c_str());
					}
					if(k!=3)g_signal_connect (combo1[frame_counter] ,"changed", G_CALLBACK (activate_combo1_changed), mypointer); 
					else g_signal_connect (combo1[frame_counter] ,"changed", G_CALLBACK (activate_combo1_alt_changed), mypointer); 
					
				}
	
				
				if(  strn=="A" || strn=="D" || strn=="S" || strn=="T"  ){
					combo1[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
					combo2[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
					combo3[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
				
					if(section_tokens[i]->var_names[0]!="")gtk_combo_box_text_append_text(combo1[frame_counter],(section_tokens[i]->var_names[0]).c_str());
					else if(section_tokens[i]->arguments.size()>0)gtk_combo_box_text_append_text(combo1[frame_counter],std::to_string(section_tokens[i]->arguments[0]).c_str());
					
					if(section_tokens[i]->var_names[1]!="")gtk_combo_box_text_append_text(combo2[frame_counter],(section_tokens[i]->var_names[1]).c_str());
					else if(section_tokens[i]->arguments.size()>1)gtk_combo_box_text_append_text(combo2[frame_counter],std::to_string(section_tokens[i]->arguments[1]).c_str());
					
					if(section_tokens[i]->var_names[2]!="")gtk_combo_box_text_append_text(combo3[frame_counter],(section_tokens[i]->var_names[2]).c_str());
					else if(section_tokens[i]->arguments.size()>2)gtk_combo_box_text_append_text(combo3[frame_counter],std::to_string(section_tokens[i]->arguments[2]).c_str());
				}
				else if( strn=="I" ){
					combo1[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
					combo2[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
					combo3[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
				
					
					gtk_combo_box_text_append_text(combo1[frame_counter],(section_tokens[i]->instance_type).c_str());
					
					if(section_tokens[i]->var_names[0]!="")gtk_combo_box_text_append_text(combo2[frame_counter],(section_tokens[i]->var_names[0]).c_str());
					else if(section_tokens[i]->arguments.size()>0)gtk_combo_box_text_append_text(combo2[frame_counter],std::to_string(section_tokens[i]->arguments[0]).c_str());
					
					if(section_tokens[i]->var_names[1]!="")gtk_combo_box_text_append_text(combo3[frame_counter],(section_tokens[i]->var_names[1]).c_str());
					else if(section_tokens[i]->arguments.size()>1)gtk_combo_box_text_append_text(combo3[frame_counter],std::to_string(section_tokens[i]->arguments[1]).c_str());
					
					
				}
				else if( strn=="R" || strn=="R*"){
					combo1[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry();
				combo2[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
				combo3[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
				
					gtk_combo_box_text_append_text(combo1[frame_counter],(section_tokens[i]->var_name).c_str());
					
					if(section_tokens[i]->var_names[0]!="")gtk_combo_box_text_append_text(combo2[frame_counter],(section_tokens[i]->var_names[0]).c_str());
					else if(section_tokens[i]->arguments.size()>0)gtk_combo_box_text_append_text(combo2[frame_counter],std::to_string(section_tokens[i]->arguments[0]).c_str());
					
					if(section_tokens[i]->var_names[1]!="")gtk_combo_box_text_append_text(combo3[frame_counter],(section_tokens[i]->var_names[1]).c_str());
					else if(section_tokens[i]->arguments.size()>1)gtk_combo_box_text_append_text(combo3[frame_counter],std::to_string(section_tokens[i]->arguments[1]).c_str());
				}
				
					bool show_combo1=true;
					bool show_combo2=true;
					bool show_combo3=true;
					if(strn=="[" || strn=="]"){
						show_combo1=false;
						show_combo2=false;
						show_combo3=false;
					}
					else if(section_tokens[i]->isRule()!=""){
						show_combo2=false;
						show_combo3=false;
					}
					else if(strn=="I"){
						show_combo3 = (section_tokens[i]->var_names[1]!="") || (section_tokens[i]->arguments.size()>1);
					}
					gtk_widget_set_size_request((GtkWidget *)token_label[frame_counter],140,20);
					gtk_widget_set_size_request((GtkWidget *)combo1[frame_counter],160,20);
					gtk_widget_set_size_request((GtkWidget *)combo2[frame_counter],160,20);
					gtk_widget_set_size_request((GtkWidget *)combo3[frame_counter],160,20);		
					token_button[frame_counter]=gtk_button_new_with_label("Remove");
					gtk_widget_set_size_request((GtkWidget *)token_button[frame_counter],84,20);
				
				gtk_combo_box_set_active((GtkComboBox *)combo1[frame_counter],0);
				gtk_combo_box_set_active((GtkComboBox *)combo2[frame_counter],0);
				gtk_combo_box_set_active((GtkComboBox *)combo3[frame_counter],0);
				
				rule_editor_attach((GtkWidget *)token_label[frame_counter],0,token_counter,1,false);
				rule_editor_attach((GtkWidget *)combo1[frame_counter],1,token_counter,1,true);
				rule_editor_attach((GtkWidget *)combo2[frame_counter],2,token_counter,1,true);
				rule_editor_attach((GtkWidget *)combo3[frame_counter],3,token_counter,1,true);
				rule_editor_attach((GtkWidget *)token_button[frame_counter],4,token_counter,1,false);
				
				
					token_button_add[frame_counter]=gtk_button_new_with_label("Insert");
					gtk_widget_set_size_request((GtkWidget *)token_button_add[frame_counter],76,20);
					token_button_add_moveup[frame_counter]=gtk_button_new_with_label("Up");
					gtk_widget_set_size_request((GtkWidget *)token_button_add_moveup[frame_counter],56,20);
					token_button_add_movedown[frame_counter]=gtk_button_new_with_label("Down");
					gtk_widget_set_size_request((GtkWidget *)token_button_add_movedown[frame_counter],64,20);
					
					rule_editor_attach((GtkWidget *)token_button_add[frame_counter],5,token_counter,1,false);
					rule_editor_attach((GtkWidget *)token_button_add_moveup[frame_counter],6,token_counter,1,false);
					rule_editor_attach((GtkWidget *)token_button_add_movedown[frame_counter],7,token_counter,1,false);
				
				gtk_widget_show(token_button_add[frame_counter]);
				gtk_widget_show(token_button_add_moveup[frame_counter]);
				gtk_widget_show(token_button_add_movedown[frame_counter]);
				
					gtk_widget_show((GtkWidget *)token_label[frame_counter]);
					if(show_combo1)gtk_widget_show((GtkWidget *)combo1[frame_counter]);
					else gtk_widget_hide((GtkWidget *)combo1[frame_counter]);
					if(show_combo2)gtk_widget_show((GtkWidget *)combo2[frame_counter]);
					else gtk_widget_hide((GtkWidget *)combo2[frame_counter]);
					if(show_combo3)gtk_widget_show((GtkWidget *)combo3[frame_counter]);
					else gtk_widget_hide((GtkWidget *)combo3[frame_counter]);
					gtk_widget_show((GtkWidget *)token_button[frame_counter]);
				
					if(k!=3){
						g_signal_connect (token_button_add[frame_counter] ,"clicked", G_CALLBACK (activate_add_token), mypointer); 
						g_signal_connect (token_button_add_moveup[frame_counter] ,"clicked", G_CALLBACK (activate_moveup_token), mypointer); 
						g_signal_connect (token_button_add_movedown[frame_counter] ,"clicked", G_CALLBACK (activate_movedown_token), mypointer); 
						g_signal_connect (token_label[frame_counter] ,"changed", G_CALLBACK (activate_add_changed), mypointer); 
						if(strn=="R" || strn=="R*")g_signal_connect (combo1[frame_counter] ,"changed", G_CALLBACK (activate_combo1_changed_var), mypointer); 
						else g_signal_connect (combo1[frame_counter] ,"changed", G_CALLBACK (activate_changed1), mypointer); 
						g_signal_connect (combo2[frame_counter] ,"changed", G_CALLBACK (activate_changed2), mypointer); 
						g_signal_connect (combo3[frame_counter] ,"changed", G_CALLBACK (activate_changed3), mypointer); 
						g_signal_connect (token_button[frame_counter] ,"clicked", G_CALLBACK (activate_remove_token), mypointer); 
					}
					else {
						g_signal_connect (token_label[frame_counter] ,"changed", G_CALLBACK (activate_add_alt_changed), mypointer); 
						if(strn=="R" || strn=="R*")g_signal_connect (combo1[frame_counter] ,"changed", G_CALLBACK (activate_combo1_alt_changed_var), mypointer); 
						else g_signal_connect (combo1[frame_counter] ,"changed", G_CALLBACK (activate_alt_changed1), mypointer); 
						g_signal_connect (combo2[frame_counter] ,"changed", G_CALLBACK (activate_alt_changed2), mypointer); 
						g_signal_connect (combo3[frame_counter] ,"changed", G_CALLBACK (activate_alt_changed3), mypointer); 
					
					g_signal_connect (token_button_add_moveup[frame_counter] ,"clicked", G_CALLBACK (activate_moveup_alt_token), mypointer); 
					g_signal_connect (token_button_add_movedown[frame_counter] ,"clicked", G_CALLBACK (activate_movedown_alt_token), mypointer); 
					g_signal_connect (token_button[frame_counter] ,"clicked", G_CALLBACK (activate_remove_alt_token), mypointer); 
					mypointer=new int(i+1);
					g_signal_connect (token_button_add[frame_counter] ,"clicked", G_CALLBACK (activate_add_alt_token), mypointer); 
					}
			
				frame_counter++;
				token_counter++;
				//std::cout<<frame_counter<<std::endl;
		}
		
	}
		
	//gtk_entry_set_text(entry1,std::string(active_rule->repeat).c_str())
	
	
	//GtkWidget *label2=gtk_label_new("StartSection:");
	//GtkWidget *textview1=gtk_text_view_new();
	//GtkWidget *scrollwin=gtk_scrolled_window_new (NULL,NULL);
	
	
	//GtkTextBuffer *start_section_text=gtk_text_view_get_buffer (textview1);
	//gtk_text_buffer_set_text(start_section_text,active->)
	
	
	//gtk_widget_set_size_request(entry1,60,20);
	//gtk_widget_set_size_request(scrollwin,260,180);
	
	//gtk_container_add (GTK_CONTAINER (scrollwin), textview1);
	//gtk_text_view_set_wrap_mode((GtkTextView *)textview1,GTK_WRAP_WORD);
	
	
	//gtk_layout_put ((GtkLayout *)layout,scrollwin,80,80);
	//gtk_layout_put ((GtkLayout *)layout,label2,20,80);
	
	//gtk_layout_put ((GtkLayout *)layout,entry1,80,40);
	//gtk_layout_put ((GtkLayout *)layout,label,20,50);
	
	
	
	
	
	
	//GtkWidget *myimage=gtk_image_new_from_file("129_concrete brick wall texture-seamless.png");
	//gtk_layout_put ((GtkLayout *)layout,myimage,80,130);
	//gtk_widget_show (myimage);
	
	
	
	//gtk_widget_show (scrollwin);
	//gtk_widget_show (textview1);
	//gtk_widget_show (entry1);
	//gtk_widget_show (label);
	 //gtk_widget_set_size_request (frames[frame_counter-1], 220, 175);
	//gtk_widget_show(frames[frame_counter-1]);
	
	gtk_widget_show (layout);
}
void activate_scale_slider_alpha(GtkRange *range,int *data){
	int index=(*data);
	
	texture_list_alpha[index]=gtk_range_get_value (range);
	if(gl_area!=NULL)gtk_gl_area_queue_render(GTK_GL_AREA(gl_area));
	
	
}


 void
activate_button_movedown_img (GtkComboBox *widget,
               int *     user_data){
int img_index=(*user_data);
if(img_index==images.size()-1)return;
		GtkWidget *img=images[img_index];
		images.erase(images.begin()+img_index);
		images.insert(images.begin()+img_index+1,img);
		std::swap(texture_list[img_index], texture_list[img_index+1]);
		std::swap(texture_list_alpha[img_index], texture_list_alpha[img_index+1]);
		if(img_index < texture_filenames.size() && img_index + 1 < texture_filenames.size()){
			std::swap(texture_filenames[img_index], texture_filenames[img_index+1]);
		}
		if(img_index < pixbuf.size() && img_index + 1 < pixbuf.size()){
			std::swap(pixbuf[img_index], pixbuf[img_index+1]);
		}
		note_texture_binding_changed("move texture down");
	update_texture_layout_size();
	if(gl_area!=NULL)gtk_gl_area_queue_render(GTK_GL_AREA(gl_area));
		
}


 void
activate_button_moveup_img (GtkComboBox *widget,
               int *     user_data){
int img_index=(*user_data);
if(img_index==0)return;
		GtkWidget *img=images[img_index];
		images.erase(images.begin()+img_index);
		images.insert(images.begin()+img_index-1,img);
		std::swap(texture_list[img_index], texture_list[img_index-1]);
		std::swap(texture_list_alpha[img_index], texture_list_alpha[img_index-1]);
		if(img_index < texture_filenames.size() && img_index - 1 < texture_filenames.size()){
			std::swap(texture_filenames[img_index], texture_filenames[img_index-1]);
		}
		if(img_index < pixbuf.size() && img_index - 1 < pixbuf.size()){
			std::swap(pixbuf[img_index], pixbuf[img_index-1]);
		}
		note_texture_binding_changed("move texture up");
	update_texture_layout_size();
	if(gl_area!=NULL)gtk_gl_area_queue_render(GTK_GL_AREA(gl_area));
}

 void
activate_button_remove_img (GtkComboBox *widget,
               int *     user_data){
int img_index=(*user_data);
	//for(int i=img_index;i<img_counter;i++){
		
		//
	//}
	gtk_container_remove((GtkContainer *)layout2,(GtkWidget *)button_remove_img[img_counter-1]);
	gtk_container_remove((GtkContainer *)layout2,(GtkWidget *)button_movedown_img[img_counter-1]);
	gtk_container_remove((GtkContainer *)layout2,(GtkWidget *)button_moveup_img[img_counter-1]);
	gtk_container_remove((GtkContainer *)layout2,(GtkWidget *)images[img_index]);
	
	//std::cout<<img_index<<std::endl;
	images.erase(images.begin()+img_index);
	texture_list.erase(texture_list.begin()+img_index);
	texture_list_alpha.erase(texture_list_alpha.begin()+img_index);
	if(img_index < texture_filenames.size()){
		texture_filenames.erase(texture_filenames.begin()+img_index);
	}
	if(img_index < pixbuf.size()){
		g_object_unref(pixbuf[img_index]);
		pixbuf.erase(pixbuf.begin()+img_index);
	}
	img_counter--;
	note_texture_binding_changed("remove texture");
	update_texture_layout_size();
	start_async_regeneration(false);
}





void generate_widgets(){
	setup_textures=false;
	setup_vars=false;
	setup_rules=false;
}


gboolean
layout_focus (GtkWidget       *widget,
               GtkDirectionType direction,
               gpointer         user_data){
return false;
}

gboolean
rule_focus (GtkWidget       *widget,
               GtkDirectionType direction,
               gpointer         user_data){
return false;
}

gboolean
vars_focus (GtkWidget       *widget,
               GtkDirectionType direction,
               gpointer         user_data){
return false;
}


gboolean
texture_focus (GtkWidget       *widget,
               GtkDirectionType direction,
               gpointer         user_data){
return false;				   
}



void activate_stop(GtkButton *item) {
	
	play=false;
	if(gl_area!=NULL)gtk_gl_area_queue_render(GTK_GL_AREA(gl_area));
	
}

void activate_play(GtkButton *item) {
	
	play=true;
	if(gl_area!=NULL)gtk_gl_area_queue_render(GTK_GL_AREA(gl_area));
	
}

void activate_reset_view(GtkButton *item) {
	play=false;
	scale_global=1.0f;
	angle_view=0.0f;
	elevation_view=0.0f;

	if(scale_slider_main!=NULL)gtk_range_set_value((GtkRange *)scale_slider_main, scale_global);
	if(scale_slider3!=NULL)gtk_range_set_value((GtkRange *)scale_slider3, angle_view);
	if(scale_slider4!=NULL)gtk_range_set_value((GtkRange *)scale_slider4, elevation_view);
	if(gl_area!=NULL)gtk_gl_area_queue_render(GTK_GL_AREA(gl_area));
}

void activate_open(GtkButton *item) {
	try{
		GtkFileChooserNative *native_open;
		GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
		gint res;
		GtkFileFilter *filter;

		native_open = gtk_file_chooser_native_new("Open Grammar File",
		                                     (GtkWindow *)window,
		                                     action,
		                                     "_Open",
		                                     "_Cancel");

		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, "Grammar file");
		gtk_file_filter_add_pattern(filter, "*.grammar");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(native_open), filter);

		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, "All files");
		gtk_file_filter_add_pattern(filter, "*");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(native_open), filter);

		res = gtk_native_dialog_run(GTK_NATIVE_DIALOG(native_open));
		if (res == GTK_RESPONSE_ACCEPT)
		{
			GtkFileChooser *native_chooser = GTK_FILE_CHOOSER(native_open);
			grammar_filename = std::string(gtk_file_chooser_get_filename(native_chooser));

			std::ifstream fin(grammar_filename.c_str());
			if (!fin.is_open()) {
				errorout("E: Could not open file " + grammar_filename);
			}
			else {
				std::string fulltext;
				std::string line;
				while(std::getline(fin, line)) {
					fulltext += line + "\n";
				}
				fin.close();
				gtk_text_buffer_set_text(mybuffer, fulltext.c_str(), fulltext.length());
				activate(NULL);
			}
		}

		g_object_unref(native_open);
	}catch(...){
		std::string error_str="Error "+std::to_string(22);
		errorout(error_str);
	}
}


			   
void activate_saveas(GtkButton *item) {
	
 try{
    GtkFileChooserNative *native;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
    gint res;
     GtkFileFilter *filter;

    native = gtk_file_chooser_native_new("Save File",
                                         (GtkWindow *)window,
                                         action,
                                         "_Save",
                                         "_Cancel");

    // Filters
    filter = gtk_file_filter_new(); // filter 1
    gtk_file_filter_set_name(filter, "Grammar file");
    gtk_file_filter_add_pattern(filter, "*.grammar");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(native), filter);
    filter = gtk_file_filter_new(); // filter 2
    gtk_file_filter_set_name(filter, "All files");
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(native), filter);

    // default file name
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(native), "test.grammar");

    res = gtk_native_dialog_run(GTK_NATIVE_DIALOG(native));
    if (res == GTK_RESPONSE_ACCEPT)
    {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
        grammar_filename = std::string(gtk_file_chooser_get_filename(chooser));

        // save the file
        save_to_file(grammar_filename.c_str());

activate(NULL);
        //g_free(grammar_filename);
    }

    g_object_unref(native);
}catch(...){
	
	std::string error_str="Error "+std::to_string(22);
	errorout(error_str);
}
}


	   
void activate_choose_image_button(GtkButton *item) {
	errorout("Sketch image import has been removed.");
}
// Text tags for syntax highlighting
GtkTextTag *tag_rule = NULL;     // blue for rule names
GtkTextTag *tag_var = NULL;      // green for variables
GtkTextTag *tag_number = NULL;   // orange for numbers
GtkTextTag *tag_comment = NULL;  // gray for comments
GtkTextTag *tag_op = NULL;       // purple for operators
GtkTextTag *tag_ident = NULL;    // default identifier
GtkTextTag *tag_bold = NULL;     // bold for single-letter tokens



void
txt_inserted (GtkTextBuffer *textbuffer,
               GtkTextIter   *location,
               gchar         *text,
               gint           len,
               gpointer       user_data){
	init_buffer=false;
}


void
txt_changed (GtkTextBuffer *textbuffer,
               gpointer       user_data){
	(void)textbuffer;
	(void)user_data;
}


gboolean
keypressed_textview (GtkWidget *widget,
               GdkEvent  *event,
               gpointer   user_data){
				   //gtk_text_view_reset_cursor_blink((GtkTextView *)view);
				   //std::cout<<"*";
				   return FALSE;
			   }

// Tooltip query handler: show rule/variable info on hover
static gboolean view_query_tooltip_handler(GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer user_data) {
	GtkTextView *tv = GTK_TEXT_VIEW(widget);
	GtkTextIter iter;
	gint bx, by;
	gtk_text_view_window_to_buffer_coords(tv, GTK_TEXT_WINDOW_WIDGET, x, y, &bx, &by);
	if (!gtk_text_view_get_iter_at_location(tv, &iter, bx, by)) return FALSE;
	// find word boundaries
	GtkTextIter s = iter, e = iter;
	if (!gtk_text_iter_starts_word(&s)) gtk_text_iter_backward_word_start(&s);
	gtk_text_iter_forward_word_end(&e);
	gchar *word = gtk_text_buffer_get_text(gtk_text_view_get_buffer(tv), &s, &e, FALSE);
	if (!word) return FALSE;
	std::string tooltip_text;
	std::string wstr(word);
	// Check rules first
	if (grammar) {
		for (auto r : grammar->rule_list) {
			if (!r) continue;
			if (r->rule_name == wstr) {
				std::ostringstream ss;
				ss << "Rule: " << r->rule_name << "\n";
				ss << "Signature: ";
				if (r->var_name != "") ss << r->var_name << " "; else ss << "repeat=" << r->repeat << " ";
				// vars
				if (r->var_counter > 0) {
					ss << "\nVars: ";
					for (int i = 0; i < r->var_counter; ++i) {
						if (r->var_names[i] != "") ss << r->var_names[i] << (i+1<r->var_counter?", ":"");
					}
				}
				ss << "\nTokens (sections): ";
				for (int k = 0; k < 3; ++k) {
					ss << r->section_tokens[k].size();
					if (k < 2) ss << "/";
				}
				ss << "\nProbability: " << r->probability;
				// alternate
				if (r->alternate) ss << "\nHas alternate (" << r->alternate->section_tokens[1].size() << " tokens)";
				// Example: compact print of the rule
				{
					std::string ex = r->print();
					if (ex.size() > 400) ex = ex.substr(0, 400) + "...";
					ss << "\nExample: " << ex;
				}
				tooltip_text = ss.str();
				break;
			}
		}
	}
	// Check variables next
	if (tooltip_text.empty()){
		for (auto v : variable_list) {
			if (!v) continue;
			if (v->var_name == wstr) {
				std::ostringstream ss;
				ss << "Variable: " << v->var_name << "\n";
				ss << "Value: " << v->value;
				ss << "\nMin: " << v->min << " Max: " << v->max;
				ss << (v->integer ? "\nType: integer" : "\nType: float");
				if (v->instance_count > 0) ss << "\nInstances: " << v->instance_count;
				tooltip_text = ss.str();
				break;
			}
		}
	}
	// If still empty, try to find a matching token example in grammar->tokens_new
	if (tooltip_text.empty() && grammar) {
		for (auto t : grammar->tokens_new) {
			if (!t) continue;
			if (t->token_name == wstr) {
				std::ostringstream ss;
				ss << "Token: " << t->token_name << "\n";
				if (t->var_name.size()) ss << "Var: " << t->var_name << "\n";
				ss << "Args: ";
				for (size_t i = 0; i < t->arguments.size(); ++i) {
					if (i) ss << ", "; ss << t->arguments[i];
				}
				if (t->instance_type.size()) ss << "\nInstance: " << t->instance_type;
				std::string p = t->print();
				if (p.size() > 400) p = p.substr(0,400) + "...";
				ss << "\nExample: " << p;
				tooltip_text = ss.str();
				break;
			}
		}
	}
	// Limit tooltip length to avoid huge popups
	if (tooltip_text.size() > 800) tooltip_text = tooltip_text.substr(0,800) + "...";
	if (!tooltip_text.empty()) gtk_tooltip_set_text(tooltip, tooltip_text.c_str());
	g_free(word);
	return !tooltip_text.empty();
}

// Key press handler for indent/newline behavior
static gboolean view_key_press_handler(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
	GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));

	// Handle Tab (insert 4 spaces) and Shift-Tab (dedent)
	if (event->keyval == GDK_KEY_Tab) {
		if (event->state & GDK_SHIFT_MASK) {
			// Dedent: remove up to 4 spaces before cursor on the same line
			GtkTextIter line_start = iter;
			gtk_text_iter_set_line_offset(&line_start, 0);
			GtkTextIter remove_start = iter;
			for (int i = 0; i < 4; ++i) {
				if (gtk_text_iter_compare(&remove_start, &line_start) <= 0) break;
				GtkTextIter prev = remove_start;
				gtk_text_iter_backward_char(&prev);
				gunichar c = gtk_text_iter_get_char(&prev);
				if (c == ' ') remove_start = prev; else break;
			}
			if (gtk_text_iter_compare(&remove_start, &iter) < 0) {
				gtk_text_buffer_delete(buf, &remove_start, &iter);
			}
		} else {
			gtk_text_buffer_insert_at_cursor(buf, "    ", 4);
		}
		return TRUE;
	}

	// Handle Enter: insert newline and copy indentation from current line
	if (event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_ISO_Enter) {
		GtkTextIter line_start = iter;
		gtk_text_iter_set_line_offset(&line_start, 0);
		GtkTextIter ind_end = line_start;
		// move ind_end forward while spaces or tabs
		while (gtk_text_iter_compare(&ind_end, &iter) < 0) {
			gunichar c = gtk_text_iter_get_char(&ind_end);
			if (c == ' ' || c == '\t') gtk_text_iter_forward_char(&ind_end);
			else break;
		}
		gchar *indent = gtk_text_buffer_get_text(buf, &line_start, &ind_end, FALSE);
		gtk_text_buffer_insert_at_cursor(buf, "\n", 1);
		if (indent) { gtk_text_buffer_insert_at_cursor(buf, indent, -1); g_free(indent); }
		return TRUE;
	}

	return FALSE; // let GTK handle other keys
}

void activate_perspective_transform_button(GtkButton *button){
	errorout("Perspective transform has been removed.");
}

void activateeval(GtkButton *item) {
	errorout("Sketch evaluation has been removed.");
}


std::string tohexstr(unsigned short uchar){
	std::ostringstream ss;
	ss << std::hex<<uchar;
	return ss.str();
}



void activate_texture_filechooser(GtkButton *item) {
	try{
GtkFileChooserNative *native;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;
     GtkFileFilter *filter;

    native = gtk_file_chooser_native_new("Open Texture File",
                                         (GtkWindow *)window,
                                         action,
                                         "_Open",
                                         "_Cancel");

    // Filters
    filter = gtk_file_filter_new(); // filter 1
    gtk_file_filter_set_name(filter, "texture file");
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(native), filter);
    
    filter = gtk_file_filter_new(); // filter 1
    gtk_file_filter_set_name(filter, "texture file");
    gtk_file_filter_add_pattern(filter, "*.jpg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(native), filter);
    
    filter = gtk_file_filter_new(); // filter 2
    gtk_file_filter_set_name(filter, "All files");
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(native), filter);

    // default file name
    //gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(native), "texture.png");
     std::string filename;
    res = gtk_native_dialog_run(GTK_NATIVE_DIALOG(native));
    if (res == GTK_RESPONSE_ACCEPT)
    {
   
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
        filename = gtk_file_chooser_get_filename(chooser);
		if(img_counter >= 50){
			errorout("Texture limit reached for inspector controls");
			g_object_unref(native);
			return;
		}
		
	
		
		
		//for(int i=0;i<img_counter;i++){
		 // gtk_container_remove((GtkContainer *)layout2,images[i]);
		//}
		
		
		//texture_filenames.push_back(filename);
			
			
		
			 //int picwidth,picheight,n;
			// unsigned char* texture_data = stbi_load(filename.c_str(), &picwidth, &picheight, &n,STBI_rgb_alpha);
			if(!append_texture_preview_image(filename)){
				g_object_unref(native);
				return;
			}
			if(gl_area!=NULL){
				gtk_gl_area_make_current(GTK_GL_AREA(gl_area));
			}
			int texid = generateTexture(filename.c_str());
			texture_list.push_back(texid);
			texture_list_alpha.push_back(1.0f);
			texture_filenames.push_back(filename);
			note_texture_binding_changed("add texture");
			
			/*if(){
					SSL_CTX *ctx;
					int client;
					SSL *ssl;
					char buf[20024];
					char input[BUFFER];
					int bytes;
					std::string writeout;
					
		
				ctx = InitCTX();
				LoadCertificates(ctx, "mycert.pem", "mycertkey.pem");
					client= OpenConnection(hostname.c_str(), atoi(portnum.c_str()));   
					ssl = SSL_new(ctx);      
					SSL_set_fd(ssl, client);  
					if ( SSL_connect(ssl) != FAIL ) 
					{    
					
					
						std::cout<<"Connected with encryption "<< SSL_get_cipher(ssl)<<std::endl;
						ShowCerts(ssl);
						


							
							SSL_write(ssl, username.c_str(), strlen(username.c_str()));  
							bytes = SSL_read(ssl, buf, sizeof(buf)); 
							SSL_write(ssl, password.c_str(), strlen(password.c_str())); 
						
							bytes = SSL_read(ssl, buf, sizeof(buf));
							buf[bytes]=0;
							std::string OK=buf;
							std::cout<<OK<<std::endl;
							if(OK=="DOWN"){
								writeout="TEXTUREUP";
								SSL_write(ssl, writeout.c_str(),strlen(writeout.c_str()));
								bytes = SSL_read(ssl, buf, sizeof(buf)); 
								buf[bytes]=0;
								 OK=buf;
								 
								for(int j=0;j<20;j++){
											std::stringstream	ss;		
									for(int i=j*128*128*3/20;i<(j+1)*128*128*3/20;i++){
									
										   ss  <<(int)(newtexture.data[i])<<",";
										
									}
									std::string outstr=ss.str();
									SSL_write(ssl, outstr.c_str(),strlen(outstr.c_str()));
									//std::cout<<outstr<<std::endl;
								}
								bytes = SSL_read(ssl, buf, sizeof(buf));
								buf[bytes]=0;
								 OK=buf;
								
								}
							else if(OK=="UP"){
								std::ifstream fin(grammar_filename.c_str());
								if (!fin.is_open()) {
									std::cout << "E: Could not open file " << grammar_filename << std::endl;
								}
								std::string fulltext;
								std::string line;
								while(std::getline(fin, line) ) {
								  fulltext+=line;
								}
								fin.close();
								SSL_write(ssl, fulltext.c_str(), strlen(fulltext.c_str())); 
								
							}
						
					}
					
					SSL_free(ssl);       
					close(client);       
					SSL_CTX_free(ctx);   
					
				}
				else {
				}
		*/
		//images[img_counter]=gtk_image_new_from_file(("texture"+std::to_string(img_counter)+".png").c_str());
		
		
	//	for(int i=0;i<img_counter;i++){
	//			gtk_container_remove((GtkContainer *)layout2,(GtkWidget *)button_remove_img[i]);
	//			gtk_container_remove((GtkContainer *)layout2,(GtkWidget *)images[i]);
	//		}
		
		img_counter++;
		//gtk_widget_queue_draw_area(drawing_area2,0,0, draw2width,draw2height);

		initialize_render_texture_buffers();
		
		start_async_regeneration(false);
        
    }
	 g_object_unref(native);

}   catch(...){
	
	std::string error_str="Error "+std::to_string(22);
	errorout(error_str);
}

			  
}	
void activate_generate_image_button(GtkButton *item, gpointer userdata){
	errorout("Procedural texture generation has been removed.");
}
			   
void activateply(GtkButton *item) {
	try{
GtkFileChooserNative *native=NULL;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
    gint res;
     GtkFileFilter *filter;

    native = gtk_file_chooser_native_new("Save File",
                                         (GtkWindow *)window,
                                         action,
                                         "_Save",
                                         "_Cancel");

    // Filters
    filter = gtk_file_filter_new(); // filter 1
    gtk_file_filter_set_name(filter, "PLY file");
    gtk_file_filter_add_pattern(filter, "*.ply");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(native), filter);
    filter = gtk_file_filter_new(); // filter 2
    gtk_file_filter_set_name(filter, "All files");
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(native), filter);

    // default file name
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(native), "test.ply");

    res = gtk_native_dialog_run(GTK_NATIVE_DIALOG(native));
    if (res == GTK_RESPONSE_ACCEPT)
    {
        std::string filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
        filename = gtk_file_chooser_get_filename(chooser);
        auto mydata = build_base_vertex_data();
        grammar->context->PLY(mydata.data(),filename);
         
        //if(filename!=NULL)g_free(filename);
        //else throw 2;
    }
    else 
    throw 555;

    if(native!=NULL)g_object_unref(native);
    else throw 1;
}
catch(...){
	
	std::string error_str="Error "+std::to_string(22);
	errorout(error_str);
}
}		


gboolean mouse_press_callback(GtkWidget *da, GdkEventButton *event, gpointer userdata)
{
	return TRUE;
}
static gboolean
handle_mouse (GtkWidget *da, GdkEventButton *event, gpointer t){
	 int w = gtk_widget_get_allocated_width (da);
  int h = gtk_widget_get_allocated_height (da);
  mousex=event->x;
  mousey=event->y;
	gtk_widget_queue_draw_area (da, 0, 0, w, h);
	return TRUE;
}


static gboolean
handle_mouse2 (GtkWidget *da, GdkEventButton *event, gpointer t){
	 int w = gtk_widget_get_allocated_width (da);
  int h = gtk_widget_get_allocated_height (da);
  //mousex=event->x;
  //mousey=event->y;
	gtk_widget_queue_draw_area (da, 0, 0, w, h);
	return TRUE;
}
		





void activate_skip(GtkButton *item) {
	show_main_workspace();
}
  
void activate_login(GtkButton *item) {



	username=gtk_entry_get_text((GtkEntry *)entry1);
	password=gtk_entry_get_text((GtkEntry *)entry2);
	  
	   std::ofstream fout("userdetails.txt");
		if (!fout.is_open()) {
			std::cerr << "E: Could not open file for saving"  << std::endl;
			
		}
		else {
		
			fout<<username<<std::endl;
			fout<<password;

			fout.close();
		}

loggedin=true;

SSL_CTX *ctx;
	int client;
	SSL *ssl;
	char buf[10024];
	char input[BUFFER];
	int bytes;
	std::string writeout;

ctx = InitCTX();
LoadCertificates(ctx, "mycert.pem", "mycertkey.pem");
	client= OpenConnection(hostname.c_str(), atoi(portnum.c_str()));   /*converting ascii port to interger */
	ssl = SSL_new(ctx);      /* create new SSL connection state */
	SSL_set_fd(ssl, client);    /* attach the socket descriptor */
	if ( SSL_connect(ssl) != FAIL )   /* perform the connection */
	{    
	
	
		std::cout<<"Connected with encryption "<< SSL_get_cipher(ssl)<<std::endl;
		ShowCerts(ssl);
		/* get any certs */


		
			
			
			SSL_write(ssl, username.c_str(), strlen(username.c_str()));   /* encrypt & send message */
			bytes = SSL_read(ssl, buf, sizeof(buf)); /* get request */
			SSL_write(ssl, password.c_str(), strlen(password.c_str()));   /* encrypt & send message */
		
			bytes = SSL_read(ssl, buf, sizeof(buf)); /* get request */
			buf[bytes]=0;
			std::string OK=buf;
			if(OK=="UP") {
				std::ifstream fin(grammar_filename.c_str());
				if (!fin.is_open()) {
					std::cout << "E: Could not open file " << grammar_filename << std::endl;
				}
				std::string fulltext;
				std::string line;
				while(std::getline(fin, line) ) {
				  fulltext+=line;
				}
				fin.close();
				SSL_write(ssl, fulltext.c_str(), strlen(fulltext.c_str())); 
	
			}
			else if(OK=="DOWN"){
				writeout="GRAMMAR";
				SSL_write(ssl, writeout.c_str(), strlen(writeout.c_str()));
				bytes = SSL_read(ssl, buf, sizeof(buf)); /* get request */
				buf[bytes]=0;
				std::string fulltext=buf;
				std::ofstream fout(grammar_filename.c_str());
				fout<<fulltext;
				fout.close();
			}
		}
	
	SSL_free(ssl);        /* release connection state */
	close(client);         /* close socket */
	SSL_CTX_free(ctx);        /* release */
	
show_main_workspace();
}
std::string text1,text2;
			   
void activate_app(GtkApplication *app){
	main_thread_id = std::this_thread::get_id();
	debugout("activate_app: building interface");
		
		
	
	uint64_t timeSeed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::seed_seq ss{uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed>>32)};
    rng.seed(ss);
    
    
  /* Create new top level window. */
  window = gtk_application_window_new (app);//gtk_window_new( GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(window), 1280, 800);
  gtk_window_maximize(GTK_WINDOW(window));
  gtk_window_set_title(GTK_WINDOW(window), appname.c_str());
  gtk_container_set_border_width(GTK_CONTAINER(window), 0);
  install_app_css();
  add_css_class(window, "progen-window");
 
  
  
  
  
  root_shell = gtk_box_new (GTK_ORIENTATION_VERTICAL, FALSE);
  top_toolbar_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, FALSE);
  GtkActionBar *viewer_zoom_bar = (GtkActionBar *)gtk_action_bar_new ();
  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, FALSE);
  gtk_widget_set_hexpand(root_shell, TRUE);
  gtk_widget_set_vexpand(root_shell, TRUE);
  gtk_box_set_spacing(GTK_BOX(root_shell), 18);
  gtk_widget_set_margin_start(root_shell, 16);
  gtk_widget_set_margin_end(root_shell, 16);
  gtk_widget_set_margin_top(root_shell, 16);
  gtk_widget_set_margin_bottom(root_shell, 16);
  gtk_widget_set_hexpand(top_toolbar_box, TRUE);
  gtk_box_set_spacing(GTK_BOX(top_toolbar_box), 0);
  gtk_widget_set_hexpand(box, TRUE);
  gtk_widget_set_vexpand(box, TRUE);
  gtk_box_set_spacing(GTK_BOX(box), 16);
  //box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, FALSE);
  paned=gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  box3=gtk_box_new(GTK_ORIENTATION_VERTICAL, FALSE);
  gtk_box_set_spacing(GTK_BOX(box3), 16);
  gtk_widget_set_hexpand(box3, TRUE);
  gtk_widget_set_vexpand(box3, TRUE);
  //box10 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, FALSE);
  box10=gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_paned_set_wide_handle(GTK_PANED(paned), TRUE);
  //g_object_set (box, "margin", 12, NULL);
  //gtk_box_set_spacing (GTK_BOX (box), 6);
//  g_object_set (paned, "margin", 12, NULL);
 // gtk_box_set_spacing (GTK_BOX (paned), 6);
  
 actionbar3=NULL;
 actionbar6=NULL;
 actionbar7=NULL;
 add_css_class(top_toolbar_box, "shell-header");
 add_css_class(box3, "preview-pane");
 add_css_class(box, "workspace-pane");

  
  
  gl_area = gtk_gl_area_new();
  gtk_gl_area_set_has_depth_buffer(GTK_GL_AREA(gl_area), TRUE);
  gtk_gl_area_set_has_stencil_buffer(GTK_GL_AREA(gl_area), FALSE);
  gtk_gl_area_set_has_alpha(GTK_GL_AREA(gl_area), FALSE);
  gtk_widget_add_events(gl_area, 
      GDK_BUTTON_PRESS_MASK | 
      GDK_BUTTON_RELEASE_MASK | 
      GDK_POINTER_MOTION_MASK);
  g_signal_connect(gl_area, "button-press-event", G_CALLBACK(on_button_press), NULL);
  g_signal_connect(gl_area, "button-release-event", G_CALLBACK(on_button_release), NULL);
  g_signal_connect(gl_area, "motion-notify-event", G_CALLBACK(on_motion_notify), NULL);
  g_signal_connect(gl_area, "realize", G_CALLBACK(realize), NULL);
  g_signal_connect(gl_area, "unrealize", G_CALLBACK(unrealize), NULL);
  g_signal_connect(gl_area, "resize", G_CALLBACK(resize_glarea), NULL);
  g_signal_connect(gl_area, "render", G_CALLBACK(render), NULL);
  GtkWidget *scrollwin=gtk_scrolled_window_new (NULL,NULL);
#if HAVE_GTKSOURCEVIEW
  GtkSourceBuffer *source_buffer = gtk_source_buffer_new(NULL);
  view = gtk_source_view_new_with_buffer(source_buffer);
#else
  view = gtk_text_view_new();
#endif
  viewerror = gtk_text_view_new();
  gtk_widget_set_margin_top(scrollwin, 4);
  gtk_text_view_set_top_margin(GTK_TEXT_VIEW(view), 10);
  gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(view), 10);
  gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(view), GTK_TEXT_WINDOW_TOP, 8);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 10);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(view), 10);
#if HAVE_GTKSOURCEVIEW
  gtk_source_buffer_set_highlight_syntax(source_buffer, FALSE);
  gtk_source_buffer_set_highlight_matching_brackets(source_buffer, TRUE);
  gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(view), TRUE);
  gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(view), 4);
  gtk_source_view_set_indent_width(GTK_SOURCE_VIEW(view), 4);
  gtk_source_view_set_insert_spaces_instead_of_tabs(GTK_SOURCE_VIEW(view), TRUE);
  gtk_source_view_set_auto_indent(GTK_SOURCE_VIEW(view), TRUE);
  gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(view), TRUE);
  gtk_source_view_set_show_right_margin(GTK_SOURCE_VIEW(view), TRUE);
  gtk_source_view_set_right_margin_position(GTK_SOURCE_VIEW(view), 100);
#endif
  gtk_text_view_set_wrap_mode((GtkTextView *)view,GTK_WRAP_NONE);
  gtk_text_view_set_monospace ((GtkTextView *)view,TRUE);
  gtk_text_view_set_editable((GtkTextView *)viewerror,FALSE);
  gtk_text_view_set_cursor_visible((GtkTextView *)viewerror,FALSE);
  gtk_text_view_set_wrap_mode((GtkTextView *)viewerror,GTK_WRAP_NONE);
  gtk_text_view_set_monospace ((GtkTextView *)viewerror,TRUE);
  add_css_class(view, "editor-view");
  add_css_class(viewerror, "console-view");

	// Disable hover tooltips in the editor to avoid query churn while typing.
	gtk_widget_set_has_tooltip(view, FALSE);

  // Bind the editor buffer.
#if HAVE_GTKSOURCEVIEW
  mybuffer = GTK_TEXT_BUFFER(source_buffer);
#else
  mybuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
#endif
  // Keep the editor interaction path lightweight while typing.
  gtk_container_add (GTK_CONTAINER (scrollwin), view);
  
  
  
  GtkWidget *scrollwin4 = gtk_scrolled_window_new(NULL, NULL);
  GtkWidget *right_panel_paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
  GtkWidget *header_shell = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 18);
  GtkWidget *header_brand = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  GtkWidget *header_actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  GtkWidget *header_status = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  GtkWidget *preview_intro = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  GtkWidget *workspace_intro = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  GtkWidget *preview_frame = gtk_frame_new(NULL);
  GtkWidget *preview_shell = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
  GtkWidget *stage_frame = gtk_frame_new(NULL);
  GtkWidget *preview_controls_frame = gtk_frame_new(NULL);
  GtkWidget *preview_controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  GtkWidget *preview_action_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  GtkWidget *editor_frame = gtk_frame_new(NULL);
  GtkWidget *console_frame = gtk_frame_new(NULL);
  GtkWidget *editor_shell = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  GtkWidget *console_shell = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  GtkWidget *brand_kicker = gtk_label_new("PROCEDURAL MODEL STUDIO");
  GtkWidget *brand_title = gtk_label_new("Progen3d");
  GtkWidget *brand_note = gtk_label_new("Design grammars, regenerate meshes, and inspect the live result.");
  GtkWidget *preview_kicker = gtk_label_new("RENDER");
  GtkWidget *preview_title = gtk_label_new("Preview Stage");
  GtkWidget *preview_subtitle = gtk_label_new("Use the camera rail and transport controls to inspect the generated model.");
  GtkWidget *workspace_kicker = gtk_label_new("WORKSPACE");
  GtkWidget *workspace_title = gtk_label_new("Grammar Editor");
  GtkWidget *workspace_subtitle = gtk_label_new("Edit the active grammar and monitor regeneration output in one place.");
  GtkWidget *editor_label = gtk_label_new("Editor");
  GtkWidget *console_label = gtk_label_new("Console");
  add_css_class(header_actions, "header-actions");
  add_css_class(preview_action_row, "preview-actions");
  add_css_class(brand_kicker, "brand-kicker");
  add_css_class(brand_title, "brand-title");
  add_css_class(brand_note, "brand-note");
  add_css_class(preview_kicker, "column-kicker");
  add_css_class(preview_title, "column-title");
  add_css_class(preview_subtitle, "column-subtitle");
  add_css_class(workspace_kicker, "column-kicker");
  add_css_class(workspace_title, "column-title");
  add_css_class(workspace_subtitle, "column-subtitle");
  add_css_class(editor_label, "panel-label");
  add_css_class(console_label, "panel-label");
  add_css_class(preview_frame, "surface-frame");
  add_css_class(stage_frame, "viewport-frame");
  add_css_class(preview_controls_frame, "surface-frame");
  add_css_class(editor_frame, "surface-frame");
  add_css_class(console_frame, "surface-frame");
  add_css_class(editor_shell, "panel-body");
  add_css_class(console_shell, "panel-body");
  add_css_class(header_status, "status-pill");

  gtk_label_set_xalign(GTK_LABEL(brand_kicker), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(brand_title), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(brand_note), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(preview_kicker), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(preview_title), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(preview_subtitle), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(workspace_kicker), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(workspace_title), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(workspace_subtitle), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(editor_label), 0.0f);
  gtk_label_set_xalign(GTK_LABEL(console_label), 0.0f);

  gtk_container_add(GTK_CONTAINER(scrollwin4), viewerror);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin4), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrollwin4), 180);
  gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(scrollwin4), FALSE);

  gtk_widget_set_hexpand(scrollwin, TRUE);
  gtk_widget_set_vexpand(scrollwin, TRUE);
  gtk_widget_set_hexpand(scrollwin4, TRUE);
  gtk_widget_set_vexpand(scrollwin4, TRUE);
  gtk_widget_set_hexpand(right_panel_paned, TRUE);
  gtk_widget_set_vexpand(right_panel_paned, TRUE);
  gtk_widget_set_hexpand(editor_frame, TRUE);
  gtk_widget_set_vexpand(editor_frame, TRUE);
  gtk_widget_set_hexpand(console_frame, TRUE);
  gtk_widget_set_vexpand(console_frame, FALSE);
  gtk_widget_set_hexpand(editor_shell, TRUE);
  gtk_widget_set_vexpand(editor_shell, TRUE);
  gtk_widget_set_hexpand(console_shell, TRUE);
  gtk_widget_set_vexpand(console_shell, TRUE);
  gtk_widget_set_hexpand(header_brand, TRUE);
  gtk_widget_set_hexpand(header_actions, FALSE);
  gtk_widget_set_halign(header_actions, GTK_ALIGN_END);
  gtk_widget_set_valign(header_actions, GTK_ALIGN_CENTER);
  gtk_widget_set_halign(header_status, GTK_ALIGN_END);
  gtk_widget_set_hexpand(gl_area, TRUE);
  gtk_widget_set_vexpand(gl_area, TRUE);
  gtk_widget_set_hexpand(box3, TRUE);
  gtk_widget_set_vexpand(box3, TRUE);
  gtk_widget_set_hexpand(box, TRUE);
  gtk_widget_set_vexpand(box, TRUE);
  gtk_widget_set_hexpand(preview_frame, TRUE);
  gtk_widget_set_vexpand(preview_frame, TRUE);
  gtk_widget_set_hexpand(stage_frame, TRUE);
  gtk_widget_set_vexpand(stage_frame, TRUE);
  gtk_widget_set_hexpand(preview_controls_frame, TRUE);
  gtk_widget_set_vexpand(preview_controls_frame, FALSE);
  gtk_widget_set_hexpand(preview_controls, TRUE);
  gtk_widget_set_size_request(gl_area, 360, 360);

  gtk_widget_set_margin_start(preview_shell, 16);
  gtk_widget_set_margin_end(preview_shell, 16);
  gtk_widget_set_margin_top(preview_shell, 16);
  gtk_widget_set_margin_bottom(preview_shell, 16);
  gtk_widget_set_margin_start(preview_controls, 16);
  gtk_widget_set_margin_end(preview_controls, 16);
  gtk_widget_set_margin_top(preview_controls, 16);
  gtk_widget_set_margin_bottom(preview_controls, 16);
  gtk_widget_set_margin_start(editor_shell, 14);
  gtk_widget_set_margin_end(editor_shell, 14);
  gtk_widget_set_margin_top(editor_shell, 14);
  gtk_widget_set_margin_bottom(editor_shell, 14);
  gtk_widget_set_margin_start(console_shell, 14);
  gtk_widget_set_margin_end(console_shell, 14);
  gtk_widget_set_margin_top(console_shell, 14);
  gtk_widget_set_margin_bottom(console_shell, 14);

  file_label = gtk_label_new("");
  update_file_label();
  gtk_widget_set_hexpand(file_label, FALSE);
  gtk_widget_set_halign(file_label, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_start(file_label, 10);
  gtk_widget_set_margin_end(file_label, 10);
  gtk_widget_set_margin_top(file_label, 8);
  gtk_widget_set_margin_bottom(file_label, 8);
  gtk_label_set_xalign(GTK_LABEL(file_label), 0.5f);

  layout = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(layout), 8);
  gtk_grid_set_column_spacing(GTK_GRID(layout), 12);
  gtk_grid_set_column_homogeneous(GTK_GRID(layout), FALSE);
  gtk_widget_set_margin_start(layout, 8);
  gtk_widget_set_margin_end(layout, 8);
  gtk_widget_set_margin_top(layout, 8);
  gtk_widget_set_margin_bottom(layout, 8);
  layout2 = NULL;
  layout3 = NULL;
  layout4 = NULL;
  texture_scrollwin = NULL;
  vadj = NULL;

  texture_filechooser = gtk_button_new_with_label("Add Texture");
  grammar_mode_button = gtk_button_new_with_label("Focus Editor");
  inspector_mode_button = NULL;
  openbutton = gtk_button_new_with_label("Open");
  runbutton = gtk_button_new_with_label("Run");
  newbutton = gtk_button_new_with_label("New");
  savebutton = gtk_button_new_with_label("Save As");
  plybutton = gtk_button_new_with_label("Export PLY");
  resetviewbutton = gtk_button_new_with_label("Reset View");
  playbutton3 = gtk_button_new_with_label("Play");
  stopbutton3 = gtk_button_new_with_label("Stop");
  scale_slider_main = NULL;
  scale_slider3 = NULL;
  scale_slider4 = NULL;

  add_css_class(runbutton, "primary-action");

  gtk_widget_set_size_request(grammar_mode_button, 118, 34);
  gtk_widget_set_size_request(openbutton, 84, 34);
  gtk_widget_set_size_request(runbutton, 84, 34);
  gtk_widget_set_size_request(newbutton, 84, 34);
  gtk_widget_set_size_request(savebutton, 92, 34);
  gtk_widget_set_size_request(texture_filechooser, 112, 34);
  gtk_widget_set_size_request(plybutton, 102, 34);
  gtk_widget_set_size_request(resetviewbutton, 110, 34);
  gtk_widget_set_size_request(playbutton3, 78, 34);
  gtk_widget_set_size_request(stopbutton3, 78, 34);

  gtk_box_pack_start(GTK_BOX(header_brand), brand_kicker, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(header_brand), brand_title, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(header_brand), brand_note, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(header_status), file_label);
  gtk_box_pack_start(GTK_BOX(header_actions), grammar_mode_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(header_actions), runbutton, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(header_actions), openbutton, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(header_actions), newbutton, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(header_actions), savebutton, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(header_actions), texture_filechooser, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(header_actions), plybutton, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(header_actions), header_status, FALSE, FALSE, 8);
  gtk_box_pack_start(GTK_BOX(header_shell), header_brand, TRUE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(header_shell), header_actions, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(preview_intro), preview_kicker, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(preview_intro), preview_title, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(preview_intro), preview_subtitle, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(workspace_intro), workspace_kicker, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(workspace_intro), workspace_title, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(workspace_intro), workspace_subtitle, FALSE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER(stage_frame), gl_area);
  gtk_container_add(GTK_CONTAINER(preview_frame), preview_shell);
  gtk_box_pack_start(GTK_BOX(preview_shell), stage_frame, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(preview_action_row), resetviewbutton, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(preview_action_row), playbutton3, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(preview_action_row), stopbutton3, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(preview_controls), preview_action_row, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(preview_controls_frame), preview_controls);

  gtk_box_pack_start(GTK_BOX(box3), preview_intro, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box3), preview_frame, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box3), preview_controls_frame, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(editor_shell), editor_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(editor_shell), scrollwin, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(console_shell), console_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(console_shell), scrollwin4, TRUE, TRUE, 0);
  gtk_container_add(GTK_CONTAINER(editor_frame), editor_shell);
  gtk_container_add(GTK_CONTAINER(console_frame), console_shell);
  gtk_paned_pack1(GTK_PANED(right_panel_paned), editor_frame, TRUE, FALSE);
  gtk_paned_pack2(GTK_PANED(right_panel_paned), console_frame, FALSE, FALSE);
  gtk_paned_set_position(GTK_PANED(right_panel_paned), 540);

  gtk_box_pack_start(GTK_BOX(box), workspace_intro, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), right_panel_paned, TRUE, TRUE, 0);

  gtk_paned_pack1(GTK_PANED(paned), box3, TRUE, FALSE);
  gtk_paned_pack2(GTK_PANED(paned), box, TRUE, FALSE);
  gtk_paned_set_position(GTK_PANED(paned), 470);
  
  
  
  
  
   gtk_text_view_set_editable((GtkTextView *)view,TRUE);
    //gtk_text_view_set_overwrite((GtkTextView *)view,TRUE);

 
   
    errorbuffer= gtk_text_view_get_buffer (GTK_TEXT_VIEW (viewerror));
    flush_pending_console_messages();
    
    mybuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    debugout("activate_app: GUI ready");
    
    g_signal_connect (mybuffer ,"insert-text", G_CALLBACK (txt_inserted), NULL);  
    g_signal_connect (grammar_mode_button ,"clicked", G_CALLBACK (activate_show_grammar), NULL);
    g_signal_connect (openbutton ,"clicked", G_CALLBACK (activate_open), NULL);
    g_signal_connect (runbutton ,"clicked", G_CALLBACK (activate), NULL);
    g_signal_connect (newbutton ,"clicked", G_CALLBACK (new_activate), NULL);
    g_signal_connect (savebutton ,"clicked", G_CALLBACK (activate_saveas), NULL); 
    g_signal_connect (resetviewbutton ,"clicked", G_CALLBACK (activate_reset_view), NULL);
    g_signal_connect (texture_filechooser,"clicked", G_CALLBACK (activate_texture_filechooser), NULL); 
    
    g_signal_connect (plybutton ,"clicked", G_CALLBACK (activateply), NULL); 
    
    g_signal_connect (playbutton3 ,"clicked", G_CALLBACK (activate_play), NULL);
    g_signal_connect (stopbutton3 ,"clicked", G_CALLBACK (activate_stop), NULL); 
  /* Quit form main if got delete event */
  g_signal_connect(G_OBJECT(window), "delete-event",
                 G_CALLBACK(gtk_main_quit), NULL);
                 
                 
  gtk_box_pack_start(GTK_BOX(top_toolbar_box), header_shell, 0, 0, 0);
  gtk_box_pack_start(GTK_BOX(root_shell), top_toolbar_box, 0, 0, 0);
  gtk_box_pack_start(GTK_BOX(root_shell), (GtkWidget *)paned, 1, 1, 0);
  gtk_container_add(GTK_CONTAINER(window), root_shell);
  install_gui_event_logging(window);
  gtk_widget_show_all(GTK_WIDGET(window));
    gtk_window_present (GTK_WINDOW (window));
   
   
}
   
int main( int argc, char* argv[] ){

GtkApplication *app;
install_console_stream_redirects();
debugout("main: starting application");

for(int i=0;i<100;i++)rulenames[i]=new char(40);//due to GTK combobox error

	
	
	
	
	
	
	
	SSL_library_init();   /*load encryption and hash algo's in ssl*/
	debugout("main: SSL initialized");
	
	
	
	
	
    app= gtk_application_new ("snap.progen3d.progen3d", G_APPLICATION_DEFAULT_FLAGS);	
       g_signal_connect (app ,"activate", G_CALLBACK (activate_app), NULL);
	debugout("main: entering GTK application loop");
   
  
  
     int status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);





  return status;
  
	
}
