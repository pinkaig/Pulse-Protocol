#version 450 core

layout(location = 0) out vec4 fragColor;

uniform vec4 uOverlayColor;

void main()
{
    fragColor = uOverlayColor;
}
