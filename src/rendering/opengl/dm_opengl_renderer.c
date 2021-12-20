#include "dm_opengl_renderer.h"

#if DM_OPENGL

#include "dm_logger.h"
#include "platform/dm_platform.h"

bool dm_renderer_init_impl(dm_renderer_data* renderer_data)
{
    if (!dm_platform_init_opengl())
    {
        return false;
    }

    DM_LOG_INFO("OpenGL Info:");
    DM_LOG_INFO("       Vendor  : %s", glGetString(GL_VENDOR));
    DM_LOG_INFO("       Renderer: %s", glGetString(GL_RENDERER));
    DM_LOG_INFO("       Version : %s", glGetString(GL_VERSION));

#if DM_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
#endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, renderer_data->width, renderer_data->height);

    return true;
}

void dm_renderer_shutdown_impl()
{
    dm_platform_shutdown_opengl();
}

bool dm_renderer_resize_impl(int new_width, int new_height)
{
    glViewport(0, 0, new_width, new_height);

    return true;
}

void dm_renderer_begin_scene_impl(dm_renderer_data* renderer_data)
{
    glClearColor(
        renderer_data->clear_color.x,
        renderer_data->clear_color.y,
        renderer_data->clear_color.z,
        renderer_data->clear_color.w
    );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // testing TODO: remove

    float vertices[] =
    {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.0f, 0.5f, 0.0f
    };

    GLuint vbo, vao;
    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
        "}\0";
    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;"

        "void main()"
        "{"
            "FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);"
        "} \0";

    GLuint vertex_shader;
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertex_shader);

    int  success;
    char infoLog[512];
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(vertex_shader, 512, NULL, infoLog);
        DM_LOG_ERROR("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n %s", infoLog);
    }

    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        DM_LOG_ERROR("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n %s", infoLog);
    }

    GLuint shader_program;
    shader_program = glCreateProgram();

    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragmentShader);
    glLinkProgram(shader_program);

    glUseProgram(shader_program);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragmentShader);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void dm_renderer_end_scene_impl(dm_renderer_data* renderer_data)
{
    dm_camera_update_view_proj(&renderer_data->camera);

    dm_platform_swap_buffers();
}

#endif