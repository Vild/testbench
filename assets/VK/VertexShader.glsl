out gl_PerVertex {
    vec4 gl_Position;
};

#ifdef NORMAL
	layout(set = 4, binding=NORMAL) buffer nor { vec4 normal_in[]; };
	layout(location=NORMAL) out vec4 normal_out;
#endif

#ifdef TEXTCOORD
	layout(set = 5, binding=TEXTCOORD) buffer text { vec2 uv_in[]; };
	layout(location=TEXTCOORD) out vec2 uv_out;
#endif
layout(set = 3, binding=POSITION) buffer pos { vec4 position_in[]; };

layout(set = 0, binding=TRANSLATION) uniform TRANSLATION_NAME {
	vec4 translate;
};

layout(set = 2, binding=CAMERA_VIEW_PROJECTION) uniform CAMERA_VIEW_PROJECTION_NAME{
	mat4 view;
	mat4 proj;
};


void main() {
#ifdef NORMAL
	normal_out = normal_in[gl_VertexIndex];
#endif

#ifdef TEXTCOORD
	uv_out = uv_in[gl_VertexIndex];
#endif

	gl_Position = proj * view * (position_in[gl_VertexIndex] + translate);
	gl_Position.y = -gl_Position.y;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
	//gl_Position = proj * view * 
	//gl_Position = position_in[gl_VertexIndex] + translate;
}