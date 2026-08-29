#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
in float fragIsRing; // <--- VS SE AAYA HUA FLAG

out vec4 finalColor;

void main()
{
    vec2 center = vec2(0.5, 0.5);
    float dist = length(fragTexCoord - center);

    if (dist > 0.5) discard;

    float alphaMask = 0.0;

    if (fragIsRing > 0.5) {
        // ==========================================
        // SHOCKWAVE MATH: Sirf Outline (Hollow Ring)
        // ==========================================
        float ringRadius = 0.45; // Ring kahan hogi
        float thickness = 0.05;  // Ring kitni moti hogi
        float distFromRing = abs(dist - ringRadius); // Ring center se current pixel ki doori
        
        // Agar pixel ring se door hai, toh mask 0 ho jayega
        alphaMask = smoothstep(thickness, 0.0, distFromRing);
    } 
    else {
        // ==========================================
        // NORMAL MATH: Bhara hua gola (Smoke/Sparks)
        // ==========================================
        alphaMask = smoothstep(0.5, 0.1, dist);
    }

    finalColor = fragColor;
    finalColor.a *= alphaMask;
    finalColor.rgb *= 1.5; // Glow boost
}
