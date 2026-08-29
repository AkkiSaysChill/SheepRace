#version 330
in vec3 vertexPosition;
uniform mat4 lightSpaceMatrix;
uniform mat4 model;
void main() {
    gl_Position = lightSpaceMatrix * model * vec4(vertexPosition, 1.0);
}