out gl_PerVertex {
	vec4 gl_Position;
};

layout(set = 3, binding=POSITION) buffer pos { vec4 position_in[]; };

layout(set = 0, binding=TRANSLATION) uniform TRANSLATION_NAME {
	mat4 model;
};

layout(set = 2, binding=CAMERA_VIEW_PROJECTION) uniform CAMERA_VIEW_PROJECTION_NAME{
	mat4 view;
	mat4 proj;
};

void main() {
	gl_Position = proj * view * model * position_in[gl_VertexIndex];
	gl_Position.y = -gl_Position.y;
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
