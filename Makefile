# If RACK_DIR is not defined when calling the Makefile, default to two directories above
RACK_DIR ?= ../..

libDSPFilters := DSPFilters/shared/DSPFilters/build/libDSPFilters.a

# FLAGS will be passed to both the C and C++ compiler
FLAGS +=
CFLAGS +=
CXXFLAGS += -Wall -I./DSPFilters/shared/DSPFilters/include/DspFilters -I./DSPFilters/shared/DSPFilters/include # -static
OBJECTS += $(libDSPFilters)
DEPS += $(libDSPFilters)

$(libDSPFilters):
	git submodule update --init --recursive
	cd DSPFilters/shared/DSPFilters && mkdir -p build
	cd DSPFilters/shared/DSPFilters/build && $(CMAKE) ..
	cd DSPFilters/shared/DSPFilters/build && $(MAKE)

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.
LDFLAGS += -L$(libDSPFilters)
#LDFLAGS += -static 

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp)

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)
DISTRIBUTABLES += $(OBJECTS)

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk
