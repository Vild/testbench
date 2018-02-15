out gl_PerVertex {
    vec4 gl_Position;
};

#ifdef NORMAL
	layout(set = 3, binding=NORMAL) buffer nor { vec4 normal_in[]; };
	layout(location=NORMAL) out vec4 normal_out;
#endif

#ifdef TEXTCOORD
	layout(set = 4, binding=TEXTCOORD) buffer text { vec2 uv_in[]; };
	layout(location=TEXTCOORD) out vec2 uv_out;
#endif
layout(set = 2, binding=POSITION) buffer pos { vec4 position_in[]; };

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(-0.5, 0.5),
    vec2(0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

layout(set = 0, binding=TRANSLATION) uniform TRANSLATION_NAME
{
	vec4 translate;
};

void main() {
    gl_Position = vec4(position_in[gl_VertexIndex].xy + translate.xy, 0.0, 1.0);
	//gl_Position = position_in[gl_VertexIndex] + translate;
}

