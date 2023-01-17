#version 450

/*
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
}
*/

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragColor;

layout(std430, set = 0, binding = 0) readonly buffer typename
{
    uint data[];
} storage;

void main()
{
    gl_Position = vec4(inPosition, 1.0);
    if(storage.data[0] == 0)
        fragColor = vec3(0.2, 0.2, 0.5);
    else
        fragColor = vec3(1.0, 0.0, 0.0);
}