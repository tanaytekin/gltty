#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define ASCII_BEGIN 0x20
#define ASCII_END 0x7e
#define ASCII_COUNT ASCII_END - ASCII_BEGIN + 1

#define TTY_COLUMNS 80
#define TTY_ROWS 24
#define TTY_COUNT (TTY_ROWS * TTY_ROWS)

typedef struct {
    int width;
    int height;
    void *data;
} Bitmap;

typedef struct {
    int x;
    int y;
} Bearing;

typedef struct {
    char c;
    Bearing bearing;
    Bitmap bitmap;
    int width;
    int height;
    float u1, u2, v1, v2;
} Character;

typedef struct {
    Character chars[ASCII_COUNT];
    int char_width;
    int char_height;
    int atlas_width;
    int atlas_height;
} Font;

typedef struct {
    GLuint program;
    GLuint texture;
    GLuint vao;
    GLuint vbo;
    float projection[16];
    float *vertices;
    size_t vertices_size;
} RenderContext;

typedef struct {
    char c;
    int x;
    int y;
} Cell;

typedef struct {
    Cell *cells;
    size_t capacity;
    size_t count;
} Cells;

typedef struct {
    size_t cursor;
    size_t cursor_x;
    size_t cursor_y;
    int columns;
    int rows;
} Terminal;

static const char *vertex_src = {
"#version 330 core\n"
"layout (location = 0) in vec4 a_vert;\n"
"out vec2 v_tex_coords;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"    gl_Position = projection * vec4(a_vert.xy, 0, 1.0);\n"
"    v_tex_coords = a_vert.zw;\n"
"}\n"
};

static const char *fragment_src = {
"#version 330 core\n"
"in vec2 v_tex_coords;\n"
"out vec4 frag_color;\n"
"uniform sampler2D text;\n"
"void main()\n"
"{\n"
"    frag_color = vec4(1.0, 1.0, 1.0, texture(text, v_tex_coords).r);\n"
"}\n"
};

static void fatal(const char *format, ...)
{
    fprintf(stderr, "ERROR: ");
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

static void log_info(const char *format, ...)
{
    printf("INFO: ");
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
    printf("\n");
    fflush(stdout);
}

static void font_init(Font *font, const char *font_path, int font_size)
{
    FT_Library ft;
    FT_Error error;
    error = FT_Init_FreeType(&ft);
    if (error)
        fatal("Failed to init FreeType2.");
    FT_Face face;
    error = FT_New_Face(ft, font_path, 0, &face);
    if (error)
        fatal("Failed to load font: %s", font_path);
    if (!FT_IS_FIXED_WIDTH(face))
        fatal("Font should be a monospace font.");
    error = FT_Set_Char_Size(
          face,    /* handle to face object         */
          0,       /* char_width in 1/64 of points  */
          font_size * 64,   /* char_height in 1/64 of points */
          96,     /* horizontal device resolution  */
          96);    /* vertical device resolution    */
    if (error)
        fatal("Failed to set font size");

    int atlas_width = 0;

    for (int i = ASCII_BEGIN; i <= ASCII_END; i++) {
        error = FT_Load_Char(face, i, FT_LOAD_RENDER);
        if (error)
            fatal("Failed to load char: %c", i);

        Character *c = &font->chars[i - ASCII_BEGIN];
        c->c = i;
        c->bearing.x = face->glyph->bitmap_left;
        c->bearing.y = face->glyph->bitmap_top;
        
        c->bitmap.width = face->glyph->bitmap.width;
        atlas_width += c->bitmap.width;
        c->bitmap.height = face->glyph->bitmap.rows;
        c->width = (face->glyph->metrics.width >> 6);
        c->height = (face->glyph->metrics.height >> 6);
        
        const size_t size = c->bitmap.width * c->bitmap.height;
        c->bitmap.data = malloc(size);
        memcpy(c->bitmap.data, face->glyph->bitmap.buffer, size);
    }

    font->char_width = face->glyph->advance.x >> 6;
    font->char_height= (face->size->metrics.ascender - face->size->metrics.descender)>> 6;
    font->atlas_width = atlas_width;

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

static void check_shader_errors(GLuint shader, GLenum type)
{
    GLint success = 0;
    GLint info_log_length = 0;
    char *info_log = NULL;

    if (type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER) {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);
            info_log = malloc(info_log_length);
            info_log[info_log_length - 1] = '\0';
            glGetShaderInfoLog(shader, info_log_length, NULL, info_log);
            if (type == GL_VERTEX_SHADER)
                fatal("Vertex shader error: \n%s", info_log);
            else
                fatal("Fragment shader error: \n%s", info_log);
        }
    } else if(type == GL_PROGRAM) {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);
            info_log = malloc(info_log_length);
            info_log[info_log_length - 1] = '\0';
            glGetProgramInfoLog(shader, info_log_length, NULL, info_log);
            fatal("Shader program linking error: \n%s", info_log);
        }
    }
}

