#version 330 core

// --- Uniforms from Raylib (C++) ---
uniform float iTime;
uniform vec2 iResolution;
//uniform vec2 iMouse; // REMOVED - No longer used for camera
uniform sampler2D iChannel0;
uniform vec3 u_lightDir;
uniform float u_cloudCover;
uniform float u_cloudFalloff;
// --- NEW CAMERA UNIFORMS ---
uniform vec3 u_cameraPos;
uniform vec3 u_cameraForward;
uniform vec3 u_cameraUp;
uniform vec3 u_cameraRight;
uniform float u_cameraFovY; // Vertical FOV in radians


// Inputs from Raylib's default vertex shader
in vec2 fragTexCoord;
in vec4 fragColor;

// The final output color for this pixel
out vec4 finalColor;

// --- (Noise, FBM, Sky functions remain mostly the same) ---
// --- NO CHANGES to random, noise, fbm ---
float random(in vec2 uv) { return texture(iChannel0, uv / 64.).r; }
float noise(in vec2 uv) { /* ... same ... */ vec2 i = floor(uv); vec2 f = fract(uv); f = f*f*(3.-2.*f); float lb=random(i+vec2(0,0)); float rb=random(i+vec2(1,0)); float lt=random(i+vec2(0,1)); float rt=random(i+vec2(1,1)); return mix(mix(lb,rb,f.x), mix(lt,rt,f.x), f.y); }
#define OCTAVES 8
float fbm(in vec2 uv) { /* ... same ... */ float v=0.; float a=.5; for(int i=0;i<OCTAVES;i++){v+=noise(uv)*a;a*=.5;uv*=2.;} return v; }

vec3 Sky(in vec3 ro, in vec3 rd) {
    // This function remains exactly the same, it just uses the ro and rd passed to it.
    const float SC = 1e5;
    float dist = (SC - ro.y) / (rd.y + 1e-6); // Added epsilon to avoid division by zero if looking perfectly horizontal
    vec2 p = (ro + dist * rd).xz;
    p *= 1.2 / SC;
    vec3 lightDir = normalize(u_lightDir);
    float sundot = clamp(dot(rd, lightDir), 0.0, 1.0);
    vec3 cloudCol = vec3(1.);
    vec3 skyCol = vec3(0.3,0.5,0.85) - rd.y*rd.y*0.5;
    skyCol = mix( skyCol, 0.85 * vec3(0.7,0.75,0.85), pow( 1.0 - max(rd.y, 0.0), 4.0 ) );
    vec3 sun = 0.25*vec3(1.0,0.7,0.4)*pow(sundot,5.0)+0.25*vec3(1.0,0.8,0.6)*pow(sundot,64.0)+0.2*vec3(1.0,0.8,0.6)*pow(sundot,512.0);
    skyCol += sun;
    float t = iTime * 0.1;
    float den = fbm(vec2(p.x - t, p.y - t));
    skyCol = mix( skyCol, cloudCol, smoothstep(u_cloudCover, u_cloudFalloff, den));
    skyCol = mix( skyCol, 0.68 * vec3(.418, .394, .372), pow( 1.0 - max(rd.y, 0.0), 16.0 ) );
    return skyCol;
}

// --- REMOVE setCamera function ---
// mat3 setCamera( in vec3 ro, in vec3 ta, float cr ) { ... } // DELETE THIS


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord.xy / iResolution.xy) * 2.0 - 1.0; // NDC coords (-1 to 1)

    // Calculate the direction vector for this pixel in view space
    float aspectRatio = iResolution.x / iResolution.y;
    float tanHalfFovY = tan(u_cameraFovY * 0.5);
    vec3 viewRayDir = normalize(vec3(uv.x * aspectRatio * tanHalfFovY,
                                     uv.y * tanHalfFovY,
                                     1.0)); // Assuming negative Z is forward in view space

    // Transform the view space direction to world space using camera basis vectors
    vec3 rd = normalize(u_cameraRight * viewRayDir.x + u_cameraUp * viewRayDir.y + u_cameraForward * viewRayDir.z);

    vec3 ro = u_cameraPos;


    /*
    vec2 uv = fragCoord.xy / iResolution.xy;
    uv -= 0.5;
    uv.x *= iResolution.x / iResolution.y;
    vec2 mouse = iMouse.xy/iResolution.xy;
    vec3 ro = vec3(0.0, 0.0, 0.0);
    vec3 ta = vec3(cos(mouse.x * 6.28), mouse.y * 2.0 - 1.0, sin(mouse.x * 6.28));
    mat3 cam = setCamera(ro, ta, 0.0);
    vec3 rd = normalize(cam * vec3(uv, 1.0));
    */

    vec3 col = Sky(ro, rd);

    fragColor = vec4(vec3(col),1.0);
}

// --- (Wrapper main function is unchanged) ---
void main()
{
    vec4 shadertoyOutput;
    vec2 shadertoyInput_fragCoord = gl_FragCoord.xy;
    mainImage(shadertoyOutput, shadertoyInput_fragCoord);
    finalColor = shadertoyOutput;
}