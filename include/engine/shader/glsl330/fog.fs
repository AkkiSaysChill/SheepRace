#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;     // Rendered scene color (Slot 0)
uniform sampler2D depthTexture; // Rendered scene depth (Slot 1)

uniform float nearPlane;
uniform float farPlane;
uniform float fogDensity;
uniform vec4 fogColor;

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

void main()
{
    vec4 sceneColor = texture(texture0, fragTexCoord);
    float rawDepth = texture(depthTexture, fragTexCoord).r;

    // Skip sky background at the far clipping plane
    if (rawDepth >= 0.99999)
    {
        finalColor = sceneColor;
        return;
    }

    float linearDist = LinearizeDepth(rawDepth);

    // Standard Raylib exponential-squared fog formula
    float fogFactor = 1.0 / exp((linearDist * fogDensity) * (linearDist * fogDensity));
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    finalColor = mix(fogColor, sceneColor, fogFactor);
}
