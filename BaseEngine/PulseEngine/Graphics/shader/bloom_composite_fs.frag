#version 450 core

in  vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uScene;
uniform sampler2D uBloom;
uniform float     uBloomIntensity;

void main()
{
    vec3 scene = texture(uScene, TexCoord).rgb;
    vec3 bloom = texture(uBloom, TexCoord).rgb;
    FragColor = vec4(scene + bloom * uBloomIntensity, 1.0);
}