static GLuint create_shader_program(const char *vertex_src,
                                    const char *fragment_src)
{
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, (const GLchar * const *) &vertex_src, NULL);
    glCompileShader(vertex);
    check_shader_errors(vertex, GL_VERTEX_SHADER);

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, (const GLchar * const *) &fragment_src, NULL);
    glCompileShader(fragment);
    check_shader_errors(fragment, GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    check_shader_errors(program, GL_PROGRAM);

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

static void ortho(float *m, float left, float right, float bottom, float top,
                  float near, float far)
{
    memset(m, 0, 16 * sizeof(float));
    m[0] = 2 / (right - left);
    m[5] = 2 / (top - bottom);
    m[10] = 2 / (near - far);
    m[15] = 1;
    m[12] = (right + left) / (left - right);
    m[13] = (top + bottom) / (bottom - top);
    m[14] = (far + near) / (near - far);
}


static void init_font_texture_atlas(RenderContext *rc, Font *font)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &rc->texture);
    glBindTexture(GL_TEXTURE_2D, rc->texture);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RED,
                 font->atlas_width,
                 font->char_height,
                 0,
                 GL_RED,
                 GL_UNSIGNED_BYTE,
                 NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    {
        float x = 0.0f;
        for (int i = ASCII_BEGIN; i <= ASCII_END; i++) {
            Character *c = &font->chars[i - ASCII_BEGIN];
            float x_offset = x;
            c->u1 = x / (float) font->atlas_width; 
            x += c->bitmap.width;
            c->u2 = x / (float) font->atlas_width;
            c->v1 = 0.0f;
            c->v2 = (float)c->bitmap.height / (float)font->char_height;
            glTexSubImage2D(GL_TEXTURE_2D,
                            0,
                            x_offset,
                            0,
                            c->bitmap.width,
                            c->bitmap.height,
                            GL_RED,
                            GL_UNSIGNED_BYTE,
                            c->bitmap.data
                            );
            free(c->bitmap.data);
            c->bitmap.data = NULL;
        }
    }
}


static void render_init(RenderContext *rc, Font *font, int screen_width, int screen_height)
{
    rc->program = create_shader_program(vertex_src, fragment_src);
    ortho(rc->projection, 0.0f, screen_width, 0.f, screen_height, -100.0f, 100.0f);
    init_font_texture_atlas(rc, font);
    glUseProgram(rc->program);
    glUniformMatrix4fv(glGetUniformLocation(rc->program, "projection"), 1, GL_FALSE, rc->projection);

    rc->vertices_size = TTY_ROWS * TTY_COLUMNS * 4 * 6 * sizeof(float);
    rc->vertices = malloc(rc->vertices_size);

    glGenVertexArrays(1, &rc->vao);
    glGenBuffers(1, &rc->vbo);

    glBindVertexArray(rc->vao);
    glBindBuffer(GL_ARRAY_BUFFER, rc->vbo);
    glBufferData(GL_ARRAY_BUFFER, rc->vertices_size, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);
}

