#version 330

// Inputs from vertex shader
in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float time; 

out vec4 finalColor;

// Ek desi random number generator 
float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main()
{
    vec2 uv = fragTexCoord;

    float glitchSpeed = time * 15.0; 
    float jitter = (rand(vec2(uv.y, glitchSpeed)) - 0.5) * 0.03; // 0.03 is the intensity
    
    // Sirf randomly screen phate, hamesha nahi (for more authentic glitch)
    if (rand(vec2(time, time)) > 0.8) {
        uv.x += jitter;
    }

    // 2. CHROMATIC ABERRATION (RGB Split)
    // Red, Green aur Blue channels ko alag-alag sample karenge taaki 3D glasses jaisa effect aaye
    float offset = 0.008 * sin(time * 10.0); // Offset amount
    
    float r = texture(texture0, uv + vec2(offset, 0.0)).r;
    float g = texture(texture0, uv).g;
    float b = texture(texture0, uv - vec2(offset, 0.0)).b;

    vec4 texelColor = vec4(r, g, b, 1.0);

    // Apply lighting/tint if any
    finalColor = texelColor * colDiffuse * fragColor;
}
