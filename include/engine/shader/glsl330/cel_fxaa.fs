#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;

// --- Comic Inking Parameters ---
const float EDGE_THRESHOLD = 0.22; // Inks silhouettes and contours cleanly
const vec3  INK_COLOR      = vec3(0.04, 0.05, 0.08);

// FXAA tuning constants
const float FXAA_REDUCE_MIN = 1.0 / 128.0;
const float FXAA_REDUCE_MUL = 1.0 / 8.0;
const float FXAA_SPAN_MAX   = 8.0;

vec3 SampleCel(vec2 uv, vec2 texelSize)
{
    vec3 c = texture(texture0, uv).rgb;

    // 1. Sobel Edge Detection (Evaluated across neighbor luminance)
    vec3 lumaWeight = vec3(0.299, 0.587, 0.114);
    
    float tL = dot(texture(texture0, uv + vec2(-texelSize.x,  texelSize.y)).rgb, lumaWeight);
    float t  = dot(texture(texture0, uv + vec2( 0.0,          texelSize.y)).rgb, lumaWeight);
    float tR = dot(texture(texture0, uv + vec2( texelSize.x,  texelSize.y)).rgb, lumaWeight);
    float l  = dot(texture(texture0, uv + vec2(-texelSize.x,  0.0)).rgb,         lumaWeight);
    float r  = dot(texture(texture0, uv + vec2( texelSize.x,  0.0)).rgb,         lumaWeight);
    float bL = dot(texture(texture0, uv + vec2(-texelSize.x, -texelSize.y)).rgb, lumaWeight);
    float b  = dot(texture(texture0, uv + vec2( 0.0,         -texelSize.y)).rgb, lumaWeight);
    float bR = dot(texture(texture0, uv + vec2( texelSize.x, -texelSize.y)).rgb, lumaWeight);

    float gx = (tR + 2.0 * r + bR) - (tL + 2.0 * l + bL);
    float gy = (tL + 2.0 * t + tR) - (bL + 2.0 * b + bR);
    float edge = sqrt(gx * gx + gy * gy);

    // 2. Selective Outline Suppression (Mutes wave sparkle on open water, preserves silhouettes)
    bool isWater = (c.b > c.r * 1.35 && c.b > 0.35 && c.g < 0.65);
    float threshold = isWater ? (EDGE_THRESHOLD * 2.6) : EDGE_THRESHOLD;

    // 3. Subtle Comic Contrast (Sharpens shadow borders without wiping out island colors)
    float luma = dot(c, lumaWeight);
    float toonBoost = smoothstep(0.18, 0.24, luma) * 0.08;
    vec3 outColor = c + toonBoost;

    // 4. Inking Pass
    if (edge > threshold) {
        float inkStrength = smoothstep(threshold, threshold * 1.5, edge);
        return mix(outColor, INK_COLOR, inkStrength * 0.90);
    }

    return outColor;
}

void main()
{
    vec2 texelSize = 1.0 / resolution;

    // 1. Gather neighborhood with comic inking
    vec3 rgbNW = SampleCel(fragTexCoord + vec2(-1.0, -1.0) * texelSize, texelSize);
    vec3 rgbNE = SampleCel(fragTexCoord + vec2( 1.0, -1.0) * texelSize, texelSize);
    vec3 rgbSW = SampleCel(fragTexCoord + vec2(-1.0,  1.0) * texelSize, texelSize);
    vec3 rgbSE = SampleCel(fragTexCoord + vec2( 1.0,  1.0) * texelSize, texelSize);
    vec3 rgbM  = SampleCel(fragTexCoord, texelSize);

    // 2. FXAA Edge Filtering
    vec3 lumaWeight = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, lumaWeight);
    float lumaNE = dot(rgbNE, lumaWeight);
    float lumaSW = dot(rgbSW, lumaWeight);
    float lumaSE = dot(rgbSE, lumaWeight);
    float lumaM  = dot(rgbM,  lumaWeight);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = min(vec2(FXAA_SPAN_MAX), max(vec2(-FXAA_SPAN_MAX), dir * rcpDirMin)) * texelSize;

    vec3 rgbA = 0.5 * (
        SampleCel(fragTexCoord + dir * (1.0 / 3.0 - 0.5), texelSize) +
        SampleCel(fragTexCoord + dir * (2.0 / 3.0 - 0.5), texelSize));
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        SampleCel(fragTexCoord + dir * -0.5, texelSize) +
        SampleCel(fragTexCoord + dir *  0.5, texelSize));

    float lumaB = dot(rgbB, lumaWeight);
    finalColor = vec4(((lumaB < lumaMin) || (lumaB > lumaMax)) ? rgbA : rgbB, 1.0);
}
