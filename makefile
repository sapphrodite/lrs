TARGET := gltest
BUILD_DIR := build/
SOURCE_DIR := src/
SOURCE_SUBDIRS := engine/
INCLUDE_DIRS :=
LIBS := SDL2 GLEW GL png ogg opus
CXXFLAGS := --std=c++20 -Wall -Wextra -g3
SUBMAKES :=
TARGET_DEPS :=


LDFLAGS := $(CXXFLAGS) $(addprefix -l, $(LIBS))
CXXFLAGS += -MMD -MP -I$(SOURCE_DIR) $(addprefix -I, $(INCLUDE_DIRS))
THIS_MAKEFILE := $(firstword $(MAKEFILE_LIST))
SOURCE_DIRS := $(SOURCE_DIR) $(addprefix $(SOURCE_DIR), $(SOURCE_SUBDIRS))
SOURCES := $(foreach d, $(SOURCE_DIRS), $(wildcard $(d)*.cpp))
OBJECTS := $(patsubst $(SOURCE_DIR)%.cpp, $(BUILD_DIR)%.o, $(SOURCES))
DEPFILES := $(patsubst %.o,%.d,$(OBJECTS))
-include $(DEPFILES)
include $(SUBMAKES)


$(BUILD_DIR)%.o: $(SOURCE_DIR)%.cpp $(THIS_MAKEFILE)
	@mkdir -p "$(dir $@)"
	@echo "[CXX] $(notdir $@)"
	@$(CXX) $(CXXFLAGS) -c "$<" -o "$@"

$(TARGET): $(OBJECTS)
	@echo "[LD] $(notdir $@)"
	@$(CXX) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

all: $(TARGET)

clean:
	rm -rf $(TARGET)
	rm -rf $(BUILD_DIR)

.PHONY: clean all
