EXE=island_generator
# Define the directory for the library files
LIBDIR=CSCIx239_lib

# Main target
all: $(EXE)

# Msys/MinGW
ifeq "$(OS)" "Windows_NT"
# Add -I$(LIBDIR) to include path
CFLG=-O3 -Wall -DUSEGLEW -I$(LIBDIR)
LIBS=-lglfw3 -lglew32 -lglu32 -lopengl32 -lm
CLEAN=del -f *.exe *.o *.a $(LIBDIR)\*.o
else
# OSX
ifeq "$(shell uname)" "Darwin"
# Add -I$(LIBDIR) to include path
CFLG=-O3 -Wall -Wno-deprecated-declarations -DAPPLE_GL4 -DUSEGLEW -I$(LIBDIR)
LIBS=-lglfw -lglew -framework Cocoa -framework OpenGL -framework IOKit
# Linux/Unix/Solaris
else
# Add -I$(LIBDIR) to include path
CFLG=-O3 -Wall -I$(LIBDIR)
LIBS=-lglfw -lGLU -lGL -lm
endif
# OSX/Linux/Unix/Solaris
CLEAN=rm -f $(EXE) *.o *.a $(LIBDIR)/*.o
endif

# Define library source files with their directory prefix
LIBSOURCES=$(LIBDIR)/fatal.c $(LIBDIR)/errcheck.c $(LIBDIR)/print.c $(LIBDIR)/axes.c $(LIBDIR)/loadtexbmp.cpp $(LIBDIR)/loadobj.c $(LIBDIR)/projection.c $(LIBDIR)/lighting.c $(LIBDIR)/elapsed.c $(LIBDIR)/fps.c $(LIBDIR)/shader.c $(LIBDIR)/noise.c $(LIBDIR)/cube.c $(LIBDIR)/sphere.c $(LIBDIR)/cone.c $(LIBDIR)/cylinder.c $(LIBDIR)/torus.c $(LIBDIR)/icosahedron.c $(LIBDIR)/teapot.c $(LIBDIR)/mat4.c $(LIBDIR)/initwin.c

# Define library object files in the library directory
LIBOBJECTS=$(LIBSOURCES:.c=.o)
LIBOBJECTS:=$(LIBOBJECTS:.cpp=.o)

# Dependencies
# Header file dependency is now within the library directory
$(LIBDIR)/CSCIx239.h: $(LIBDIR)/mat4.h
island_generator.o: island_generator.cpp $(LIBDIR)/CSCIx239.h PerlinNoise.hpp

# Create archive (uses objects from the library directory)
CSCIx239.a: $(LIBOBJECTS)
	ar -rcs $@ $^

# Compile rules for library source files in the subdirectory
# This pattern rule compiles a .c file in the LIBDIR and creates a .o file in the same LIBDIR
$(LIBDIR)/%.o: $(LIBDIR)/%.c
	gcc -c $(CFLG) $< -o $@

# This pattern rule compiles a .cpp file in the LIBDIR and creates a .o file in the same LIBDIR
$(LIBDIR)/%.o: $(LIBDIR)/%.cpp
	g++ -c $(CFLG) $< -o $@

# Compile rule for the main program's source file (uses the header from the lib dir)
.cpp.o:
	g++ -c $(CFLG) $<

# Link
island_generator:island_generator.o CSCIx239.a
	g++ $(CFLG) -o $@ $^ $(LIBS)

# Clean
clean:
	$(CLEAN)
