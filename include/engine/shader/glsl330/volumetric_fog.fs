#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragPosition;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec3 viewPos;
uniform vec3 u_lightDir; // Normalized vector pointing up toward the sun
uniform vec3 u_horizonColor;

out vec4 finalColor;

// --- Tuned White Atmospheric Fog Constants ---
const float FOG_START_DIST     = 800.0;   // Keeps close terrain/plane completely clear
const float FOG_END_DIST       = 6500.0;  // Full dense white coverage in mid-distance
const float FOG_HEIGHT_FALLOFF = 0.0025;  // Thicker near sea, thins out at high altitude
const float FOG_SEA_LEVEL      = 0.0;

// Pure, luminous white mist colors
const vec3 PURE_WHITE_FOG      = vec3(0.94, 0.96, 0.99);
const vec3 FOG_SUN_GLOW        = vec3(1.00, 0.98, 0.92);

void main()
{
    vec4 texColor = texture(texture0, fragTexCoord) * colDiffuse * fragColor;
    if (texColor.a < 0.1) discard;

    vec3 N = normalize(fragNormal);
    vec3 L = normalize(u_lightDir);
    if (L.y < 0.0) L = -L; // Ensure light vector points skyward

    // Terrain lighting: vibrant ambient + soft directional fill
    float diff = max(dot(N, L), 0.0);
    vec3 ambient = vec3(0.48, 0.52, 0.58);
    vec3 litColor = texColor.rgb * (ambient + diff * 0.65);

    // --- Distance & Vector Setup ---
    vec3 camToFrag = fragPosition - viewPos;
    float dist = length(camToFrag);
    vec3 rayDir = camToFrag / max(dist, 0.001);

    // 1. Height Falloff Weight (Volumetric vertical profile)
    float deltaY = rayDir.y * dist;
    float heightWeight;
    if (abs(deltaY) > 0.001) {
        heightWeight = (exp(-FOG_HEIGHT_FALLOFF * max(0.0, viewPos.y - FOG_SEA_LEVEL)) - 
                        exp(-FOG_HEIGHT_FALLOFF * max(0.0, fragPosition.y - FOG_SEA_LEVEL))) / 
                       (FOG_HEIGHT_FALLOFF * deltaY);
        heightWeight *= dist;
    } else {
        heightWeight = exp(-FOG_HEIGHT_FALLOFF * max(0.0, viewPos.y - FOG_SEA_LEVEL)) * dist;
    }

    // 2. Smooth Linear Distance Ramp (Not too far, not too close)
    float linearFog = clamp((dist - FOG_START_DIST) / (FOG_END_DIST - FOG_START_DIST), 0.0, 1.0);
    linearFog = smoothstep(0.0, 1.0, linearFog);

    // 3. Volumetric Altitude Blend
    float altitudeDensity = clamp(heightWeight * 0.0003, 0.0, 1.0);
    float totalFogAmount = clamp(linearFog * 0.85 + altitudeDensity * 0.35, 0.0, 1.0);

    // 4. Forward Sun Rim Glow in the white mist
    float sunScatter = max(0.0, dot(rayDir, L));
    vec3 finalFogColor = mix(PURE_WHITE_FOG, FOG_SUN_GLOW, pow(sunScatter, 6.0) * 0.35);

    // Final composite into pure white fog without dirty gamma distortion
    vec3 finalRgb = mix(litColor, finalFogColor, totalFogAmount);

    finalColor = vec4(finalRgb, texColor.a);
}