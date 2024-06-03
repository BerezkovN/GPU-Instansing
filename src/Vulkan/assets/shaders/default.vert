#version 450

layout(binding = 0) uniform Matrices {
    mat4 view;
    mat4 proj;
} matrices;

layout(push_constant) uniform PerObject {
	vec4 translate;
	vec4 uv;
} perObject;

// Vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

// Out
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {

    mat4 modelMat;
    modelMat[0] = vec4(1.0, 0.0, 0.0, 0.0);
    modelMat[1] = vec4(0.0, 1.0, 0.0, 0.0);
    modelMat[2] = vec4(0.0, 0.0, 1.0, 0.0);
    modelMat[3] = vec4(perObject.translate.x, perObject.translate.y, perObject.translate.z, 1.0);

    gl_Position = matrices.proj * matrices.view * modelMat * vec4(inPosition, 1.0);
    fragColor = inColor;

    fragTexCoord = vec2(perObject.uv[gl_VertexIndex / 2], perObject.uv[2 + (((gl_VertexIndex + 1) % 4) / 2)]);
}