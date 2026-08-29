#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexTangent; // We need this for normal mapping

// Input uniform values
uniform mat4 mvp;      // Model-View-Projection matrix
uniform mat4 matModel; // Model matrix
uniform mat3 matNormal;  // Normal matrix (inverse-transpose of matModel)

// Output vertex attributes (to fragment shader)
out vec3 fragPosition; // Vertex position in world space
out vec2 fragTexCoord;
out vec3 fragNormal;
out mat3 TBN;          // Tangent-Bitangent-Normal matrix

void main()
{
    // Send vertex position to fragment shader (in world space)
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    
    // Send texture coordinates to fragment shader
    fragTexCoord = vertexTexCoord;
    
    // Send vertex normal to fragment shader (in world space)
    fragNormal = normalize(matNormal * vertexNormal);

    // Calculate the TBN matrix for normal mapping
    // T = Tangent, B = Bitangent, N = Normal
    vec3 T = normalize(matNormal * vertexTangent.xyz);
    vec3 N = fragNormal;
    
    // Re-orthogonalize T with respect to N (Gram-Schmidt)
    T = normalize(T - dot(T, N) * N);
    
    // Get B (Bitangent)
    // The vertexTangent.w component stores the handedness of the bitangent
    vec3 B = cross(N, T) * vertexTangent.w;

    TBN = mat3(T, B, N);

    // Calculate vertex position in clip space
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}