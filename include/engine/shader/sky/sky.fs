#version 330 core

// --- Uniforms from Raylib ---
uniform float iTime;
uniform vec2 iResolution;
uniform vec3 u_lightDir;

// Camera Uniforms
uniform vec3 u_cameraPos;
uniform vec3 u_cameraForward;
uniform vec3 u_cameraUp;
uniform vec3 u_cameraRight;
uniform float u_cameraFovY;

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

// Shared Horizon Color (Must match ocean.fs)
const vec3 SHARED_HORIZON_HAZE = vec3(0.58, 0.72, 0.88);

// Ocean Palette
const vec3 OCEAN_DEEP  = vec3(0.015, 0.08, 0.22);
const vec3 OCEAN_MID   = vec3(0.035, 0.16, 0.38);
const vec3 OCEAN_LIGHT = vec3(0.080, 0.32, 0.58);

// Fast 2D Noise
vec2 hash2(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453123);
}

float noise(vec2 p) {
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

const mat2 m = mat2(1.6, 1.2, -1.2, 1.6);

float fbm(vec2 p) {
    float f = 0.0;
    f += 0.500 * noise(p); p = m * p * 2.02;
    f += 0.250 * noise(p); p = m * p * 2.03;
    f += 0.125 * noise(p);
    return f;
}

// Infinite Ocean Generator (Evaluated beyond the 3D mesh boundaries)
vec3 RenderInfiniteOcean(vec3 ro, vec3 rd, vec3 sunDir) {
    // Intersect ray with water surface plane (Y = 0)
    float t = -max(ro.y, 1.0) / rd.y;
    vec3 hitPos = ro + rd * t;

    // Distant water base color
    float NdotL = max(dot(vec3(0.0, 1.0, 0.0), sunDir), 0.0);
    vec3 waterColor = mix(OCEAN_MID, OCEAN_LIGHT, NdotL * 0.4);

    // Specular Sun Reflection on water plane
    vec3 R = reflect(rd, vec3(0.0, 1.0, 0.0));
    float RdotSun = max(0.0, dot(R, sunDir));
    float spec = pow(RdotSun, 120.0) * 0.6;
    waterColor += vec3(1.0, 0.95, 0.85) * spec;

    // Atmospheric Horizon Fog (Ramps to 100% at the horizon line)
    float fog = 1.0 - exp(-t * 0.00015);
    float horizonBlend = smoothstep(0.0, -0.04, rd.y);
    float totalFog = clamp(max(fog, 1.0 - horizonBlend), 0.0, 1.0);

    return mix(waterColor, SHARED_HORIZON_HAZE, totalFog);
}

vec3 Sky(in vec3 ro, in vec3 rd, in vec3 lightDir) {
    float sundot = clamp(dot(rd, lightDir), 0.0, 1.0);

    // 1. Atmosphere Gradient (Zenith to Horizon)
    vec3 zenithCol  = vec3(0.15, 0.40, 0.82);
    vec3 horizonCol = SHARED_HORIZON_HAZE;
    float skyHeight = max(rd.y, 0.0);
    vec3 skyCol = mix(horizonCol, zenithCol, pow(skyHeight, 0.65));

    // Horizon mist band
    float horizonBand = exp(-skyHeight * 14.0);
    skyCol = mix(skyCol, SHARED_HORIZON_HAZE, horizonBand);

    // 2. Compact Sun Disk & Soft Corona
    float sunDisk   = pow(sundot, 1800.0) * 1.5;
    float sunCorona = pow(sundot, 128.0) * 0.25;
    skyCol += vec3(1.0, 0.96, 0.85) * (sunDisk + sunCorona);

    // 3. Clouds (Only computed when looking above horizon)
    if (rd.y > 0.015) {
        vec2 domeUV = (rd.xz / (rd.y + 0.35)) * 1.15;
        vec2 wind = vec2(iTime * 0.0010, iTime * 0.0005);
        vec2 p = domeUV - wind;

        float density = fbm(p);
        float puffs   = fbm(p * 2.2 + vec2(1.7, 4.3));
        float clouds  = smoothstep(-0.02, 0.38, density + puffs * 0.45);

        float sunFacing = clamp(dot(vec3(rd.x, 0.3, rd.z), lightDir) * 0.5 + 0.5, 0.0, 1.0);
        vec3 cloudHighlight = vec3(1.00, 0.99, 0.96);
        vec3 cloudShadow    = vec3(0.62, 0.69, 0.82);
        vec3 cloudColor     = mix(cloudShadow, cloudHighlight, sunFacing * 0.45 + density * 0.55);

        float horizonFade = smoothstep(0.015, 0.16, rd.y);
        skyCol = mix(skyCol, cloudColor, clouds * horizonFade * 0.94);
        return skyCol;
    }

    // 4. Infinite Ocean Floor (Below Horizon)
    return RenderInfiniteOcean(ro, rd, lightDir);
}

void main() {
    vec2 uv = (gl_FragCoord.xy / iResolution.xy) * 2.0 - 1.0;
    float aspectRatio = iResolution.x / iResolution.y;
    float tanHalfFovY = tan(u_cameraFovY * 0.5);

    vec3 viewRayDir = normalize(vec3(uv.x * aspectRatio * tanHalfFovY,
                                     uv.y * tanHalfFovY,
                                     1.0));

    vec3 rd = normalize(u_cameraRight * viewRayDir.x +
                        u_cameraUp * viewRayDir.y +
                        u_cameraForward * viewRayDir.z);

    vec3 sunDir = normalize(u_lightDir);
    if (length(sunDir) < 0.1) sunDir = normalize(vec3(0.35, 0.85, 0.40));

    vec3 col = Sky(u_cameraPos, rd, sunDir);
    finalColor = vec4(col, 1.0);
}