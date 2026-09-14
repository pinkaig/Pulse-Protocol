#version 450 core
layout(location = 0) in vec2 aVertexPosition;
layout(location = 1) in vec4 aVertexColor;    // Tint color (RGBA)
layout(location = 2) in vec2 aTexture;

layout(location = 0) out vec4 vColor;
layout(location = 1) out vec2 vTex;

uniform mat3 uModel_to_NDC;
uniform vec2 uOffset;
uniform vec2 uScale;

void main() {
    gl_Position = vec4(vec2(uModel_to_NDC * vec3(aVertexPosition, 1.f)), 0.0, 1.0);
    vColor = aVertexColor;
    vTex = aTexture;
}
