#version 450 core
in vec2 TexCoords;
out vec4 color;
uniform sampler2D text;
uniform vec4 textColor;
void main() {
    float mask = texture(text, TexCoords).r;   // <- RED channel
    color = vec4(textColor.rgb, textColor.a * mask);
}
