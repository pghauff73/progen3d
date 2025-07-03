


TARGET=progen3d
OBJS=main.o  Grammar.o   Context.o   Mesh.o  PLYWriter.o  Scope.o Solution.o
       
CXX = g++
CXXFLAGS =  -g -O3 -I /usr/include -I /usr/include/atkmm-1.6/ -I /usr/include/pangomm-1.4/ -I /usr/include/freetype2/ -I /usr/include/cairomm-1.0/ \
             -I /usr/lib/x86_64-linux-gnu/pangomm-1.4/include/  \
              -I /usr/lib/x86_64-linux-gnu/sigc++-2.0/include/ -I /usr/include/sigc++-2.0/   \
              -I /usr/include/gtk-3.0/ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/pango-1.0 -I/usr/include/harfbuzz \
             -I/usr/include/cairo -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/at-spi2-atk/2.0 -I/usr/include/at-spi-2.0 -I/usr/include/dbus-1.0 \
             -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include -I/usr/include/atk-1.0  -I /usr/include/opencv4/  -L /usr/lib/x86_64-linux-gnu/
             
             
LIBS =                /lib/x86_64-linux-gnu/libGLU.a         /lib/x86_64-linux-gnu/libpng16.a                          /lib/x86_64-linux-gnu/libX11.a      
LIBDYN = -lstdc++ -lgtk-3 -lpango-1.0 -lgdk-3 -lgdk_pixbuf-2.0 -lpangocairo-1.0    \
  -lgio-2.0 -lgobject-2.0 -lglib-2.0  -lGLU -lGL   -lepoxy -ldl -lm -lpthread -lcairo -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_highgui -lcrypto -lssl


LIBSA =            			   /usr/lib/x86_64-linux-gnu/libgmodule-2.0.a  \
			 /usr/lib/x86_64-linux-gnu/libX11.a /usr/lib/x86_64-linux-gnu/libXi.a /usr/lib/x86_64-linux-gnu/libXfixes.a /usr/lib/x86_64-linux-gnu/libcairo-gobject.a \
			  /usr/lib/x86_64-linux-gnu/libcairo.a   \
			    /usr/lib/x86_64-linux-gnu/libfribidi.a  \
			    /usr/lib/x86_64-linux-gnu/libharfbuzz.a /usr/lib/x86_64-linux-gnu/libfontconfig.a /usr/lib/x86_64-linux-gnu/libfreetype.a /usr/lib/x86_64-linux-gnu/libXinerama.a \
			    /usr/lib/x86_64-linux-gnu/libXrandr.a  /usr/lib/x86_64-linux-gnu/libXext.a \
			    /usr/lib/x86_64-linux-gnu/librt.a /usr/lib/x86_64-linux-gnu/libz.a /usr/lib/x86_64-linux-gnu/libdl.a  \
			    /usr/lib/x86_64-linux-gnu/libselinux.a /usr/lib/x86_64-linux-gnu/libresolv.a /usr/lib/x86_64-linux-gnu/libffi.a /usr/lib/x86_64-linux-gnu/libpcre.a \
			    /usr/lib/x86_64-linux-gnu/libxcb.a /usr/lib/x86_64-linux-gnu/libpixman-1.a \
			    /usr/lib/x86_64-linux-gnu/libpng16.a /usr/lib/x86_64-linux-gnu/libxcb-shm.a /usr/lib/x86_64-linux-gnu/libxcb-render.a /usr/lib/x86_64-linux-gnu/libXrender.a \
			    /usr/lib/x86_64-linux-gnu/libdbus-1.a /usr/lib/x86_64-linux-gnu/libthai.a  \
			    /usr/lib/x86_64-linux-gnu/libexpat.a /usr/lib/x86_64-linux-gnu/libuuid.a /usr/lib/x86_64-linux-gnu/libblkid.a /usr/lib/x86_64-linux-gnu/libpcre2-8.a \
			    /usr/lib/x86_64-linux-gnu/libXau.a /usr/lib/x86_64-linux-gnu/libXdmcp.a  /usr/lib/x86_64-linux-gnu/libdatrie.a \
			  /usr/lib/x86_64-linux-gnu/liblzma.a 
# Rules start here

$(TARGET):$(OBJS)
	 $(CXX) $(CXXFLAGS) -o $(TARGET)  $^    $(LIBDYN)

.o:.cpp
	$(CC) $(CXXFLAGS) -c $<

clean:
	rm -f $(TARGET) *.o
	
	

install: $(TARGET)
	mv progen3d $(DESTDIR)
	mkdir $(DESTDIR)/textures
	cp ./textures/*.png $(DESTDIR)/textures
	cp *.grammar $(DESTDIR)
	cp ./snap/gui/progen3d.desktop $(DESTDIR)
	cp ./snap/gui/progen3d.png $(DESTDIR)
