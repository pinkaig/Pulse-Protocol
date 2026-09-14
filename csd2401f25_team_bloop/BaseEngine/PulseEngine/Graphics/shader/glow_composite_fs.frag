#version 450 core

in  vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uGlowTex;
uniform float     uGlowIntensity;

void main()
{
    vec4 glow = texture(uGlowTex, TexCoord);
    FragColor = glow * uGlowIntensity;
}
