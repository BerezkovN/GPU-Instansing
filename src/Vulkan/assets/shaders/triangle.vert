#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

// Vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

// Instances attributes
layout(location = 2) in vec4 inTranslate;
layout(location = 3) in vec4 inRotation;
layout(location = 4) in vec4 inUv;

// Out
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {

    mat4 modelMat;
    float s = sin(inRotation.z);
	float c = cos(inRotation.z);
    modelMat[0] = vec4(c, -s, 0.0, 0.0);
    modelMat[1] = vec4(s,  c, 0.0, 0.0);
    modelMat[2] = vec4(0.0, 0.0, 1.0, 0.0);
    modelMat[3] = vec4(inTranslate.x, inTranslate.y, inTranslate.z, 1.0);

    gl_Position = ubo.proj * ubo.view * modelMat * vec4(inPosition, 1.0);
    fragColor = inColor;

    /* * *
	*  gl_VertexIndex   inUv_index
	*  0                0, 2
	*  1                0, 3
	*  2                1, 3
	*  3                1, 2
	*
	*  Meaning
	*
	*  inUv[0 + 4*k] = topLeft.x
	*  inUv[1 + 4*k] = bottomRight.x
	*  inUv[2 + 4*k] = topLeft.y 
	*  inUv[3 + 4*k] = bottomRight.y
	*
	*  where k - instance id
	*/
    fragTexCoord = vec2(inUv[gl_VertexIndex / 2], inUv[2 + (((gl_VertexIndex + 1) % 4) / 2)]);
}