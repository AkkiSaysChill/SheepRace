#version 330 core

in vec3 fragPosition;
in vec2 fragTexCoord;

uniform vec3 viewPos;
uniform float time;
uniform vec3 u_lightDir;

out vec4 finalColor;

// --- Shared Horizon Color (Must match sky.fs) ---
const vec3 SHARED_HORIZON_HAZE = vec3(0.58, 0.72, 0.88);

// --- Oceanic Palette ---
const vec3 TOON_DEEP   = vec3(0.015, 0.08, 0.22);
const vec3 TOON_MID    = vec3(0.035, 0.16, 0.38);
const vec3 TOON_LIGHT  = vec3(0.080, 0.32, 0.58);
const vec3 FOAM_TINT   = vec3(0.70, 0.85, 0.98);

// Fast 2D Procedural Noise
vec2 hash2(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453123);
}

float gradNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(dot(-1.0 + 2.0 * hash2(i + vec2(0.0, 0.0)), f - vec2(0.0, 0.0)),
            dot(-1.0 + 2.0 * hash2(i + vec2(1.0, 0.0)), f - vec2(1.0, 0.0)), u.x),
        mix(dot(-1.0 + 2.0 * hash2(i + vec2(0.0, 1.0)), f - vec2(0.0, 1.0)),
            dot(-1.0 + 2.0 * hash2(i + vec2(1.0, 1.0)), f - vec2(1.0, 1.0)), u.x),
        u.y
    );
}

float voronoiFoam(vec2 p) {
    vec2 i_pos = floor(p);
    vec2 f_pos = fract(p);

    float minDist = 1.0;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 point = hash2(i_pos + neighbor);
            point = 0.5 + 0.35 * sin(time * 0.9 + 6.2831 * point);
            vec2 diff = neighbor + point - f_pos;
            minDist = min(minDist, length(diff));
        }
    }
    return 1.0 - minDist;
}

float GetFoamPattern(vec2 worldPos) {
    vec2 warp = vec2(
        gradNoise(worldPos * 0.012 + time * 0.04),
        gradNoise(worldPos * 0.012 + vec2(5.2, 1.3) - time * 0.03)
    ) * 16.0;

    vec2 p = (worldPos + warp) * 0.035;
    float v1 = voronoiFoam(p);
    float v2 = voronoiFoam(p * 1.5 + vec2(time * 0.08, -time * 0.06));

    float foam = smoothstep(0.70, 0.94, v1 * 0.65 + v2 * 0.45);
    float macroMask = gradNoise(worldPos * 0.005 + vec2(time * 0.015, 0.0)) * 0.5 + 0.5;
    foam *= smoothstep(0.48, 0.72, macroMask);
    return foam;
}

void main() {
    vec3 toFrag = fragPosition - viewPos;
    float dist = length(toFrag);
    vec3 ray = normalize(toFrag);

    // 1. Sun Direction Vector (skySun points toward the sun)
    vec3 sDir = normalize(u_lightDir);
    if (length(sDir) < 0.1) sDir = normalize(vec3(-0.35, 0.85, -0.40));

    // 2. Wave Shimmer Normal (Noise is already [-1, 1], removed the biased -0.5)
    vec2 waveUV = fragPosition.xz * 0.012 + vec2(time * 0.05, time * 0.03);
    float n1 = gradNoise(waveUV);
    float n2 = gradNoise(waveUV * 2.1 - time * 0.04);
    vec3 normal = normalize(vec3(-n1 * 0.15, 1.0, -n2 * 0.15));

    // 3. Cel-Shaded Bands
    float NdotL = dot(normal, sDir);
    vec3 waterBase;
    if (NdotL > 0.35)       waterBase = TOON_LIGHT;
    else if (NdotL > -0.15) waterBase = TOON_MID;
    else                    waterBase = TOON_DEEP;

    // 4. Fresnel Rim
    float NdotV = max(0.0, dot(normal, -ray));
    float fresnel = pow(1.0 - NdotV, 3.5);
    waterBase = mix(waterBase, TOON_LIGHT, step(0.70, fresnel) * 0.30);

    // 5. Specular Reflection
    // reflect() expects the vector pointing TOWARD the surface (ray)
    vec3 R = reflect(ray, normal);
    float RdotSun = max(0.0, dot(R, sDir));
    float spec = pow(RdotSun, 180.0);

    // Suppress specular artifacts if the sun is below the horizon or viewing from underneath
    spec *= step(0.0, dot(vec3(0.0, 1.0, 0.0), sDir));
    spec *= step(0.0, R.y);

    vec3 specColor = vec3(1.0, 0.96, 0.85) * spec * 2.5;

    // 6. Surface Froth
    float foam = GetFoamPattern(fragPosition.xz);
    foam *= clamp(1.0 - dist / 7000.0, 0.0, 1.0);
    vec3 surfaceRgb = mix(waterBase + specColor, FOAM_TINT, foam * 0.40);

    // 7. Gamma Correction
    surfaceRgb = pow(surfaceRgb, vec3(1.0 / 1.6));

    // 8. Distance Horizon Fade
    float distanceFade = smoothstep(6000.0, 35000.0, dist);
    float horizonAngleFade = smoothstep(0.0, -0.04, ray.y);
    float totalHorizonFog = clamp(max(distanceFade, 1.0 - horizonAngleFade), 0.0, 1.0);

    vec3 finalRgb = mix(surfaceRgb, SHARED_HORIZON_HAZE, totalHorizonFog);

    finalColor = vec4(finalRgb, 1.0);
}