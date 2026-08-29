#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 lightDir;
uniform vec4 lightColor;
uniform vec4 ambient;
uniform vec3 viewPos;

uniform mat4 lightVP;
uniform sampler2D shadowMap;
uniform int shadowMapResolution;

uniform float u_shininess;
uniform float u_specularStrength;

out vec4 finalColor;

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec3 normal = normalize(fragNormal);
    vec3 viewD = normalize(viewPos - fragPosition);
    vec3 l = normalize(-lightDir); // Ensure light direction is normalized

    
    
    // Instead of: float NdotL = max(dot(normal, l), 0.0);
    float NdotL = dot(normal, l);
    float intensity = smoothstep(0.0, 0.05, NdotL); // Sharp transition at 0
    if (intensity > 0.9) intensity = 1.0;
    else if (intensity > 0.5) intensity = 0.6;
    else intensity = 0.3; // Shadow band
    float softDiffuse = NdotL * 0.5 + 0.5; 
    vec3 diffuseLight = lightColor.rgb * softDiffuse;

    
    vec3 specular = vec3(0.0);
    if (NdotL > 0.0)
    {
        // Using Blinn-Phong (Halfway vector) for more stable highlights
        vec3 halfwayDir = normalize(l + viewD);
        float specCo = pow(max(0.0, dot(normal, halfwayDir)), u_shininess);
        specular = lightColor.rgb * specCo * u_specularStrength;
    }

    
    vec4 fragPosLightSpace = lightVP * vec4(fragPosition, 1.0);
    fragPosLightSpace.xyz /= fragPosLightSpace.w;
    fragPosLightSpace.xyz = fragPosLightSpace.xyz * 0.5 + 0.5;
    
    float shadow = 0.0;
    float bias = max(0.005 * (1.0 - NdotL), 0.0005);
    float curDepth = fragPosLightSpace.z;

    // Use a larger 5x5 filter for softer shadow edges
    vec2 texelSize = 1.0 / vec2(float(shadowMapResolution));
    for(int x = -2; x <= 2; ++x)
    {
        for(int y = -2; y <= 2; ++y)
        {
            float pcfDepth = texture(shadowMap, fragPosLightSpace.xy + vec2(x, y) * texelSize).r; 
            shadow += (curDepth - bias > pcfDepth) ? 1.0 : 0.0;        
        }    
    }
    shadow /= 25.0; 

    // Mix diffuse and ambient naturally
    vec3 lighting = (ambient.rgb + (1.0 - shadow) * diffuseLight) * colDiffuse.rgb;
    vec3 finalRGB = (texelColor.rgb * lighting) + ((1.0 - shadow) * specular);

    finalColor = vec4(finalRGB, texelColor.a);

    // Gamma correction
    finalColor.rgb = pow(finalColor.rgb, vec3(1.0/2.2));
}