#version 450

layout(location = 0) in vec3 inPosition;

layout(std140, set = 0, binding = 0) uniform param_struct
{
    vec4 space; //offset x/y, scale, aspect_ratio
} params;

layout(location = 0) out vec2 pos;

void main()
{
    vec2 offset = params.space.xy;
    float scale = params.space.z;
    float ratio = params.space.w;
    vec2 coords = inPosition.xy;
    coords.y *= -1.0; //inverting y coordinates to match the demo reference
    offset.y *= -1.0;
    if (ratio < 1.0)
        coords.y /= ratio;
    else
        coords.x *= ratio;
    coords = coords * scale - offset;
    gl_Position = vec4(inPosition, 1.0);
    pos = coords;
}