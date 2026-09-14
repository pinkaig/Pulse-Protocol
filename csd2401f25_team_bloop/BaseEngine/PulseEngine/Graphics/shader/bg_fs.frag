#version 450 core
	layout (location=0) in vec4 vColor;
    	layout (location=1) in vec2 vTex;

	layout (location=0) out vec4 color;
    	uniform sampler2D uTex;

	void main () {
        vec4 texColor = texture(uTex, vTex);
        color = texColor * vColor;
        if(color.a < 0.01)
            discard;
	}
