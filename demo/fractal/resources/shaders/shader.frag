#version 450

layout(std140, set = 0, binding = 0) uniform param_struct
{
    vec4 space;
    uvec4 func;
} params;

layout(location = 0) out vec4 outColor;

vec3 hsv2rgb(vec3 c)
    {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}
vec2 multiply(vec2 a, vec2 b) {
    return vec2((a.x * b.x) + (a.y * a.y * -1.0), (a.y * b.x) + (a.x * a.y));
}
vec2 divide(vec2 a, vec2 b) {
    vec2 num = multiply(a, vec2(b.x, -1.0 * b.y));
    float div = (b.x * b.x) + abs(b.y * b.y);
    return vec2(num.x / div, num.y / div);
}
vec2 expa(vec2 a) {
    return vec2(exp(a.x) * cos(a.y), exp(a.x) * sin(a.y));
}

void main() {
    float w = 1080;
    float h = 720;
    float m = h;
    float ofst = (w - h) / 2;
    float s = params.space.z;
    float offsetx = params.space.x;
    float offsety = params.space.y;
    float dx = (((gl_FragCoord.x - ofst) / 720.0) - 0.5) * s - offsetx;
    float dy = ((gl_FragCoord.y / 720.0) - 0.5) * s - offsety;
    vec2 coords = vec2(dx, dy);

    //coords = multiply(coords, coords);
    //coords = expa(divide(vec2(100.0, 0.0), coords));
    //coords = divide(multiply(coords, coords) + vec2(1.0, 0.0), multiply(coords, coords) + vec2(-1.0, 0.0));
     
    float ang = (degrees(atan(-coords.y, -coords.x)) + 180.0) / 360.0;
    float rad = sqrt(coords.x*coords.x + coords.y*coords.y);
    //rad = fract(rad + frame);
    vec3 colors = hsv2rgb(vec3(ang, 1.0, fract(log2(rad))));
    outColor = vec4(colors.x, colors.y, colors.z, 1.0);
}