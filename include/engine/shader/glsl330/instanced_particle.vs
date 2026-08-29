#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in mat4 instanceTransform;

uniform mat4 mvp;
uniform mat4 view;

out vec2 fragTexCoord;
out vec4 fragColor;
out float fragIsRing; // <--- YEH NAYA ADD KIYA

void main()
{
    vec3  instancePos   = instanceTransform[0].xyz;
    float instanceSize  = instanceTransform[0].w;
    vec4  instanceColor = instanceTransform[1];
    float rotation      = instanceTransform[2].x;
    float billboardMode = instanceTransform[2].y;
    
    // Yahan hum m10 (col2.z) se isRing flag extract kar rahe hain
    fragIsRing          = instanceTransform[2].z; 

    vec3 camRight = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 camUp    = vec3(view[0][1], view[1][1], view[2][1]);

    vec3 billRight = mix(camRight, vec3(camRight.x, 0.0, camRight.z), billboardMode);
    vec3 billUp    = mix(camUp,    vec3(0.0, 1.0, 0.0),               billboardMode);

    if (fragIsRing > 0.5) {
        billRight = vec3(1.0, 0.0, 0.0); // World X Axis
        billUp    = vec3(0.0, 0.0, 1.0); // World Z Axis
    }

    billRight = normalize(billRight);
    billUp    = normalize(billUp);

    float cosR = cos(rotation);
    float sinR = sin(rotation);
    vec3 rotatedRight = cosR * billRight - sinR * billUp;
    vec3 rotatedUp    = sinR * billRight + cosR * billUp;

    vec3 worldPos = instancePos
        + (rotatedRight * vertexPosition.x * instanceSize)
        + (rotatedUp    * vertexPosition.z * instanceSize);

    gl_Position  = mvp * vec4(worldPos, 1.0);
    fragTexCoord = vertexTexCoord;
    fragColor    = instanceColor;
}
