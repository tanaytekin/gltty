#include "gltty.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


#include <string.h>

static Character ascii_chars[ASCII_COUNT];

void fatal(const char *format, ...)
{
    fprintf(stderr, "ERROR: ");
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

int main(void)
{
    const char *font_path = "/usr/share/fonts/TTF/JetBrainsMono-Regular.ttf";
    const int font_size = 16;

    FT_Library ft;
    FT_Face face = init_ft_and_load_font(&ft, font_path, font_size);

    const int font_width = face->size->metrics.max_advance>>6;
    const int font_height = face->size->metrics.height>>6;


    int ascii_atlas_width, ascii_atlas_height;
    load_ascii_chars(face, ascii_chars, &ascii_atlas_width, &ascii_atlas_height);

    printf("ascii_atlas_width: %d, ascii_atlas_height: %d\n",
            ascii_atlas_width, ascii_atlas_height);

    const int n_columns = 80;
    const int n_rows = 24;

    const int screen_width = n_columns * font_width;
    const int screen_height = n_rows * font_height;

    GLFWwindow *window = init_gl_and_window(screen_width, screen_height);

    GLuint shader_program = create_shader_program("vs.glsl", "fs.glsl");

    float start_x = 300.0f;
    float start_y = 100.0f;
    float vertices[] = {
        start_x, start_y,
        start_x + 100.f, start_y,
        start_x + 100.f, start_y + 300.0f,
    };

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);


    float projection[16];
    ortho(projection, 0.0f, 800.0f, 0.0f, 600.0f, -100.0f, 100.0f);

    glUseProgram(shader_program);
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, projection);


    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.2f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    return 0;
}
