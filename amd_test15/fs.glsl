#version 330

uniform vec3 p1;
uniform vec3 p2;
uniform vec3 m1;
uniform vec3 m2;
out int retval;

const mat3x4 D = mat3x4(
    6,  3, -6,  3,
    -6, -4,  6, -2,
    0,  1,  0,  0);


void main()
{
    mat4x3 pvt = mat4x3(p1, m1, p2, m2);

    mat3x3 m = pvt * D;
    //mat3x3 m = mat4x3(p1, m1, p2, m2) * D;

    vec3 d1 = m * p1;
    vec3 d2 = p1 * transpose(m);

    retval = d1.x == d2.x && d1.y == d2.y && d1.z == d2.z
        ? 1 : 0;
}
