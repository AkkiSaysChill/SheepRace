#version 330
in vec2 fragTexcoord;
in vec3 fragNormal;
in vec3 fragPosition;
in vec4 fragColor;
in vec4 lightSpacePos; // Position in light's space

out vec4 finalColor;

// Uniforms for lighting
uniform vec3 viewPos;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform float ambientStrength;

// Uniform for shadow mapping
uniform sampler2D shadowMap;

uniform sampler2D textureSampler;

uniform float shadowBias; // New uniform for adjustable bias

float calculateShadow() {
    // Perform perspective division and scale to [0, 1] range
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w; 
    projCoords = projCoords * 0.5 + 0.5;

    // If the fragment is outside the light's frustum (0-1 range), it's fully lit
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) return 1.0;
    
    // Get closest depth value from light's perspective (shadow map)
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    
    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
    // Compare depth values
    float shadow = step(closestDepth + shadowBias, currentDepth); 
    
    // Shadow is attenuated lighting (0.1 means 90% light removed, very dark shadow)
    return mix(1.0, 0.1, shadow); 
}


void main()
{
    // Normalize vectors
    vec3 norm = normalize(fragNormal);
    vec3 lightDirN = normalize(lightDir);

    // 1. Ambient Lighting
    vec3 ambient = ambientStrength * lightColor;

    // 2. Diffuse Lighting
    float diff = max(dot(norm, -lightDirN), 0.0); 
    vec3 diffuse = diff * lightColor;

    // 3. Shadow Calculation
    float shadowFactor = calculateShadow();
    
    // 4. Combine: Ambient + (Shadow * Diffuse)
    vec3 totalLighting = (ambient + shadowFactor * diffuse);

    // Apply lighting to the final color
    vec4 textureColor = texture(textureSampler, fragTexcoord); 
    finalColor = vec4(totalLighting, 1.0) * textureColor * fragColor;
}
