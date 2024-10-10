#version 450

layout(location = 0) out vec3 fragColor;

layout(binding = 0) uniform Unis {
	float time;
} uni;

vec2 positions[3] = vec2[](
	vec2(0.5, 0.0),
	vec2(-0.5, 0.0),
	vec2(0.0, 0.5)
);

vec3 colors[3] = vec3[](
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0)
);

void main() {
	float t = 3.14 * uni.time;
	mat2 rot = mat2(cos(t), -sin(t), sin(t), cos(t)); 
	gl_Position = vec4(rot * positions[gl_VertexIndex], 0.0, 1.0);
	fragColor = colors[gl_VertexIndex];
	fragColor.b = sin(t);
}
