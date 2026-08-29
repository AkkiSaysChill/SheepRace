#version 330 core
out vec4 FragColor;

in vec2 fragTexCoord;

uniform sampler2D texture0;
uniform float time; // Time uniform for animating noise and scanlines

// Function to generate a random float
float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main()
{
    // Base color from the scene
    vec3 col = texture(texture0, fragTexCoord).rgb;

    // 1. Desaturation
    float gray = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(col, vec3(gray), 0.8); // 80% desaturation

    // 2. Vignette
    vec2 uv = fragTexCoord - 0.5;
    float vignette = 1.0 - dot(uv, uv) * 1.5; // Adjust multiplier for strength
    col *= vignette;

    // 3. Film Grain / Noise
    float noise = (rand(fragTexCoord + time) - 0.5) * 0.2; // Adjust multiplier for strength
    col += noise;

    // 4. Scanlines
    float scanline = sin((fragTexCoord.y + time * 0.1) * 300.0) * 0.05; // Adjust for frequency and strength
    col -= scanline;

    // 5. Color Tint (optional, uncomment to use)
    // col *= vec3(1.0, 0.8, 0.8); // Example: reddish tint

    // Clamp final color to avoid weird artifacts
    col = clamp(col, 0.0, 1.0);

    FragColor = vec4(col, 1.0);
}
