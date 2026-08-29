
#version 330

#define PI 3.14159265358979323846

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
in mat3 TBN; // Tangent-Bitangent-Normal matrix for normal mapping

// Output fragment color
out vec4 finalColor;

// --- PBR Material Uniforms ---
// uniform sampler2D albedoMap;
uniform sampler2D texture0;
uniform sampler2D mraMap;      // Metallic(R), Roughness(G), AO(B)
uniform sampler2D normalMap;
uniform sampler2D emissiveMap; // r: Height g:emissive

uniform vec2 tiling;
uniform vec2 offset;

uniform int useTexAlbedo;
uniform int useTexNormal;
uniform int useTexMRA;
uniform int useTexEmissive;

uniform vec4  albedoColor;
uniform vec4  emissiveColor;
uniform float normalValue;
uniform float metallicValue;
uniform float roughnessValue;
uniform float aoValue;
uniform float emissivePower;

// --- Lighting & View Uniforms ---
uniform vec3 viewPos;
uniform vec3 ambientColor; // From PBR
uniform float ambient;     // From PBR

// --- Directional Light & Shadow Uniforms (from shadowmap.fs) ---
uniform vec3 lightDir;     // Direction *to* the light
uniform vec4 lightColor;   // Color and intensity
uniform mat4 lightVP;      // Light's View-Projection matrix
uniform sampler2D shadowMap;
uniform int shadowMapResolution;


// --- PBR Helper Functions (copied from pbr.fs) ---

vec3 SchlickFresnel(float hDotV,vec3 refl)
{
    return refl + (1.0 - refl)*pow(1.0 - hDotV, 5.0);
}

float GgxDistribution(float nDotH,float roughness)
{
    float a = roughness*roughness*roughness*roughness;
    float d = nDotH*nDotH*(a - 1.0) + 1.0;
    d = PI*d*d;
    return (a/max(d,0.0000001));
}

float GeomSmith(float nDotV,float nDotL,float roughness)
{
    float r = roughness + 1.0;
    float k = r*r/8.0;
    float ik = 1.0 - k;
    float ggx1 = nDotV/(nDotV*ik + k);
    float ggx2 = nDotL/(nDotL*ik + k);
    return ggx1*ggx2;
}


void main()
{
    // --- 1. Get Material Properties (from pbr.fs) ---
    vec3 albedo = albedoColor.rgb; // Start with the flat color
    if (useTexAlbedo == 1)
    {
        // If using texture, multiply texture by the flat color (for tinting)
        albedo *= texture(texture0, vec2(fragTexCoord.x*tiling.x + offset.x, fragTexCoord.y*tiling.y + offset.y)).rgb;
    }


    float metallic = clamp(metallicValue, 0.0, 1.0);
    float roughness = clamp(roughnessValue, 0.0, 1.0);
    float ao = clamp(aoValue, 0.0, 1.0);
    
    if (useTexMRA == 1)
    {
        vec4 mra = texture(mraMap, vec2(fragTexCoord.x*tiling.x + offset.x, fragTexCoord.y*tiling.y + offset.y));
        metallic = clamp(mra.r + metallicValue, 0.04, 1.0);
        roughness = clamp(mra.g + roughnessValue, 0.04, 1.0);
        ao = (mra.b + aoValue)*0.5;
    }

    // --- 2. Get Fragment Normal (from pbr.fs) ---
    // N is the final normal in world space
    vec3 N = normalize(fragNormal);
    if (useTexNormal == 1)
    {
        // Get normal from texture, convert from [0, 1] to [-1, 1]
        vec3 texNormal = texture(normalMap, vec2(fragTexCoord.x*tiling.x + offset.x, fragTexCoord.y*tiling.y + offset.y)).rgb;
        texNormal = normalize(texNormal*2.0 - 1.0);
        // Transform from tangent space to world space
        N = normalize(TBN * texNormal);
    }

    // --- 3. Get View Vector (from pbr.fs) ---
    vec3 V = normalize(viewPos - fragPosition);

    // --- 4. Get Base Reflectivity (from pbr.fs) ---
    // If dia-electric (non-metal) use 0.04, if metal use albedo
    vec3 baseRefl = mix(vec3(0.04), albedo.rgb, metallic);

    // --- 5. Calculate PBR Direct Light (modified from pbr.fs) ---
    vec3 L = normalize(-lightDir); // Direction *from* light source
    vec3 H = normalize(V + L);     // Halfway vector
    vec3 radiance = lightColor.rgb; // Light's color and intensity

    // Cook-Torrance BRDF
    float nDotV = max(dot(N,V), 0.000001);
    float nDotL = max(dot(N,L), 0.000001);
    float hDotV = max(dot(H,V), 0.0);
    float nDotH = max(dot(N,H), 0.0);
    
    float D = GgxDistribution(nDotH, roughness);
    float G = GeomSmith(nDotV, nDotL, roughness);
    vec3  F = SchlickFresnel(hDotV, baseRefl);

    // Calculate specular part
    vec3 spec = (D*G*F)/(4.0*nDotV*nDotL + 0.0001); // Epsilon to avoid divide by zero
    
    // Calculate diffuse part
    vec3 kD = vec3(1.0) - F;      // Diffuse = 1.0 - Specular
    kD *= 1.0 - metallic;         // Non-metals have diffuse
    
    // Total direct light (sum of diffuse and specular)
    vec3 directLight = ((kD*albedo.rgb/PI + spec)*radiance*nDotL);
    
    // --- 6. Calculate Shadow (from shadowmap.fs) ---
    vec4 fragPosLightSpace = lightVP * vec4(fragPosition, 1);
    fragPosLightSpace.xyz /= fragPosLightSpace.w;
    fragPosLightSpace.xyz = (fragPosLightSpace.xyz + 1.0)/2.0;
    vec2 sampleCoords = fragPosLightSpace.xy;
    float curDepth = fragPosLightSpace.z;

    // Use N (our final normal) and L (our light dir)
    float bias = max(0.0002*(1.0 - dot(N, L)), 0.00002) + 0.00001;
    
    int shadowCounter = 0;
    const int numSamples = 9;
    vec2 texelSize = vec2(1.0/float(shadowMapResolution));

    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            float sampleDepth = texture(shadowMap, sampleCoords + texelSize*vec2(x, y)).r;
            if (curDepth - bias > sampleDepth) shadowCounter++;
        }
    }
    // Convert counter to a 0.0 (full shadow) to 1.0 (full light) multiplier
    float shadowAmount = 1.0 - (float(shadowCounter)/float(numSamples));

    // --- 7. Calculate Emissive (from pbr.fs) ---
    vec3 emissive = vec3(0);
    if (useTexEmissive == 1) {
        emissive = (texture(emissiveMap, vec2(fragTexCoord.x*tiling.x + offset.x, fragTexCoord.y*tiling.y + offset.y)).rgb).g*emissiveColor.rgb*emissivePower;
    }
    
    // --- 8. Calculate Ambient (from pbr.fs) ---
   // NEW
    vec3 ambientFinal = albedo * ambientColor * ambient;
    
    // --- 9. Combine Everything ---
    // Ambient light
    // + (Direct light * Ambient Occlusion * Shadow)
    // + Emissive light
    vec3 color = ambientFinal + (directLight * ao * shadowAmount) + emissive;
    
    // --- 10. Gamma Correction ---
    color = pow(color, vec3(1.0/2.2));
    finalColor = vec4(color, 1.0);
}