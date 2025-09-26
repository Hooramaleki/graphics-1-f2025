#include "Window.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstddef>
#include <cmath>
#include <iostream>

struct vec2 { float x, y; };
struct vec3 { float x, y, z; };

struct Vertex
{
    vec2 pos;
    vec3 col;
};


//rainbow triangle : each corner is a different color
//left down -> r
//right down -> g
//top -> b
static const Vertex vertices_rainbow[3] =
{
    { { -0.15f, -0.1f }, { 1.0f, 0.0f, 0.0f } },
    { {  0.15f, -0.1f }, { 0.0f, 1.0f, 0.0f } },
    { {   0.f,  0.15f }, { 0.0f, 0.0f, 1.0f } }
};


//white triangle: all corners are white
static const Vertex vertices_white[3] =
{
    { { -0.15f, -0.1f }, { 1.0f, 1.0f, 1.0f } },
    { {  0.15f, -0.1f }, { 1.0f, 1.0f, 1.0f } },
    { {  0.0f,  0.15f }, { 1.0f, 1.0f, 1.0f } }
};


//base triangle: reused for color-changing, moving, and rotating ones
static const Vertex vertices_base[3] =
{
    { { -0.15f, -0.1f }, { 1.0f, 1.0f, 1.0f } },
    { {  0.15f, -0.1f }, { 1.0f, 1.0f, 1.0f } },
    { {  0.0f,   0.15f }, { 1.0f, 1.0f, 1.0f } }
};

//shaders
static const char* vertex_shader_text =
"#version 330\n"
"in vec3 vCol;\n"
"in vec2 vPos;\n"
"out vec3 color;\n"
"uniform mat4 uModel;\n"
"void main()\n"
"{\n"
"    vec4 p = vec4(vPos, 0.0, 1.0);\n"
"    gl_Position = uModel * p;\n"
"    color = vCol;\n"
"}\n";

static const char* fragment_shader_text =
"#version 330\n"
"in vec3 color;\n"
"out vec4 fragment;\n"
"uniform vec3 u_color;\n"
"void main()\n"
"{\n"
"    fragment = vec4(color * u_color, 1.0);\n"
"}\n";



//matrix helpers

//creates an identity 4×4 matrix (all zeros except 1s on the diagonal)
static void mat4_identity(float m[16])
{
    for (int i = 0; i < 16; ++i) m[i] = 0.0f;
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

//builds a translation matrix that moves points by (tx, ty, tz).
static void mat4_translate(float m[16], float tx, float ty, float tz)
{
    mat4_identity(m);
    m[12] = tx;
    m[13] = ty;
    m[14] = tz;
}

//builds a rotation matrix that rotates points around the z axis
static void mat4_rotate_z(float m[16], float radians)
{
    float c = std::cos(radians);
    float s = std::sin(radians);
    m[0] = c;  m[1] = s;  m[2] = 0.0f; m[3] = 0.0f;
    m[4] = -s; m[5] = c;  m[6] = 0.0f; m[7] = 0.0f;
    m[8] = 0.0f; m[9] = 0.0f; m[10] = 1.0f; m[11] = 0.0f;
    m[12] = 0.0f; m[13] = 0.0f; m[14] = 0.0f; m[15] = 1.0f;
}

static void mat4_mul(const float A[16], const float B[16], float out[16])
{
    for (int col = 0; col < 4; ++col)
    {
        for (int row = 0; row < 4; ++row)
        {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k)
                sum += A[k * 4 + row] * B[col * 4 + k];
            out[col * 4 + row] = sum;
        }
    }
}



