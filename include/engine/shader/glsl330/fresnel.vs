#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

uniform float scroll;

out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;
out vec4 clipSpacePos;

void main()
{
    vec3 pos = vertexPosition;
    
    // Procedural Vertex Waves
    float wave1 = sin(pos.x * 0.5 + (scroll * 2.0)) * 0.5;
    float wave2 = cos(pos.z * 0.3 + (scroll * 1.5)) * 0.5;
    pos.y = wave1 + wave2;
    
    fragPosition  = vec3(matModel * vec4(pos, 1.0));
    fragTexCoord  = vertexTexCoord;
    fragColor     = vertexColor;

    fragNormal    = normalize(mat3(matModel) * vec3(0.0, 1.0, 0.0));

    // Derive clipSpacePos from gl_Position so both always match.
    gl_Position = mvp * vec4(pos, 1.0);
    clipSpacePos = gl_Position;
}
