#version 450

layout(location = 0) in vec2 pos;

layout(std140, set = 0, binding = 1) uniform param_struct
{
    uvec4 func; //func_id, iterations, padding
    float frame;
} params;

layout(location = 0) out vec4 outColor;

const float PI = radians(180);

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec2 multiply(vec2 a, vec2 b)
{
    return vec2((a.x * b.x) + (a.y * b.y * -1.0), (a.y * b.x) + (a.x * b.y));
}

vec2 divide(vec2 a, vec2 b)
{
    vec2 num = multiply(a, vec2(b.x, -1.0 * b.y));
    float div = (b.x * b.x) + abs(b.y * b.y);
    return vec2(num.x / div, num.y / div);
}

vec2 expa(vec2 a)
{
    return vec2(exp(a.x) * cos(a.y), exp(a.x) * sin(a.y));
}

vec2 aux(vec2 a)
{
    vec2 x = a + vec2(-2.0, 0);
    return multiply(multiply(multiply(x, x), a + vec2(1.0, -2.0)), a + vec2(2.0, 2.0));
}

vec2 func0(vec2 coords)
{
    vec2 res = coords;
    uint iterations = params.func.y;
    for(uint i = 0; i < iterations; i++)
    {
        res = multiply(res, res);
    }
    return res;
}

vec2 func1(vec2 coords)
{
    vec2 res = coords;
    uint iterations = params.func.y;
    for(uint i = 0; i < iterations; i++)
    {
        res = expa(divide(vec2(100.0, 0.0), res));
    }
    return res;
}

vec2 func2(vec2 coords)
{
    vec2 res = coords;
    uint iterations = params.func.y;
    for(uint i = 0; i < iterations; i++)
    {
        vec2 s = res + vec2(-2.0, 0);
        vec2 m = multiply(multiply(multiply(s, s), res + vec2(1.0, -2.0)), res + vec2(2.0, 2.0));
        res = expa(res) + divide(m, multiply(res, multiply(res, res)));
    }
    return res;
}

vec2 func3(vec2 coords)
{
    vec2 res = coords;
    uint iterations = params.func.y;
    for(uint i = 0; i < iterations; i++)
    {
        vec2 p = multiply(res, res);
        res = divide(p + vec2(1.0, 0.0), p + vec2(-1.0, 0.0));
    }
    return res;
}

vec2 func4(vec2 coords)
{
    vec2 res = coords;
    uint iterations = params.func.y;
    for(uint i = 0; i < iterations; i++)
    {
        vec2 s = res + vec2(2.0, 0);
        res = multiply(multiply(multiply(s, s), res + vec2(-1.0, -2.0)), res + vec2(0.0, 1.0));
    }
    return res;
}

vec2 func(vec2 coords)
{
    uint func_id = params.func.x;
    switch(func_id)
    {
    case 0:
        return func0(coords);
    case 1:
        return func1(coords);
    case 2:
        return func2(coords);
    case 3:
        return func3(coords);
    case 4:
        return func4(coords);
    }
    return coords;
}


void main() {
    vec2 coords = func(pos);

    //coords = multiply(coords, coords);
    //coords = expa(divide(vec2(100.0, 0.0), coords));
    //coords = expa(coords) + divide(aux(coords), multiply(coords, multiply(coords, coords)));
    //coords = divide(multiply(coords, coords) + vec2(1.0, 0.0), multiply(coords, coords) + vec2(-1.0, 0.0));
     
    float ang = atan(coords.y, coords.x) / (PI * 2);
    float rad = sqrt(coords.x*coords.x + coords.y*coords.y);
    vec3 colors = hsv2rgb(vec3(ang, 1.0, fract(log2(rad) + params.frame)));
    outColor = vec4(colors, 1.0);
}