int main()
{
    // window setup
    CreateWindow(800, 800, "graphics 1");

    // compile shaders
    const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShader(vertex_shader);

    const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShader(fragment_shader);

    // link program
    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    // locations in shaders
    const GLint vpos_location = glGetAttribLocation(program, "vPos");
    const GLint vcol_location = glGetAttribLocation(program, "vCol");
    const GLint u_color_loc = glGetUniformLocation(program, "u_color");
    const GLint u_model_loc = glGetUniformLocation(program, "uModel");

    // setup rainbow triangle buffers
    GLuint vb_rainbow = 0;
    glGenBuffers(1, &vb_rainbow);
    glBindBuffer(GL_ARRAY_BUFFER, vb_rainbow);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_rainbow), vertices_rainbow, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint va_rainbow = 0;
    glGenVertexArrays(1, &va_rainbow);
    glBindVertexArray(va_rainbow);
    glBindBuffer(GL_ARRAY_BUFFER, vb_rainbow);
    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(vcol_location);
    glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, col));
    glBindVertexArray(0);

    // setup white triangle buffers
    GLuint vb_white = 0;
    glGenBuffers(1, &vb_white);
    glBindBuffer(GL_ARRAY_BUFFER, vb_white);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_white), vertices_white, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint va_white = 0;
    glGenVertexArrays(1, &va_white);
    glBindVertexArray(va_white);
    glBindBuffer(GL_ARRAY_BUFFER, vb_white);
    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(vcol_location);
    glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, col));
    glBindVertexArray(0);

    // setup base triangle buffers (for color-change, moving, rotating)
    GLuint vb_base = 0;
    glGenBuffers(1, &vb_base);
    glBindBuffer(GL_ARRAY_BUFFER, vb_base);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_base), vertices_base, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint va_base = 0;
    glGenVertexArrays(1, &va_base);
    glBindVertexArray(va_base);
    glBindBuffer(GL_ARRAY_BUFFER, vb_base);
    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(vcol_location);
    glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, col));
    glBindVertexArray(0);

    // y positions for stacking triangles
    const float yOffsets[5] = { 0.8f, 0.4f, 0.0f, -0.4f, -0.8f };

    glUseProgram(program);
    glUniform3f(u_color_loc, 1.0f, 1.0f, 1.0f);

    // main loop
    while (!WindowShouldClose())
    {
        float t = (float)glfwGetTime();

        // background color
        glClearColor(239.0f / 255.0f, 136.0f / 255.0f, 190.0f / 255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);

        // 1. plain white triangle
        {
            float model[16];
            mat4_translate(model, 0.0f, yOffsets[0], 0.0f);
            glUniformMatrix4fv(u_model_loc, 1, GL_FALSE, model);
            glUniform3f(u_color_loc, 1.0f, 1.0f, 1.0f);
            glBindVertexArray(va_white);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // 2. rainbow triangle
        {
            float model[16];
            mat4_translate(model, 0.0f, yOffsets[1], 0.0f);
            glUniformMatrix4fv(u_model_loc, 1, GL_FALSE, model);
            glUniform3f(u_color_loc, 1.0f, 1.0f, 1.0f);
            glBindVertexArray(va_rainbow);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // 3. color changing triangle (uses time to change)
        {
            float model[16];
            mat4_translate(model, 0.0f, yOffsets[2], 0.0f);
            glUniformMatrix4fv(u_model_loc, 1, GL_FALSE, model);

            float rc = 0.5f + 0.5f * std::sin(t * 2.0f);
            float gc = 0.5f + 0.5f * std::sin(t * 2.3f + 1.0f);
            float bc = 0.5f + 0.5f * std::sin(t * 2.7f + 2.0f);
            glUniform3f(u_color_loc, rc, gc, bc);

            glBindVertexArray(va_base);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // 4. moving triangle (slides left and right)
        {
            float model[16];
            float x = std::sin(t * 1.2f);
            mat4_translate(model, x, yOffsets[3], 0.0f);
            glUniformMatrix4fv(u_model_loc, 1, GL_FALSE, model);
            glUniform3f(u_color_loc, 1.0f, 0.2f, 0.9f);
            glBindVertexArray(va_base);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // 5. rotating triangle (spins in place)
        {
            float rot[16], trans[16], model[16];
            float angle = t * 1.0f;
            mat4_rotate_z(rot, angle);
            mat4_translate(trans, 0.0f, yOffsets[4], 0.0f);
            mat4_mul(trans, rot, model);

            glUniformMatrix4fv(u_model_loc, 1, GL_FALSE, model);
            glUniform3f(u_color_loc, 0.8f, 0.5f, 0.9f);
            glBindVertexArray(va_base);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        Loop();
    }

    // cleanup
    glDeleteVertexArrays(1, &va_rainbow);
    glDeleteBuffers(1, &vb_rainbow);
    glDeleteVertexArrays(1, &va_white);
    glDeleteBuffers(1, &vb_white);
    glDeleteVertexArrays(1, &va_base);
    glDeleteBuffers(1, &vb_base);
    glDeleteProgram(program);
    glDeleteShader(fragment_shader);
    glDeleteShader(vertex_shader);

    DestroyWindow();
    return 0;
}
