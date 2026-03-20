TARGET = progen3d

CXX = g++
IMGUI_ROOT ?= third_party/debs/imgui/usr
IMGUI_INCLUDE_DIR ?= $(IMGUI_ROOT)/include/imgui
IMGUI_BACKEND_INCLUDE_DIR ?= $(IMGUI_INCLUDE_DIR)/backends
IMGUI_BACKEND_DIR ?= $(IMGUI_ROOT)/share/doc/libimgui-dev/examples/backends
IMGUI_LIB ?= $(IMGUI_ROOT)/lib/x86_64-linux-gnu/libimgui.a
GLFW_LIB ?= third_party/debs/glfw/usr/lib/x86_64-linux-gnu/libglfw.so.3.3

ifneq ($(strip $(IMGUI_CORE_DIR)),)
IMGUI_CORE_OBJS = \
	imgui.o \
	imgui_draw.o \
	imgui_tables.o \
	imgui_widgets.o
IMGUI_LINK_LIBS =
else
IMGUI_CORE_OBJS =
IMGUI_LINK_LIBS = $(IMGUI_LIB)
endif

OBJS = \
	$(IMGUI_CORE_OBJS) \
	imgui_main.o \
	imgui_render.o \
	imgui_stb_impl.o \
	Grammar.o \
	Context.o \
	Mesh.o \
	PLYWriter.o \
	Scope.o \
	Solution.o \
	stbimage/stb_image_impl.o \
	imgui_impl_glfw.o \
	imgui_impl_opengl3.o

CXXFLAGS = -g -O3 -std=c++17 \
	-I/usr/include \
	-I$(IMGUI_INCLUDE_DIR) \
	-I$(IMGUI_BACKEND_INCLUDE_DIR) \
	-Ithird_party/debs/glfw-dev/usr/include \
	-I/usr/include/glm \
	-I/usr/include/stb \
	-Ithird_party/debs/stb/usr/include/stb

LDFLAGS = -Wl,-rpath,'$$ORIGIN/third_party/debs/glfw/usr/lib/x86_64-linux-gnu'
LIBS = $(IMGUI_LINK_LIBS) $(GLFW_LIB) -lGLU -lGL -lepoxy -ldl -lm -lpthread

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

imgui.o: $(IMGUI_CORE_DIR)/imgui.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

imgui_draw.o: $(IMGUI_CORE_DIR)/imgui_draw.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

imgui_tables.o: $(IMGUI_CORE_DIR)/imgui_tables.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

imgui_widgets.o: $(IMGUI_CORE_DIR)/imgui_widgets.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

imgui_impl_glfw.o: $(IMGUI_BACKEND_DIR)/imgui_impl_glfw.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

imgui_impl_opengl3.o: $(IMGUI_BACKEND_DIR)/imgui_impl_opengl3.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) *.o stbimage/*.o
