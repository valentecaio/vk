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
LFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
CFLAGS = -Wall -std=c++17 $(OPENMP_FLAG) $(NDEBUG_FLAG) \
 -I lib/ \
 -I lib/glm-1.0.1/

# Main executable
EXE = $(BUILDDIR)/vk


### Automatic variables ###

# Source files in src/ directory
SRC := $(wildcard $(SRCDIR)/*.cpp)

# Object files in build/ directory
OBJ = $(SRC:.cpp=.o)
OBJ := $(subst $(SRCDIR)/, $(BUILDDIR)/, $(OBJ))


### Rules ###

# Build
all: $(EXE) shaders

# Compile shaders (from SHADERDIR to BUILDDIR)
shaders:
	@mkdir -p $(dir $(BUILDDIR))
	cd $(SHADERDIR) && bash compile.sh $(CURDIR)/$(BUILDDIR)

# Compile source files (from SRCDIR to BUILDDIR)
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	g++ $(CFLAGS) -c -o $@ $< $(LFLAGS)

# Link
$(EXE): $(OBJ)
	g++ $(CFLAGS) -o $@ $^ $(LFLAGS)

clean:
	rm -f *.o $(BUILDDIR)/* $(SHADERDIR)/*.spv

# print variables
print:
	@echo CFLAGS = $(CFLAGS)
	@echo LFLAGS = $(LFLAGS)
	@echo EXE = $(EXE)
	@echo SRC = $(SRC)
	@echo OBJ = $(OBJ)

# compile and run in debug mode
run: $(EXE)
	./$(EXE)

# run in multi-threaded mode
run_mt:
	@$(MAKE) run OPENMP=1

.PHONY: all clean print run run_mt shaders

# EOF