static void render(RenderContext *rc, Font *font, Cells *cells)
{
    for (size_t k = 0; k < cells->count; k++) {
            const Cell *cell = &cells->cells[k];
            const Character *c = &font->chars[cell->c - ASCII_BEGIN];
            const int i = cell->x;
            const int j = cell->y;
            float xc = i * font->char_width + c->bearing.x;
            float yc = j * font->char_height +
                       font->char_height/4.0f - c->height + c->bearing.y;
            float u1 = c->u1;
            float u2 = c->u2;
            float v1 = c->v1;
            float v2 = c->v2;
            float w = c->width;
            float h = c->height;
            float vertices_per_char[] = {
                xc,     yc,   u1, v2,
                xc+w,   yc,   u2, v2,
                xc+w, yc+h,   u2, v1,

                xc+w, yc+h,   u2, v1,
                xc,   yc+h,   u1, v1,
                xc,     yc,   u1, v2
            };

            memcpy(rc->vertices + k * sizeof(vertices_per_char) / sizeof(float), vertices_per_char, sizeof(vertices_per_char));
    }
    glBufferSubData(GL_ARRAY_BUFFER, 0, cells->count * 6 * 4 * sizeof(float), rc->vertices);
    glBindVertexArray(rc->vao);
    glUseProgram(rc->program);
    glActiveTexture(GL_TEXTURE0);
    glDrawArrays(GL_TRIANGLES, 0, 6 * cells->count);
}

static void init_cells(Cells *cells)
{
    memset(cells, 0, sizeof(Cells));
    cells->capacity = TTY_COUNT;
    cells->cells = malloc(sizeof(Cell) * cells->capacity);
    if (cells->cells == NULL)
        fatal("Malloc failed.");
}

static void push_cell(Cells *cells, char c, int x, int y)
{
    if (cells->count >= cells->capacity)
        fatal("Cells capacity overflow");
    Cell *cell = &cells->cells[cells->count];
    cell->c = c;
    cell->x = x;
    cell->y = TTY_ROWS - 1 - y;
    cells->count++;
}

static void init_terminal(Terminal *t)
{
    memset(t, 0, sizeof(Terminal));
    t->columns = TTY_COLUMNS;
    t->rows = TTY_ROWS;
}

static inline bool is_printable_ascii(char c)
{
    return (c >= ASCII_BEGIN) && (c <= ASCII_END);
}
static void recalculate_cursor(Terminal *t)
{
    t->cursor_y += t->cursor_x / t->columns;
    t->cursor_x = t->cursor_x % t->columns;
}
static void write_to_terminal(Terminal *t, Cells *c, void *buf, size_t size)
{
    char *b = buf;
    for(size_t i = 0; i < size; i++) {
        if (is_printable_ascii(b[i])) {
            recalculate_cursor(t);
            push_cell(c, b[i], t->cursor_x, t->cursor_y);
            t->cursor_x++;
        } else if (b[i] == '\n') {
            t->cursor_x = 0;
            t->cursor_y++;
        }
    }
}

int main(void)
{
    const char *font_path = "/usr/share/fonts/TTF/JetBrainsMono-Regular.ttf";
    const int font_size = 16;
    
    if (glfwInit() != GLFW_TRUE)
        fatal("Failed to init GLFW.");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);


    Font font = {0};
    font_init(&font, font_path, font_size);

    const int screen_width = font.char_width * TTY_COLUMNS;
    const int screen_height  = font.char_height * TTY_ROWS;
 
    GLFWwindow *window = glfwCreateWindow(screen_width, screen_height, "gltty", NULL, NULL);
    if (!window)
        fatal("Failed to create GLFW window.");
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        fatal("Failed to load GLAD.");

    RenderContext rc = {0};
    render_init(&rc, &font, screen_width, screen_height);

    Cells cells = {0};
    init_cells(&cells);

    char *buf = malloc(1024);
    strcpy(buf, vertex_src);

    Terminal terminal;
    init_terminal(&terminal);
    write_to_terminal(&terminal,  &cells, buf, strlen(buf));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        render(&rc, &font, &cells);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    return 0;
}

