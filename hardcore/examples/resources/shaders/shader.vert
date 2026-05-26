#version 450

#extension GL_EXT_buffer_reference : require

layout(buffer_reference) readonly buffer Vertices
{
    vec3 positions[];
};

layout(buffer_reference) readonly buffer Indexes
{
    uint indexes[];
};

layout(buffer_reference) readonly buffer VertexParams
{
    vec2 offset;
    float scale;
    float ratio;
};

layout(buffer_reference) readonly buffer FragParams
{
    uint func_idx;
    uint iterations;
    float frame;
};

layout(push_constant) uniform Buffers
{
    Vertices vertices;
    Indexes indexes;
    VertexParams vertex_params;
    FragParams frag_params;
} buffers;

layout (location = 0) out vec2 pos;

void main()
{
    vec2 offset = buffers.vertex_params.offset;
    float scale = buffers.vertex_params.scale;
    float ratio = buffers.vertex_params.ratio;

    uint vertex_index = buffers.indexes.indexes[gl_VertexIndex];
    gl_Position = vec4(buffers.vertices.positions[vertex_index], 1.0);

    vec2 coords = gl_Position.xy;
    coords.y *= -1.0;//inverting y coordinates to match the demo reference
    offset.y *= -1.0;

    if (ratio < 1.0) coords.y /= ratio;
    else coords.x *= ratio;

    coords = coords * scale - offset;
    pos = coords;
}
