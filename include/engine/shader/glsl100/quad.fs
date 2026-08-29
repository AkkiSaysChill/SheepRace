#version 100

precision mediump float;

varying vec2 fragTexCoord;

uniform sampler2D sceneTexture;
uniform sampler2D depthTexture;
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;
uniform mat4 invViewProj;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture2D(shadowMap, projCoords.xy).r;
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float shadow = currentDepth > closestDepth  ? 1.0 : 0.0;

    return shadow;
}

void main()
{
    float depth = texture2D(depthTexture, fragTexCoord).r;
    vec4 worldPos = invViewProj * vec4(fragTexCoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    worldPos /= worldPos.w;

    vec4 fragPosLightSpace = lightSpaceMatrix * worldPos;
    float shadow = ShadowCalculation(fragPosLightSpace);
    vec4 finalColor = texture2D(sceneTexture, fragTexCoord);
    gl_FragColor = vec4(finalColor.rgb * (1.0 - shadow), finalColor.a);
}