#version 330 core

uniform float iTime;
uniform vec2 iResolution;       // Quarter-res dimensions
uniform vec3 u_lightDir;        // Points toward sun (m_currentSkySunVector)
uniform vec3 u_skyColor;
uniform vec3 u_horizonColor;
uniform float u_cloudCover;     // From DayNightCycle (0.0 - 1.0)
uniform float u_cloudFalloff;

uniform vec3 u_cameraPos;
uniform vec3 u_cameraForward;
uniform vec3 u_cameraUp;
uniform vec3 u_cameraRight;
uniform float u_cameraFovY;

uniform sampler2D iChannel0;
uniform sampler2D depthTexture;
uniform float u_nearPlane;
uniform float u_farPlane;

in vec2 fragTexCoord;
out vec4 finalColor;

const float CLOUD_MIN_Y = 250.0;
const float CLOUD_MAX_Y = 650.0;
const int   MAX_STEPS   = 28;   // Slight bump for cleaner 3D ridges (still safe for Intel UHD)

float hash31(vec3 p) {
    p = fract(p * 0.1031);
    p += dot(p, p.yzx + 33.33);
    return fract((p.x + p.y) * p.z);
}

float noise3D(vec3 x) {
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);

    return mix(
        mix(mix(hash31(i + vec3(0,0,0)), hash31(i + vec3(1,0,0)), f.x),
            mix(hash31(i + vec3(0,1,0)), hash31(i + vec3(1,1,0)), f.x), f.y),
        mix(mix(hash31(i + vec3(0,0,1)), hash31(i + vec3(1,0,1)), f.x),
            mix(hash31(i + vec3(0,1,1)), hash31(i + vec3(1,1,1)), f.x), f.y),
        f.z
    );
}

// Sculpted cumulus density with billowy edge erosion
float sampleCloudDensity(vec3 p) {
    float h = clamp((p.y - CLOUD_MIN_Y) / (CLOUD_MAX_Y - CLOUD_MIN_Y), 0.0, 1.0);
    // Flat bottom, rounded towering tops
    float heightMask = smoothstep(0.0, 0.16, h) * smoothstep(1.0, 0.55, h);
    if (heightMask <= 0.001) return 0.0;

    // Wind drift matched to sky.fs
    vec3 wind = vec3(iTime * 0.035, 0.0, iTime * 0.018);
    vec3 q = p * 0.0020 - wind;

    // Base cloud shape
    float base = 0.60 * noise3D(q) + 0.40 * noise3D(q * 2.15);

    float coverage = mix(0.68, 0.34, clamp(u_cloudCover, 0.0, 1.0));
    float d = smoothstep(coverage, coverage + 0.22 * max(u_cloudFalloff, 0.2), base) * heightMask;
    if (d <= 0.001) return 0.0;

    // High-frequency detail erosion (carves cauliflower bumps that catch sunlight)
    float detail = noise3D(q * 5.2 - wind * 1.5);
    d = clamp(d - (1.0 - detail) * 0.28 * (1.0 - d), 0.0, 1.0);

    return d;
}

float hgPhase(float cosTheta, float g) {
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * 3.14159 * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
}

float linearizeDepth(float d) {
    float z = d * 2.0 - 1.0;
    return (2.0 * u_nearPlane * u_farPlane) / (u_farPlane + u_nearPlane - z * (u_farPlane - u_nearPlane));
}

