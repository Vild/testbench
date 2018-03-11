layout(binding=POSITION) buffer pos { vec4 position_in[]; };

layout(binding=TRANSLATION) uniform TRANSLATION_NAME {
	mat4 model;
};

layout(binding=CAMERA_VIEW_PROJECTION) uniform CAMERA_VIEW_PROJECTION_NAME {
	mat4 view;
	mat4 proj;
};

void main() {
	gl_Position = proj * view * model * position_in[gl_VertexID];
};
