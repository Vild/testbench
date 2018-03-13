layout(location = 0) out vec4 outColor;

layout(location = 4) in vec3 wColor;
void main() {
	outColor = vec4(wColor + normalize(gl_FragCoord.xyz), 1.0);
}