void main() {
    vec2 uv = gl_FragCoord.xy / iResolution.xy;
    vec2 ndc = uv * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    float tanHalfFov = tan(u_cameraFovY * 0.5);

    vec3 viewRay = normalize(vec3(ndc.x * aspect * tanHalfFov, ndc.y * tanHalfFov, 1.0));
    vec3 rd = normalize(u_cameraRight * viewRay.x + u_cameraUp * viewRay.y + u_cameraForward * viewRay.z);

    // Safe ray-slab intersection
    bool insideCloud = (u_cameraPos.y >= CLOUD_MIN_Y && u_cameraPos.y <= CLOUD_MAX_Y);
    float tStart = 0.0;
    float tEnd   = 4000.0;

    if (abs(rd.y) < 0.0005) {
        if (!insideCloud) {
            finalColor = vec4(0.0);
            return;
        }
    } else {
        float tMin = (CLOUD_MIN_Y - u_cameraPos.y) / rd.y;
        float tMax = (CLOUD_MAX_Y - u_cameraPos.y) / rd.y;
        tStart = max(0.0, min(tMin, tMax));
        tEnd   = min(max(tMin, tMax), tStart + 4000.0);
    }

    float rawDepth = texture(depthTexture, uv).r;
    if (rawDepth < 0.9999) {
        float sceneDist = linearizeDepth(rawDepth) / max(dot(rd, u_cameraForward), 0.001);
        tEnd = min(tEnd, sceneDist);
    }

    tEnd = min(tEnd, 12000.0);
    if (tEnd <= tStart) {
        finalColor = vec4(0.0);
        return;
    }

    float marchDist = tEnd - tStart;
    float stepSize = clamp(marchDist / float(MAX_STEPS), 12.0, 90.0);

    // Interleaved Gradient Noise jitter
    float jitter = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));
    float t = tStart + jitter * stepSize;

    vec3 sunDir = normalize(u_lightDir);
    if (length(sunDir) < 0.01) sunDir = normalize(vec3(0.3, 0.8, 0.4));
    bool isNight = (sunDir.y < 0.0);
    if (isNight) sunDir = -sunDir;

    float cosTheta = dot(rd, sunDir);
    // Dual-lobe phase: strong forward silver lining + subtle back-scatter
    float phase = mix(hgPhase(cosTheta, 0.78), hgPhase(cosTheta, -0.25), 0.30);

    vec3 skyCol = (length(u_skyColor) > 0.01) ? u_skyColor : vec3(0.48, 0.65, 0.90);
    vec3 horCol = (length(u_horizonColor) > 0.01) ? u_horizonColor : vec3(0.72, 0.80, 0.88);

    // Warm sunlit highlight vs cool shadowed underside (creates 3D depth)
    vec3 sunCol = isNight ? vec3(0.32, 0.42, 0.62)
                          : mix(vec3(1.0, 0.55, 0.25), vec3(1.05, 1.00, 0.94), clamp(sunDir.y * 2.5, 0.0, 1.0));
    vec3 shadowBottomCol = isNight ? vec3(0.04, 0.06, 0.12)
                                   : mix(vec3(0.22, 0.24, 0.34), vec3(0.30, 0.38, 0.52), clamp(sunDir.y * 2.0, 0.0, 1.0));
    vec3 shadowTopCol    = isNight ? vec3(0.12, 0.16, 0.26)
                                   : mix(horCol, skyCol, 0.55) * 0.85;

    // Exponential sun-march distances: tight local crevices + big macro shadows
    const float sunDists[4] = float[4](15.0, 38.0, 85.0, 175.0);

    vec3 lightEnergy = vec3(0.0);
    float transmittance = 1.0;

    for (int i = 0; i < MAX_STEPS; i++) {
        if (t > tEnd) break;

        vec3 pos = u_cameraPos + rd * t;
        float density = sampleCloudDensity(pos);

        if (density > 0.005) {
            // 1. Exponential sun self-shadowing
            float opticalDepth = 0.0;
            for (int s = 0; s < 4; s++) {
                float dSun = sampleCloudDensity(pos + sunDir * sunDists[s]);
                opticalDepth += dSun * (s == 0 ? 0.8 : 1.0);
            }

            // 2. Multi-scattering approximation (sharp ridge + soft deep scatter)
            float directLight = exp(-opticalDepth * 1.15);
            float bouncedLight = exp(-opticalDepth * 0.28) * 0.35;
            float sunEnergy = max(directLight, bouncedLight);

            // 3. Beer-Powder effect (darkens dense crevices when looking away from sun)
            float powder = 1.0 - exp(-density * 3.5);
            float powderBlend = mix(1.0, powder, clamp(1.0 - cosTheta, 0.0, 1.0) * 0.65);

            // 4. Height-based Ambient Occlusion (dark flat undersides, bright tops)
            float h = clamp((pos.y - CLOUD_MIN_Y) / (CLOUD_MAX_Y - CLOUD_MIN_Y), 0.0, 1.0);
            float heightAO = smoothstep(0.0, 0.75, h);
            vec3 ambient = mix(shadowBottomCol, shadowTopCol, heightAO);
            // Darken thick interior cores slightly for extra 3D volume
            ambient *= mix(1.0, 0.72, density);

            vec3 stepLight = ambient + sunCol * sunEnergy * powderBlend * (0.65 + phase * 1.65);
            float stepAlpha = 1.0 - exp(-density * stepSize * 0.022);

            lightEnergy += transmittance * stepLight * stepAlpha;
            transmittance *= (1.0 - stepAlpha);

            if (transmittance < 0.02) break;
        }
        t += stepSize;
        stepSize *= 1.05;
    }

    // Distance aerial perspective (blends far clouds into horizon haze)
    float distFade = exp(-tStart * 0.00018);
    float alpha = (1.0 - transmittance) * distFade;

    finalColor = vec4(lightEnergy, alpha);
}
