#version 330

// Input vertex attributes
in vec3 vertexPosition;

// Input uniform values
// raylib will automatically send the Model-View-Projection matrix to this uniform
uniform mat4 matMvp;

void main()
{
    // Calculate final vertex position
    gl_Position = matMvp * vec4(vertexPosition, 1.0);
}