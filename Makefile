TARGET = progen3d

CXX = g++
IMGUI_LIB = third_party/debs/imgui/usr/lib/x86_64-linux-gnu/libimgui.a
GLFW_LIB = third_party/debs/glfw/usr/lib/x86_64-linux-gnu/libglfw.so.3.3
IMGUI_BACKEND_SRCS = \
	third_party/debs/imgui/usr/share/doc/libimgui-dev/examples/backends/imgui_impl_glfw.cpp \
	third_party/debs/imgui/usr/share/doc/libimgui-dev/examples/backends/imgui_impl_opengl3.cpp
OBJS = \
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
	-Ithird_party/debs/imgui/usr/include/imgui \
	-Ithird_party/debs/imgui/usr/include/imgui/backends \
	-Ithird_party/debs/glfw-dev/usr/include \
	-I/usr/include/glm \
	-I/usr/include/stb \
	-Ithird_party/debs/stb/usr/include/stb

LDFLAGS = -Wl,-rpath,'$$ORIGIN/third_party/debs/glfw/usr/lib/x86_64-linux-gnu'
LIBS = $(IMGUI_LIB) $(GLFW_LIB) -lGLU -lGL -lepoxy -ldl -lm -lpthread

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

imgui_impl_glfw.o: third_party/debs/imgui/usr/share/doc/libimgui-dev/examples/backends/imgui_impl_glfw.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

imgui_impl_opengl3.o: third_party/debs/imgui/usr/share/doc/libimgui-dev/examples/backends/imgui_impl_opengl3.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) *.o stbimage/*.o
