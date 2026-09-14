#version 450 core

in  vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTex;
uniform bool      uHorizontal;

// 5-tap Gaussian weights (sigma ~1.0, sum = 1)
const float weight[5] = float[](0.2270, 0.1945, 0.1216, 0.0540, 0.0162);

void main()
{
    vec2 texelSize = 1.0 / textureSize(uTex, 0);
    vec3 result = texture(uTex, TexCoord).rgb * weight[0];

    if (uHorizontal)
    {
        for (int i = 1; i < 5; ++i)
        {
            result += texture(uTex, TexCoord + vec2(texelSize.x * i, 0.0)).rgb * weight[i];
            result += texture(uTex, TexCoord - vec2(texelSize.x * i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for (int i = 1; i < 5; ++i)
        {
            result += texture(uTex, TexCoord + vec2(0.0, texelSize.y * i)).rgb * weight[i];
            result += texture(uTex, TexCoord - vec2(0.0, texelSize.y * i)).rgb * weight[i];
        }
    }

    FragColor = vec4(result, 1.0);
}
