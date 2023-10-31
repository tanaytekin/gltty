#include "gltty.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

GLFWwindow *init_gl_and_window(int screen_width, int screen_height)
{
    const bool resizable = false;

    if (!glfwInit()) {
        fatal("Failed to init GLFW3.");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE: GLFW_FALSE);


    GLFWwindow *window = glfwCreateWindow(screen_width, screen_height, "gltty", NULL, NULL);
    if (!window) {
        fatal("Failed to create GLFW3 window.");
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fatal("Failed to init GLAD.");
    }

    return window;
}



static char *read_file_to_str(const char *path)
{
    FILE *file = fopen(path, "r");
    if (!file) {
        goto ret_fail;
    }

    size_t size;
    fseek(file, 0L, SEEK_END);
    size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    char *str;
    str = malloc(size + 1);
    str[size] = '\0';

    size_t ret = fread(str, 1, size, file);
    if (ret != size) {
        goto ret_fail;
    }

    if (fclose(file) != 0) {
        fatal("Failed to close file: %s", path);
    }

    return str;
ret_fail:
        fatal("Failed to read file: %s", path);
    return NULL;
}

void check_shader_errors(GLuint shader, GLenum type)
{
    GLint success = 0;
    GLint info_log_length = 0;
    char *info_log = NULL;

    if (type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER) {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);
            info_log = malloc(info_log_length);
            info_log[info_log_length-1] = '\0';

            glGetShaderInfoLog(shader, info_log_length, NULL, info_log);
            if (type == GL_VERTEX_SHADER) {
                fatal("Vertex shader error: \n%s", info_log);
            } else {
                fatal("Fragment shader error: \n%s", info_log);
            }
        }
    } else if(type == GL_PROGRAM) {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);
            info_log = malloc(info_log_length);
            info_log[info_log_length-1] = '\0';

            glGetProgramInfoLog(shader, info_log_length, NULL, info_log);
            fatal("Shader program linking error: \n%s", info_log);
        }
    } else {
        fatal("Unsupported shader type.");
    }
}

GLuint create_shader_program(const char *vertex_path, const char *fragment_path)
{
    char *vertex_str = read_file_to_str(vertex_path);
    char *fragment_str = read_file_to_str(fragment_path);

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, (const GLchar * const *) &vertex_str, NULL);
    glCompileShader(vertex);
    check_shader_errors(vertex, GL_VERTEX_SHADER);

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, (const GLchar * const *) &fragment_str, NULL);
    glCompileShader(fragment);
    check_shader_errors(fragment, GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    check_shader_errors(program, GL_PROGRAM);

    free(vertex_str);
    free(fragment_str);

    return program;
}

void ortho(float *m, float left, float right, float bottom, float top, float near, float far)
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
