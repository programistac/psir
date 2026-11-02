CC := gcc

TARGET := psx_demo
BUILD_DIR := build

CFLAGS := -std=c89

SOURCES := \
	main.c \
	platform/platform_sdl.c \
	gpu/gpu.c \
	gte/gte.c \
	math/math3d.c \
	gpu/shader.c \
	gpu/framebuffer.c \
	gpu/texture.c \
	renderer/renderer.c

OBJECTS := $(SOURCES:%.c=$(BUILD_DIR)/%.o)

.PHONY: clean

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) -lGL -lGLEW -lSDL2 -lm -lpng -lSDL2_image

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
