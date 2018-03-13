layout(set = 0, binding=POSITION) buffer pos { vec4 position_in[]; };
layout(set = 1, binding=INDEX) buffer idx { int index_in[]; };
layout(set = 2, binding=MODEL) buffer m { mat4 model[]; };

layout(set = 3, binding=CAMERA_VIEW_PROJECTION) uniform CAMERA_VIEW_PROJECTION_NAME {
	mat4 view;
	mat4 proj;
};
layout(location = 4) out vec3 wColor;

const vec3 colors[4] = {
	vec3(1, 0, 0),
	vec3(0, 1, 0),
	vec3(0, 0, 1),
	vec3(1, 1, 1)
};
void main() {
	int vertexID = index_in[gl_VertexID];
	vec4 wPos = model[gl_BaseInstance] * vec4(position_in[vertexID].xyz, 1);
	wColor = colors[gl_VertexID % 4];
	gl_Position = proj * view * wPos;
};
