#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 clipSpacePos;

uniform vec3 cameraPos;
uniform float r0;
uniform float scroll;
uniform sampler2D depthMap;
uniform sampler2D reflectionMap; 
uniform float cameraNear;
uniform float cameraFar;
uniform int u_isReflection;

out vec4 finalColor;

float linearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    return (2.0 * cameraNear * cameraFar) / (cameraFar + cameraNear - z * (cameraFar - cameraNear));
}

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise(vec2 x) {
    vec2 i = floor(x);
    vec2 f = fract(x);
    vec2 u = f * f * (3.0 - 2.0 * f);
    float a = hash(i + vec2(0.0, 0.0));
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y) - 0.5;
}

const mat2 rot = mat2(0.8, 0.6, -0.6, 0.8);

float getWaterHeight(vec2 pos) {
    float res = 0.0;
    float freq = 1.0;
    float amp  = 1.0;
    float waveSpeed = 0.5;
    for (int i = 0; i < 6; i++) {
        res  += noise(freq * pos + scroll * waveSpeed * float(6 - i)) * amp;
        freq *= 1.75;
        amp  *= 0.5;
        pos   = rot * pos;
    }
    return res * 4.0;
}

vec3 ACESFilm(vec3 x) {
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

void main()
{
    // FIX: Relaxed the underwater single-mesh discard pass threshold 
    // so the basin floor doesn't leave blank sky gaps in your reflection buffer.
    if (u_isReflection == 1 && fragPosition.y < -5.0) {
        discard;
    }

    // ==========================================================
    // CAMERA & VECTOR SETUP
    // ==========================================================
    vec3 viewDir = cameraPos - fragPosition;
    float distToCamera = length(viewDir);
    if (distToCamera < 0.001) viewDir = vec3(0.0, 1.0, 0.0);
    vec3 V = normalize(viewDir);

    float e = 0.005 + (distToCamera * 0.0001);
    
    // FIX: Toned down from 10.0 / 30.0 so your procedural waves 
    // maintain clear macro dimensions across the massive 500x500 water model.
    vec2 uv = fragTexCoord * 30.0;

    // ==========================================================
    // WAVE NORMAL GENERATION (FBM Noise)
    // ==========================================================
    float h  = getWaterHeight(uv);
    float hx = getWaterHeight(uv + vec2(e, 0.0));
    float hy = getWaterHeight(uv + vec2(0.0, e));

    vec3 rippleNormal = normalize(vec3(h - hx, e * 1.5, h - hy));
    vec3 N = normalize(fragNormal + rippleNormal * 0.2);

    // ==========================================================
    // PHYSICALLY ACCURATE FRESNEL (Schlick's Approximation)
    // ==========================================================
    float cosTheta = max(dot(V, N), 0.0);
    float fresnel = r0 + (1.0 - r0) * pow(1.0 - cosTheta, 5.0);
    fresnel = clamp(fresnel, 0.0, 1.0); // Let it hit full mirror opacity at the horizon!

    // ==========================================================
    // LIGHTING (Specular & Subsurface Scattering)
    // ==========================================================
    vec3 lightDirection = normalize(vec3(0.0, 0.35, 1.0));
    vec3 sunLightColour = vec3(1.0, 0.95, 0.9);

    // Subsurface Scattering (SSS)
    vec3 scatterHalf = normalize(-lightDirection + N * 0.2);
    float vDotH = pow(clamp(dot(V, -scatterHalf), 0.0, 1.0), 8.0);
    vec3 sss = 0.08 * vDotH * vec3(0.05, 0.7, 0.65);

    // Blinn-Phong Specular
    vec3 halfwayDir = normalize(lightDirection + V);
    float specAngle = max(dot(N, halfwayDir), 0.0);
    float specular = pow(specAngle, 128.0);
    float distFade = clamp(1.0 - (distToCamera / 100.0), 0.0, 1.0);
    float nearFade = smoothstep(1.5, 5.0, distToCamera);
    specular *= (distFade * nearFade * 0.35);

    // ==========================================================
    // WATER DEPTH & SHORELINE COLOR BLENDING
    // ==========================================================
    vec2 ndc = (clipSpacePos.xy / clipSpacePos.w) * 0.5 + 0.5;
    float rawSceneDepth = texture(depthMap, ndc).r;
    float sceneDepth = linearizeDepth(rawSceneDepth);
    float waterDepth = linearizeDepth(gl_FragCoord.z);
    float depthDifference = max(sceneDepth - waterDepth, 0.0);
    
    float waterDepthFade = clamp(depthDifference / 1.5, 0.0, 1.0);
    waterDepthFade = smoothstep(0.0, 1.0, waterDepthFade);
    
    vec3 waterBaseColor = vec3(0.005, 0.05, 0.12); // Deep end
    vec3 shallowColor = vec3(0.0, 0.3, 0.4);       // Near shore
    vec3 waterBodyColor = mix(shallowColor, waterBaseColor, waterDepthFade);

    // ==========================================================
    // DYNAMIC PLANAR REFLECTION WITH DEPTH MASKING
    // ==========================================================
    vec2 reflectNDC = ndc;
    vec2 offset = vec2(rippleNormal.x, rippleNormal.z) * 0.025;
    vec2 distortedNDC = clamp(reflectNDC + offset, 0.0, 1.0);

    // Read the depth behind our proposed distorted reflection coordinate
    float distortedRawDepth = texture(depthMap, distortedNDC).r;
    float distortedSceneDepth = linearizeDepth(distortedRawDepth);
    
    // FIX: If the distortion offset samples pixels in front of the water (like dry island edges),
    // cancel the offset projection back to a clean undistorted look to remove clipping bleeding.
    if (distortedSceneDepth < waterDepth) {
        distortedNDC = ndc;
    }

    distortedNDC.y = 1.0 - distortedNDC.y; // Match screen space reflection texture requirements
    vec3 dynamicReflection = texture(reflectionMap, distortedNDC).rgb;

    // ==========================================================
    // SHADER COMPOSITION & TONEMAPPING
    // ==========================================================
    vec3 result = mix(waterBodyColor, dynamicReflection, fresnel);
    
    result += sss;
    result += specular * sunLightColour;
    result = min(result, vec3(2.0)); 

    result = ACESFilm(result);
    result = pow(result, vec3(1.0 / 2.2));
    
    // ==========================================================
    // FINAL TRANSPARENCY & UNDERWATER FOG
    // ==========================================================
    // Increased opacity scaling coefficient from 0.85 to 1.8 
    // to give the water body density and obscure deep parts gracefully.
    float waterVisibilityFog = 1.0 - exp(-depthDifference * 1.8);
    waterVisibilityFog = clamp(waterVisibilityFog, 0.0, 1.0);

    float finalAlpha = mix(waterVisibilityFog * waterDepthFade, 1.0, fresnel);

    finalColor = vec4(result, finalAlpha);
}
