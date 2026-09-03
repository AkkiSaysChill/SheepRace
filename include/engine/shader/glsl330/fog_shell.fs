#version 330

in vec2 fragTexCoord;

uniform sampler2D texture0;    // Color buffer (the rendered scene)
uniform sampler2D depthTexture; // Scene depth buffer

uniform float nearPlane;
uniform float farPlane;
uniform float fogDensity;
uniform vec4 fogColor;

out vec4 finalColor;

// Linearize OpenGL non-linear depth
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; 
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

void main()
{
    vec4 sceneColor = texture(texture0, fragTexCoord);
    float rawDepth = texture(depthTexture, fragTexCoord).r;

    // If depth is at the far plane (skybox/background), skip or use horizon color
    if (rawDepth >= 0.99999)
    {
        finalColor = sceneColor;
        return;
    }

    float linearDist = LinearizeDepth(rawDepth);

    // Raylib exponential-squared fog formula
    float fogFactor = 1.0 / exp((linearDist * fogDensity) * (linearDist * fogDensity));
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    finalColor = mix(fogColor, sceneColor, fogFactor);
}