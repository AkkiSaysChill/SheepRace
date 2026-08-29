#version 330
in vec3 vertexPosition;
in vec2 vertexTexcoord;
in vec3 vertexNormal;
in vec4 vertexColor;

out vec2 fragTexcoord;
out vec3 fragNormal;
out vec3 fragPosition;
out vec4 fragColor;

// New uniform and output for shadow mapping
uniform mat4 lightSpaceMatrix;
out vec4 lightSpacePos;

// NOTE: We need to define standard Raylib uniforms for its drawing functions to work
uniform mat4 mvp; 
uniform mat4 model;

void main()
{
    // Pass transformed normal and position to fragment shader
    // Note: The model matrix is expected to be updated by DrawModel or BeginShaderMode for primitives
    fragNormal = mat3(transpose(inverse(model))) * vertexNormal;
    fragPosition = (model * vec4(vertexPosition, 1.0)).xyz;
    fragTexcoord = vertexTexcoord;
    fragColor = vertexColor;
    
    // Transform vertex position into light space coordinates for shadow mapping
    lightSpacePos = lightSpaceMatrix * vec4(vertexPosition, 1.0);
    
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
