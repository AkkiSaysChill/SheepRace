#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 lightDir;
uniform vec4 lightColor;
uniform vec3 viewPos;

uniform mat4 lightVP;
uniform sampler2D shadowMap;
uniform int shadowMapResolution;
uniform float u_shininess;
uniform float u_specularStrength;

// Unity Tri-Color Gradient Ambient
uniform vec3 u_skyColor;      // e.g. vec3(0.35, 0.55, 0.85)
uniform vec3 u_equatorColor;  // e.g. vec3(0.50, 0.55, 0.60)
uniform vec3 u_groundColor;   // e.g. vec3(0.12, 0.10, 0.08)

out vec4 finalColor;

// Unity Tri-Color GI Ambient model
vec3 CalculateAmbientGI(vec3 normal)
{
    float up = normal.y;
    if (up > 0.0) {
        return mix(u_equatorColor, u_skyColor, up);
    } else {
        return mix(u_equatorColor, u_groundColor, -up);
    }
}

// Schlick Fresnel Rim
float CalculateFresnel(vec3 normal, vec3 viewDir, float F0)
{
    float NdotV = max(dot(normal, viewDir), 0.0);
    return F0 + (1.0 - F0) * pow(1.0 - NdotV, 4.0);
}

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord) * colDiffuse * fragColor;
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(viewPos - fragPosition);
    vec3 l = normalize(-lightDir);

    // 1. Direct Lambertian Diffuse (Smooth energy conservation)
    float NdotL = max(dot(normal, l), 0.0);
    vec3 directDiffuse = lightColor.rgb * NdotL;

    // 2. Specular Blinn-Phong with Fresnel Highlight
    vec3 specular = vec3(0.0);
    if (NdotL > 0.0)
    {
        vec3 halfwayDir = normalize(l + viewDir);
        float NdotH = max(dot(normal, halfwayDir), 0.0);
        float specFactor = pow(NdotH, max(u_shininess, 1.0));
        specular = lightColor.rgb * specFactor * u_specularStrength;
    }

    // 3. Shadow Map PCF Filtering
    vec4 fragPosLightSpace = lightVP * vec4(fragPosition, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float shadow = 0.0;
    if (projCoords.z <= 1.0)
    {
        float bias = max(0.003 * (1.0 - NdotL), 0.0005);
        vec2 texelSize = 1.0 / vec2(float(shadowMapResolution));
        
        for (int x = -1; x <= 1; ++x)
        {
            for (int y = -1; y <= 1; ++y)
            {
                float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
                shadow += (projCoords.z - bias > pcfDepth) ? 1.0 : 0.0;
            }
        }
        shadow /= 9.0;
    }

    // 4. Ambient GI & Grazing Fresnel Rim
    vec3 ambientGI = CalculateAmbientGI(normal);
    float fresnelRim = CalculateFresnel(normal, viewDir, 0.04) * (1.0 - shadow);

    // 5. Lighting Composition
    vec3 litColor = (ambientGI + (1.0 - shadow) * directDiffuse) * texelColor.rgb;
    litColor += ((1.0 - shadow) * specular) + (fresnelRim * u_skyColor * 0.2);

    // Linear-to-sRGB Gamma Correction
    finalColor = vec4(pow(litColor, vec3(1.0 / 2.2)), texelColor.a);
}
