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

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = ubo.tProjection * ubo.tView * ubo.tModel * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}