#version 450 core

in  vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uScene;
uniform float     uThreshold;

void main()
{
    vec3 color = texture(uScene, TexCoord).rgb;
    float lum = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (lum > uThreshold)
        FragColor = vec4(color, 1.0);
    else
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
