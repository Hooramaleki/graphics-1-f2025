#include "Window.h"
#include "Shader.h"
#include "raymath.h"
#include <imgui/imgui.h>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <ctime>

// 4 lines in 1 square -- each line has 2 vertices, therefore 8 array elements because 4 lines * 2 vertices per line = 8
// Hint: if 1 square is 8 vertices, and Assignment 2 requires you to render 8 squares, then 8 squares * 8 vertices per square = 64 vertices;
// (Consider reserving 64 vertices worth of space if you'd like to fit all your positions in a single vertex array)


//first square defined (2d array)
static const int line_vertex_count = 8;
static const Vector2 line_vertex_positions[line_vertex_count]
{
    { -1.0f,  -1.0f },   // bottom-left
    {  1.0f,  -1.0f },   // bottom-right

    {  1.0f, -1.0f },   // bottom-right
    {  1.0f,  1.0f },   // top-right

    {   1.0f,  1.0f },   // top-right
    {  -1.0f,  1.0f },   // top-left

    { -1.0f,   1.0f },   // top-left
    { -1.0f,  -1.0f }    // bottom-left
};



int main()
{
	//create storage for all squares (64 vertices)
    //8 squares, each with 8 vertices
    //copying the base square into all_squares[0] as the starting shape
    Vector2 all_squares[8][8]; 
    memcpy(all_squares[0], line_vertex_positions, sizeof(line_vertex_positions));


    //7 smaller squares
    for (int i = 1; i < 8; ++i)
    {
        for (int j = 0; j < 8; j += 2)
        {
            //Vector2Lerp computes the midpoint between two vertices
            //all_squares[1] is centered inside all_squares[0] and so on
            Vector2 mid1 = Vector2Lerp(all_squares[i - 1][j], all_squares[i - 1][j + 1], 0.5f);
            Vector2 mid2 = Vector2Lerp(all_squares[i - 1][(j + 2) % 8], all_squares[i - 1][(j + 3) % 8], 0.5f);
            all_squares[i][j] = mid1;
            all_squares[i][j + 1] = mid2;
        }
    }



    CreateWindow(800, 800, "Graphics 1");
 
    
    GLuint a2_lines_vert = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/a2_lines.vert");
    GLuint a2_lines_frag = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/a2_lines.frag");
    GLuint a2_lines_shader = CreateProgram(a2_lines_vert, a2_lines_frag);



    //combine all squares into one array for buffer upload
    //flatten the 2d all_squares into a 1d vertex_data array of 64 Vector2s
    Vector2 vertex_data[8 * line_vertex_count];
    memcpy(vertex_data, all_squares, sizeof(vertex_data));


    //vertex Buffer Object
    GLuint vbo_line_positions;
    glGenBuffers(1, &vbo_line_positions);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_line_positions);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(line_vertex_positions2), line_vertex_positions2, GL_STATIC_DRAW);
    

    //vertex Array Object
    GLuint vao_line;
    glGenVertexArrays(1, &vao_line);
    glBindVertexArray(vao_line);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vector2), 0);
    glBindVertexArray(GL_NONE);
    glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

    //uniforms
    GLint u_color = glGetUniformLocation(a2_lines_shader, "u_color");
    GLint u_mvp = glGetUniformLocation(a2_lines_shader, "u_mvp");

    //8 colors
    Vector3 colors[8] = {
        {1.0f, 0.0f, 0.0f}, // red
        {0.0f, 1.0f, 0.0f}, // green
        {0.0f, 0.0f, 1.0f}, // blue
        {1.0f, 1.0f, 0.0f}, // yellow
        {1.0f, 0.0f, 1.0f}, // magenta
        {0.0f, 1.0f, 1.0f}, // cyan
        {1.0f, 0.5f, 0.0f}, // orange
        {0.5f, 0.0f, 1.0f}  // purple
    };






    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_ESCAPE))
            SetWindowShouldClose(true);

        Matrix proj = MatrixOrtho(-1.0f, 1.0f, -1.0f, 1.0f, 0.01f, 100.0f);
        Matrix view = MatrixLookAt({ 0.0f, 0.0f, 10.0f }, { 0.0f, 0.0f, 0.0f }, Vector3UnitY);
        Matrix world = MatrixIdentity();
        Matrix mvp = world * view * proj;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        //Compute world and mvp matrix each frame
        glUseProgram(a2_lines_shader);
        glUniformMatrix4fv(u_mvp, 1, GL_FALSE, &mvp.m0);

        glBindVertexArray(vao_line);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_line_positions);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vector2), 0);
        glLineWidth(4.0f);

        //draw all 8 squares in one loop
        for (int i = 0; i < 8; ++i)
        {
            glUniform3f(u_color, colors[i].x, colors[i].y, colors[i].z);
            glBufferData(GL_ARRAY_BUFFER, sizeof(all_squares[i]), all_squares[i], GL_STATIC_DRAW);
            glDrawArrays(GL_LINES, 0, line_vertex_count);

            //call glBufferData again here to upload the single all_squares[i] before drawing
        }


        BeginGui();
        EndGui();

        Loop();
    }

    // Cleanup
    glDeleteVertexArrays(1, &vao_line);
    glDeleteBuffers(1, &vbo_line_positions);
    glDeleteProgram(a2_lines_shader);
    glDeleteShader(a2_lines_frag);
    glDeleteShader(a2_lines_vert);

    DestroyWindow();
    return 0;
}

