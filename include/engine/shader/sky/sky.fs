#version 330 core

uniform float iTime;
uniform vec2 iResolution;
uniform vec3 u_lightDir;

// Dynamic atmospheric uniforms
uniform vec3 u_skyColor;
uniform vec3 u_horizonColor;
uniform float u_cloudCover;
uniform float u_cloudFalloff;
uniform float u_fogDensity;

// Camera Uniforms
uniform vec3 u_cameraPos;
uniform vec3 u_cameraForward;
uniform vec3 u_cameraUp;
uniform vec3 u_cameraRight;
uniform float u_cameraFovY;

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

const vec3 OCEAN_DEEP  = vec3(0.015, 0.04, 0.12);
const vec3 OCEAN_MID   = vec3(0.025, 0.09, 0.22);
const vec3 OCEAN_LIGHT = vec3(0.060, 0.20, 0.38);

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

vec3 RenderInfiniteOcean(vec3 ro, vec3 rd, vec3 sunDir, bool isNight) {
    float t = -max(ro.y, 1.0) / rd.y;
    float NdotL = max(dot(vec3(0.0, 1.0, 0.0), sunDir), 0.0);
    vec3 waterColor = mix(OCEAN_DEEP, isNight ? OCEAN_MID * 0.4 : OCEAN_LIGHT, NdotL * 0.5);

    vec3 R = reflect(rd, vec3(0.0, 1.0, 0.0));
    float RdotSun = max(0.0, dot(R, sunDir));
    float spec = pow(RdotSun, isNight ? 60.0 : 120.0) * (isNight ? 0.25 : 0.6);
    waterColor += (isNight ? vec3(0.5, 0.7, 1.0) : vec3(1.0, 0.95, 0.85)) * spec;

    // Atmospheric Fog distance calculation scaled by weather fog density
    float effectiveFogDensity = max(u_fogDensity, 0.00010);
    float fog = 1.0 - exp(-t * effectiveFogDensity);
    float horizonBlend = smoothstep(0.0, -0.04, rd.y);
    float totalFog = clamp(max(fog, 1.0 - horizonBlend), 0.0, 1.0);

    return mix(waterColor, u_horizonColor, totalFog);
}

vec3 Sky(in vec3 ro, in vec3 rd, in vec3 celDir, bool isNight) {
    if (rd.y <= 0.0) {
        return RenderInfiniteOcean(ro, rd, celDir, isNight);
    }

    float celestialDot = clamp(dot(rd, celDir), 0.0, 1.0);

    // 1. Atmosphere Gradient (Zenith to Horizon)
    float skyHeight = max(rd.y, 0.0);
    vec3 skyCol = mix(u_horizonColor, u_skyColor, pow(skyHeight, 0.65));

    // Dynamic Fog Wall on Horizon (expands upwards as fog density increases)
    float fogBandHeight = 12.0 - clamp(u_fogDensity * 8000.0, 0.0, 9.0);
    float horizonBand = exp(-skyHeight * fogBandHeight);
    skyCol = mix(skyCol, u_horizonColor, horizonBand);

    // 2. Celestial Body (Dimmed when fog or cloud cover is heavy)
    float visibilityFactor = clamp(1.0 - (u_fogDensity * 1200.0), 0.1, 1.0);
    if (!isNight) {
        float sunDisk   = pow(celestialDot, 1800.0) * 1.5;
        float sunCorona = pow(celestialDot, 128.0) * 0.25;
        vec3 sunColor   = mix(vec3(1.0, 0.5, 0.2), vec3(1.0, 0.96, 0.85), clamp(celDir.y * 2.5, 0.0, 1.0));
        skyCol += sunColor * (sunDisk + sunCorona) * visibilityFactor;
    } else {
        float moonDisk   = pow(celestialDot, 2400.0) * 1.6;
        float moonCorona = pow(celestialDot, 90.0) * 0.12;
        skyCol += vec3(0.85, 0.92, 1.0) * (moonDisk + moonCorona) * visibilityFactor;

        if (rd.y > 0.05 && u_cloudCover < 0.7 && u_fogDensity < 0.0004) {
            vec2 starCoord = rd.xz / (rd.y + 0.15) * 85.0;
            float starHash = fract(sin(dot(floor(starCoord), vec2(12.9898, 78.233))) * 43758.5453);
            if (starHash > 0.992) {
                float twinkle = sin(iTime * 2.0 + starHash * 100.0) * 0.3 + 0.7;
                skyCol += vec3(twinkle * (starHash - 0.992) * 120.0) * smoothstep(0.05, 0.25, rd.y);
            }
        }
    }

    // 3. Dynamic Clouds Layer
    if (rd.y > 0.015) {
        vec2 domeUV = (rd.xz / (rd.y + 0.35)) * 1.15;
        vec2 wind = vec2(iTime * 0.035, iTime * 0.018);
        vec2 p = domeUV - wind;

        float density = fbm(p);
        float puffs   = fbm(p * 2.2 + vec2(1.7, 4.3));
        float totalNoise = density + puffs * 0.45;

        // Map u_cloudCover (0.1 -> 0.95) to shift coverage smoothly
        // Higher u_cloudCover lowers the lower bound, covering more of the dome
        float lowerThreshold = mix(0.25, -0.22, clamp(u_cloudCover, 0.0, 1.0));
        float upperThreshold = lowerThreshold + mix(0.45, 0.25, clamp(u_cloudFalloff, 0.0, 1.0));
        float clouds = smoothstep(lowerThreshold, upperThreshold, totalNoise);

        float sunFacing = clamp(dot(vec3(rd.x, 0.3, rd.z), celDir) * 0.5 + 0.5, 0.0, 1.0);
        
        vec3 cloudHighlight = isNight ? vec3(0.20, 0.24, 0.32) : mix(vec3(1.0, 0.65, 0.45), vec3(1.00, 0.99, 0.96), clamp(celDir.y * 3.0, 0.0, 1.0));
        vec3 cloudShadow    = isNight ? vec3(0.05, 0.06, 0.10) : mix(vec3(0.35, 0.28, 0.35), vec3(0.62, 0.69, 0.82), clamp(celDir.y * 2.0, 0.0, 1.0));
        vec3 cloudColor     = mix(cloudShadow, cloudHighlight, sunFacing * 0.45 + density * 0.55);

        float horizonFade = smoothstep(0.015, 0.16, rd.y);
        skyCol = mix(skyCol, cloudColor, clouds * horizonFade * 0.94);

        // Blanket sky with fog color when fog is dense
        float fogStrength = clamp(u_fogDensity * 1500.0, 0.0, 0.85);
        skyCol = mix(skyCol, u_horizonColor, fogStrength);

        return skyCol;
    }

    return RenderInfiniteOcean(ro, rd, celDir, isNight);
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

    vec3 celDir = normalize(u_lightDir);
    if (length(celDir) < 0.01) celDir = vec3(0.0, 1.0, 0.0);
    bool isNight = (celDir.y < 0.0);
    vec3 skyTargetDir = isNight ? -celDir : celDir;

    vec3 col = Sky(u_cameraPos, rd, skyTargetDir, isNight);
    finalColor = vec4(col, 1.0);
}