SRC_DIR := src/
SHADER_DIR := shaders/

INPUT_FILE := $(SRC_DIR)main.c 
TARGET := draw_tha_Triangle
LDFLAGS := -lglfw -lvulkan
CFLAGS := -g
NDEBUG := -DNDEBUG

VERT_INPUT := $(SHADER_DIR)shader.vert
FRAG_INPUT := $(SHADER_DIR)shader.frag

VERT_OUTPUT := $(SHADER_DIR)vert.spv
FRAG_OUTPUT := $(SHADER_DIR)frag.spv

debug: $(SRC_DIR)main.c 
	cc $(CFLAGS) $(INPUT_FILE) -o $(TARGET) $(LDFLAGS)

release: $(SRC_DIR)main.c $(BUILD_DIR)
	cc $(NDEBUG) $(INPUT_FILE) -o $(TARGET) $(LDFLAGS)

test: $(TARGET) 
	./$(TARGET)   

shader: $(VERT_INPUT) $(FRAG_INPUT)
	glslc $(VERT_INPUT) -o $(VERT_OUTPUT)
	glslc $(FRAG_INPUT) -o $(FRAG_OUTPUT)

memtest:
	valgrind ./$(TARGET)

clean: 
	rm -rf $(TARGET)
	rm $(SHADER_DIR)vert.spv
	rm $(SHADER_DIR)frag.spv
