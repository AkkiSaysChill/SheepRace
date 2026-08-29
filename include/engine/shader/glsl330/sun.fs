#version 330

// Input uniform values
uniform vec4 sunColor;

// Output fragment color
out vec4 finalColor;

void main()
{
    // Just output the bright color
    finalColor = sunColor;
}