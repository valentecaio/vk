### Variables ###

# debug
NDEBUG := # set to 1 to generate a release build
ifdef NDEBUG
	NDEBUG_FLAG = -g -DNDEBUG
endif

# OpenMP
OPENMP_FLAG :=
ifdef OPENMP
	OPENMP_FLAG = -fopenmp -DOPENMP
endif

# Source and build directory, compiler and compiler tags
SRCDIR := src
BUILDDIR := build
SHADERDIR := shaders
LFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -Llib -lbase
CFLAGS = -Wall -std=c++17 $(OPENMP_FLAG) $(NDEBUG_FLAG) \
 -I include/ \
 -I include/ktx/include/ \

# Main executables
EXE1 = $(BUILDDIR)/tutorial
EXE2 = $(BUILDDIR)/shadow_mapping


### Automatic variables ###

# All cpp files from src/ directory
# SRC := $(wildcard $(SRCDIR)/*.cpp)

# Object files in build/ directory
# OBJ = $(SRC:.cpp=.o)
# OBJ := $(subst $(SRCDIR)/, $(BUILDDIR)/, $(OBJ))


### Rules ###

# Build
all: $(EXE1) ${EXE2} shaders


# Compile shaders (from SHADERDIR to BUILDDIR)
shaders:
	@mkdir -p $(dir $(BUILDDIR))
	cd $(SHADERDIR) && bash compile.sh $(CURDIR)/$(BUILDDIR)


# Compile source files (from SRCDIR to BUILDDIR)
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	g++ $(CFLAGS) -c -o $@ $< $(LFLAGS)


# tutorial
$(EXE1): $(BUILDDIR)/tutorial.o
	g++ $(CFLAGS) -o $@ $^ $(LFLAGS)

tutorial: $(EXE1) shaders
	./$(EXE1)


# shadow_mapping
$(EXE2): $(BUILDDIR)/shadow_mapping.o
	g++ $(CFLAGS) -o $@ $^ $(LFLAGS)

shadow_mapping: $(EXE2) shaders
	./$(EXE2)


clean:
	rm -f *.o $(BUILDDIR)/* $(SHADERDIR)/*.spv


# print variables
print:
	@echo CFLAGS = $(CFLAGS)
	@echo LFLAGS = $(LFLAGS)

.PHONY: all clean print run run_mt shaders

# EOF