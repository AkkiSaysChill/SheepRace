#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal; // Input normal

out vec2 fragTexCoord;
out vec3 fragNormal;
out vec3 fragPosition;

uniform mat4 mvp;
uniform mat4 matModel;

void main()
{
    // Transform vertex position to world space
    fragPosition = (matModel * vec4(vertexPosition, 1.0)).xyz;

    // Transform and normalize the normal vector
    fragNormal = normalize(mat3(matModel) * vertexNormal);

    fragTexCoord = vertexTexCoord;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
