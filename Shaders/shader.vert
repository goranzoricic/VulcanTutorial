#version 450
#extension GL_ARB_separate_shader_objects : enable

// Uniform buffer description.
layout(binding = 0) uniform UniformBufferObject {
    // Model transform.
    mat4 tModel;
    // View transform.
    mat4 tView;
    // Projection transform.
    mat4 tProjection;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTextureCoord;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTextureCoord;

void main() {
    gl_Position = ubo.tProjection * ubo.tView * ubo.tModel * vec4(inPosition, 1.0);
    fragColor = inColor;
	fragTextureCoord = inTextureCoord;
}