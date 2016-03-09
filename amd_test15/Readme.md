
Matrix × vector may not equal vector × transpose(Matrix) on AMD cards, depending on the previous code of how the matrix was computed.
The following code does not produce the expected result on AMD (driver version 15.12):

    vec3 p1, p2, m1, m2;
    const mat3x4 D = ...

    mat4x3 pvt = mat4x3(p1, m1, p2, m2);

    //mat3x3 m = mat4x3(p1, m1, p2, m2) * D;
    mat3x3 m = pvt * D;

    vec3 d1 = m * p1;
    vec3 d2 = p1 * transpose(m);
  
    bool succ = all(equal(d1, d2));
  
When using the commented line instead, all works. On Nvidia cards both cases work as expected.
