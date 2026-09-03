#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec3 viewPos;
uniform vec3 lightDir;

out vec4 finalColor;

// GTA San Andreas Coastal Palette
const vec3 FOG_HORIZON_CYAN = vec3(0.38, 0.65, 0.76); // Signature teal/blue haze
const vec3 FOG_SUN_GLOW      = vec3(0.95, 0.88, 0.72); // Warm sun scatter

const float FOG_START_DIST   = 600.0;
const float FOG_END_DIST     = 5500.0;
const float FOG_HEIGHT_DENSE = 0.0018;

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord) * fragColor;
    if (texelColor.a < 0.1) discard;

    vec3 camToFrag = fragPosition - viewPos;
    float dist = length(camToFrag);
    vec3 rayDir = camToFrag / max(dist, 0.001);

    // 1. Smooth distance ramp
    float distFactor = clamp((dist - FOG_START_DIST) / (FOG_END_DIST - FOG_START_DIST), 0.0, 1.0);
    float fogDensity = distFactor * distFactor * (3.0 - 2.0 * distFactor);

    // 2. Vertical marine layer: dense near water, fades out high in the sky
    float heightWeight = exp(-max(0.0, fragPosition.y) * FOG_HEIGHT_DENSE);
    float totalFog = clamp(fogDensity * (0.4 + 0.6 * heightWeight), 0.0, 1.0);

    // 3. Forward sun scattering (Mie glow)
    float sunDot = max(0.0, dot(rayDir, normalize(-lightDir)));
    vec3 fogColor = mix(FOG_HORIZON_CYAN, FOG_SUN_GLOW, pow(sunDot, 6.0) * 0.4);

    // Dissolve into the atmospheric haze
    finalColor = vec4(mix(texelColor.rgb, fogColor, totalFog), texelColor.a);
}
