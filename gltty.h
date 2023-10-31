#ifndef GLTTY_H_
#define GLTTY_H_

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// There are only 0x7e - 0x20 + 1 = 95 printible ascii chars.
#define ASCII_BEGIN 0x20
#define ASCII_END 0x7e
#define ASCII_COUNT ASCII_END - ASCII_BEGIN + 1

typedef struct {
    int width;
    int height;
    void *data;
} Bitmap;

typedef struct {
    int left;
    int top;
} Bearing;

typedef struct {
    char c;
    int advance;
    Bearing bearing;
    Bitmap bitmap;
} Character;



extern void fatal(const char *format, ...);

// freetype
extern FT_Face init_ft_and_load_font(FT_Library *ft, const char *font_path, int font_size);
extern void load_ascii_chars(FT_Face face, Character *ascii_chars, int *atlas_width, int *atlas_height);

// gl

extern GLFWwindow *init_gl_and_window(int screen_width, int screen_height);
extern GLuint create_shader_program(const char *vertex_path, const char *fragment_path);
void ortho(float *m, float left, float right, float bottom, float top, float near, float far);

#endif // GLTTY_H_
