#version 100

attribute vec2 vertexPosition;
attribute vec2 vertexTexCoord;

varying vec2 fragTexCoord;

void main()
{
    fragTexCoord = vertexTexCoord;
    gl_Position = vec4(vertexPosition, 0.0, 1.0);
}
