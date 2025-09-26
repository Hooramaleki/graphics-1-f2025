#include "Window.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdio>
#include <iostream>

//basic 4x4 matrix math (column-major)
static void mat4_identity(float m[16]) {
    for (int i = 0; i < 16; ++i) m[i] = 0.0f;
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static void mat4_translate(float m[16], float tx, float ty, float tz) {
    mat4_identity(m);
    m[12] = tx; m[13] = ty; m[14] = tz;
}

static void mat4_rotate_z(float m[16], float radians) {
    float c = std::cos(radians);
    float s = std::sin(radians);
    mat4_identity(m);
    m[0] = c;  m[1] = s;
    m[4] = -s; m[5] = c;
}

static void mat4_mul(const float A[16], const float B[16], float out[16]) {
    for (int col = 0; col < 4; ++col)
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k)
                sum += A[k * 4 + row] * B[col * 4 + k];
            out[col * 4 + row] = sum;
        }
}

//geometry : plain triangle vs rainbow triangle
static const float tri_pos[] = {
     0.0f,  0.25f, 0.0f,
     0.25f, -0.25f, 0.0f,
    -0.25f, -0.25f, 0.0f
};

static const float tri_poscol[] = {
     0.0f,  0.25f, 0.0f,   1.0f, 0.0f, 0.0f,
     0.25f, -0.25f, 0.0f,  0.0f, 1.0f, 0.0f,
    -0.25f, -0.25f, 0.0f,  0.0f, 0.0f, 1.0f
};

// shader sources
static const char* vs_uniform = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 u_world;
void main() { gl_Position = u_world * vec4(aPos, 1.0); }
)";

static const char* fs_uniform = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 u_color;
uniform float u_intensity;
void main() {
    vec3 c = u_color * u_intensity;
    FragColor = vec4(c, 1.0);
}
)";

static const char* vs_vcolor = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aCol;
uniform mat4 u_world;
out vec3 color;
void main() {
    gl_Position = u_world * vec4(aPos, 1.0);
    color = aCol;
}
)";

static const char* fs_vcolor = R"(
#version 330 core
in vec3 color;
out vec4 FragColor;
void main() { FragColor = vec4(color, 1.0); }
)";

//shader helpers
static GLuint CompileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[1024]; glGetShaderInfoLog(s, 1024, NULL, buf);
        std::cerr << "shader compile error: " << buf << "\n";
    }
    return s;
}

static GLuint CreateProgram(const char* vs, const char* fs) {
    GLuint vsId = CompileShader(GL_VERTEX_SHADER, vs);
    GLuint fsId = CompileShader(GL_FRAGMENT_SHADER, fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, vsId);
    glAttachShader(p, fsId);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[1024]; glGetProgramInfoLog(p, 1024, NULL, buf);
        std::cerr << "program link error: " << buf << "\n";
    }
    glDeleteShader(vsId);
    glDeleteShader(fsId);
    return p;
}

int main() {
    CreateWindow(800, 800, "OpenGL Assignment - fixed");

    //build two shader programs
    GLuint progUniform = CreateProgram(vs_uniform, fs_uniform);
    GLuint progVColor = CreateProgram(vs_vcolor, fs_vcolor);

    //uniform locations
    GLint u_world_uni = glGetUniformLocation(progUniform, "u_world");
    GLint u_color_uni = glGetUniformLocation(progUniform, "u_color");
    GLint u_intensity_uni = glGetUniformLocation(progUniform, "u_intensity");
    GLint u_world_vcol = glGetUniformLocation(progVColor, "u_world");

    //triangle with solid color
    GLuint vao_pos = 0, vbo_pos = 0;
    glGenVertexArrays(1, &vao_pos);
    glGenBuffers(1, &vbo_pos);
    glBindVertexArray(vao_pos);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_pos);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tri_pos), tri_pos, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    //triangle with rainbow vertices
    GLuint vao_col = 0, vbo_col = 0;
    glGenVertexArrays(1, &vao_col);
    glGenBuffers(1, &vbo_col);
    glBindVertexArray(vao_col);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_col);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tri_poscol), tri_poscol, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    //y-offsets: all set to zero so they draw in the center
    const float yOffsets[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    int object = 0;
    std::printf("object %d\n", object + 1);

    while (!WindowShouldClose()) {
        float t = (float)glfwGetTime();

        //clear screen with pink background
        glClearColor(239.0f / 255.0f, 136.0f / 255.0f, 190.0f / 255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        switch (object + 1) {
        case 1: { //plain white
            float trans[16]; mat4_translate(trans, 0.0f, yOffsets[0], 0.0f);
            glUseProgram(progUniform);
            glUniformMatrix4fv(u_world_uni, 1, GL_FALSE, trans);
            glUniform3f(u_color_uni, 1.0f, 1.0f, 1.0f);
            glUniform1f(u_intensity_uni, 1.0f);
            glBindVertexArray(vao_pos);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        } break;

        case 2: { //rainbow vertices
            float trans[16]; mat4_translate(trans, 0.0f, yOffsets[1], 0.0f);
            glUseProgram(progVColor);
            glUniformMatrix4fv(u_world_vcol, 1, GL_FALSE, trans);
            glBindVertexArray(vao_col);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        } break;

        case 3: { //color changing over time
            float trans[16]; mat4_translate(trans, 0.0f, yOffsets[2], 0.0f);
            float rc = 0.5f + 0.5f * std::sin(t * 2.0f);
            float gc = 0.5f + 0.5f * std::sin(t * 2.3f + 1.0f);
            float bc = 0.5f + 0.5f * std::sin(t * 2.7f + 2.0f);
            glUseProgram(progUniform);
            glUniformMatrix4fv(u_world_uni, 1, GL_FALSE, trans);
            glUniform3f(u_color_uni, rc, gc, bc);
            glUniform1f(u_intensity_uni, 1.0f);
            glBindVertexArray(vao_pos);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        } break;

        case 4: { //moving left and right
            float tx = 0.5f * std::sin(t * 1.2f);
            float trans[16]; mat4_translate(trans, tx, yOffsets[3], 0.0f);
            glUseProgram(progUniform);
            glUniformMatrix4fv(u_world_uni, 1, GL_FALSE, trans);
            glUniform3f(u_color_uni, 1.0f, 0.5f, 0.0f);
            glUniform1f(u_intensity_uni, 1.0f);
            glBindVertexArray(vao_pos);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        } break;

        case 5: { //rotating
            float rot[16], trans[16], model[16];
            mat4_rotate_z(rot, t);
            mat4_translate(trans, 0.0f, yOffsets[4], 0.0f);
            mat4_mul(trans, rot, model);
            glUseProgram(progUniform);
            glUniformMatrix4fv(u_world_uni, 1, GL_FALSE, model);
            glUniform3f(u_color_uni, 0.2f, 0.8f, 0.2f);
            glUniform1f(u_intensity_uni, 1.0f);
            glBindVertexArray(vao_pos);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        } break;
        }

        //switch object when pressing space
        if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_SPACE) == GLFW_PRESS) {
            object = (object + 1) % 5;
            std::printf("object %d\n", object + 1);
            while (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_SPACE) == GLFW_PRESS)
                Loop();
        }

        Loop(); 
    }

    
    glDeleteVertexArrays(1, &vao_pos);
    glDeleteBuffers(1, &vbo_pos);
    glDeleteVertexArrays(1, &vao_col);
    glDeleteBuffers(1, &vbo_col);
    glDeleteProgram(progUniform);
    glDeleteProgram(progVColor);

    DestroyWindow();
    return 0;
}
