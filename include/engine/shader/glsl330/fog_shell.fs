#version 330

in vec3 v_worldPos;
uniform vec3 u_camPos;

out vec4 finalColor;

// Atmospheric horizon color (matches your sky gradient)
const vec3 FOG_COLOR = vec3(0.55, 0.65, 0.82);

// Distance range where mist accumulates
const float FOG_NEAR = 1500.0;
const float FOG_FAR  = 6200.0;

void main()
{
    // Horizontal distance across the horizon
    float dist = length(v_worldPos.xz - u_camPos.xz);

    // Smooth exponential distance ramp
    float distFactor = clamp((dist - FOG_NEAR) / (FOG_FAR - FOG_NEAR), 0.0, 1.0);
    float alpha = distFactor * distFactor * (3.0 - 2.0 * distFactor);

    // Vertical atmospheric falloff: dense near sea level, fades out into the upper sky
    // This ensures the sky directly above remains completely open and blue
    float altitude = v_worldPos.y;
    float verticalFade = exp(-max(0.0, altitude) * 0.0012);

    alpha *= verticalFade * 0.85; // Capped at 85% so land silhouetting stays natural

    finalColor = vec4(FOG_COLOR, alpha);
}
