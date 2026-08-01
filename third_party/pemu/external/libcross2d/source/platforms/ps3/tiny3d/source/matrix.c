/* 
   TINY3D (c) 2010 Hermes  <www.elotrolado.net>
   This work is based in ElSemi, Phire, AerialX and may be, others PSL1GHT contributors

*/

#include "matrix.h"
#include <string.h>
#include <math.h>

static MATRIX matrix_ident =
{
    {
    {1.0, 0.0, 0.0, 0.0},
    {0.0, 1.0, 0.0, 0.0},
    {0.0, 0.0, 1.0, 0.0},
    {0.0, 0.0, 0.0, 1.0}
    }
};

MATRIX MatrixProjPerspective(float fov, float aspect, float near, float far)
{
    MATRIX project;

	float cotangent;

	cotangent = 1.0f/tanf(fov*0.00872664626f);

    memset(&project, 0, sizeof(MATRIX));
   
    project.data[0][0] =  cotangent/aspect;
    project.data[1][1] =  - cotangent;
  
    project.data[2][2] = 1.0f;
    project.data[2][3] = (near * far )/(far - near);
    project.data[3][2] = 1.0f;

    return project;
}

VECTOR MatrixVectorMultiply(MATRIX mat, VECTOR vec)
{
	float *m = (float *) &mat;
    
    VECTOR temp;
	
    temp.x = m[0] * vec.x + m[4] * vec.y + m[8]  * vec.z + m[12];
    temp.y = m[1] * vec.x + m[5] * vec.y + m[9]  * vec.z + m[13];
    temp.z = m[2] * vec.x + m[6] * vec.y + m[10] * vec.z + m[14];
    
    return temp;
}

VECTOR MatrixVectorMultiply3x3(MATRIX mat, VECTOR vec)
{
	float *m = (float *) &mat;
    
    VECTOR temp;
	
    temp.x = m[0] * vec.x + m[4] * vec.y + m[8]  * vec.z;
    temp.y = m[1] * vec.x + m[5] * vec.y + m[9]  * vec.z;
    temp.z = m[2] * vec.x + m[6] * vec.y + m[10] * vec.z;
    
    return temp;
}

MATRIX MatrixMultiply(MATRIX old_matrix, MATRIX new_matrix)
{
	MATRIX mo;
    float *m = (float *) &mo, *a = (float *) &old_matrix, *b = (float *) &new_matrix;

    m[0]  = a[0]*b[0]  + a[1]*b[4]  + a[2]*b[8];
    m[1]  = a[0]*b[1]  + a[1]*b[5]  + a[2]*b[9];
    m[2]  = a[0]*b[2]  + a[1]*b[6]  + a[2]*b[10];
    m[3]  = a[0]*b[3]  + a[1]*b[7]  + a[2]*b[11];

    m[4]  = a[4]*b[0]  + a[5]*b[4]  + a[6]*b[8];
    m[5]  = a[4]*b[1]  + a[5]*b[5]  + a[6]*b[9];
    m[6]  = a[4]*b[2]  + a[5]*b[6]  + a[6]*b[10];
    m[7]  = a[4]*b[3]  + a[5]*b[7]  + a[6]*b[11];

    m[8]  = a[8]*b[0]  + a[9]*b[4]  + a[10]*b[8];
    m[9]  = a[8]*b[1]  + a[9]*b[5]  + a[10]*b[9];
    m[10] = a[8]*b[2]  + a[9]*b[6]  + a[10]*b[10];
    m[11] = a[8]*b[3]  + a[9]*b[7]  + a[10]*b[11];

    m[12] = a[12]*b[0] + a[13]*b[4] + a[14]*b[8];
    m[13] = a[12]*b[1] + a[13]*b[5] + a[14]*b[9];
    m[14] = a[12]*b[2] + a[13]*b[6] + a[14]*b[10];
    m[15] = a[12]*b[3] + a[13]*b[7] + a[14]*b[11];

    if(a[3]) {
        m[0]  += a[3]*b[12];
        m[1]  += a[3]*b[13];
        m[2]  += a[3]*b[14];
        m[3]  += a[3]*b[15];
    }
   
    if(a[7]) {
        m[4]  += a[7]*b[12];
        m[5]  += a[7]*b[13];
        m[6]  += a[7]*b[14];
        m[6]  += a[7]*b[15];
    }
    
    if(a[11]) {
        m[8]  += a[11]*b[12];
        m[9]  += a[11]*b[13];
        m[10] += a[11]*b[14];
        m[11] += a[11]*b[15];
    }

    if(a[15]) {
        m[12] += a[15]*b[12];
        m[13] += a[15]*b[13];
        m[14] += a[15]*b[14];
        m[15] += a[15]*b[15];
    }
   

    return mo;
}


MATRIX MatrixIdentity(void)
{
    return matrix_ident;
}

MATRIX MatrixTranslation(float x, float y, float z)
{
    MATRIX result = MatrixIdentity();

    result.data[3][0] = x;
    result.data[3][1] = y;
    result.data[3][2] = z;

    return result;
}

MATRIX MatrixTranspose(MATRIX src)
{
    int row, col;

    MATRIX result;

    for(row = 0; row < 4; row++)
         for(col = 0; col < 4 ; col++) {
         
            result.data[row][col] = src.data[col][row];

         }

    return result;
}

MATRIX MatrixScale(float x, float y, float z)
{
    MATRIX result = MatrixIdentity();
    
    result.data[0][0] = x;
    result.data[1][1] = y;
    result.data[2][2] = z;

    return result;
}


MATRIX MatrixRotationX(float angle)
{
    MATRIX result = MatrixIdentity();
    
    float fcos = cosf(angle), fsin = sinf(angle);

    result.data[1][1] = fcos;
    result.data[2][1] = -fsin;

    result.data[1][2] = fsin;
    result.data[2][2] = fcos;

    return result;
}

MATRIX MatrixRotationY(float angle)
{
    MATRIX result = MatrixIdentity();

    float fcos = cosf(angle), fsin = sinf(angle);

    result.data[0][0] = fcos;
    result.data[2][0] = fsin;

    result.data[0][2] = -fsin;
    result.data[2][2] = fcos;

    return result;
}

MATRIX MatrixRotationZ(float angle)
{
    MATRIX result = MatrixIdentity();

    float fcos = cosf(-angle), fsin = sinf(-angle);

    result.data[0][0] = fcos;
    result.data[1][0] = -fsin;

    result.data[0][1] = fsin;
    result.data[1][1] = fcos;

    return result;
}

MATRIX MatrixRotationAxis(float angle, VECTOR v)
{

    MATRIX result;
    
    float fcos = cosf(angle), fsin = sinf(angle);

    v.z = - v.z;

    result = MatrixIdentity();

    if(v.x == 1.0f && v.y == 0.0f && v.z == 0.0f) {

        result.data[1][1] = fcos;
        result.data[2][1] = -fsin;

        result.data[1][2] = fsin;
        result.data[2][2] = fcos;
    
    } else if(v.x == 0.0f && v.y == 1.0f && v.z == 0.0f) {

        result.data[0][0] = fcos;
        result.data[2][0] = fsin;

        result.data[0][2] = -fsin;
        result.data[2][2] = fcos;

    } else if(v.x == 0.0f && v.y == 0.0f && v.z == 1.0f) {

        result.data[0][0] = fcos;
        result.data[1][0] = -fsin;

        result.data[0][1] = fsin;
        result.data[1][1] = fcos;

    } else {

        float vxx = v.x * v.x, vyy = v.y * v.y, vzz = v.z * v.z;

        float len = vxx + vyy + vzz;
   
        // normalize

        if (len != 0.0f && (len > 1.0001f || len < 0.9999f)) {
            len = 1.0f / sqrtf(len);
            v.x *= len; v.y *= len; v.z *= len;

            vxx = v.x * v.x; vyy = v.y * v.y; vzz = v.z * v.z;
        }

        float vxy = v.x * v.y, vxz = v.x * v.z, vyz = v.y * v.z;

        float vsinx = fsin * v.x, vsiny = fsin * v.y, vsinz = fsin * v.z;
        
        result.data[0][0] = vxx + fcos * (1.0f - vxx);
        result.data[0][1] = vxy + fcos * (-vxy) + vsinz;
        result.data[0][2] = vxz + fcos * (-vxz) - vsiny;

        result.data[1][0] = vxy + fcos * (-vxy) - vsinz;
        result.data[1][1] = vyy + fcos * (1.0f - vyy);
        result.data[1][2] = vyz + fcos * (-vyz) + vsinx;

        result.data[2][0] = vxz + fcos * (-vxz) + vsiny;
        result.data[2][1] = vyz + fcos * (-vyz) - vsinx;
        result.data[2][2] = vzz + fcos * (1.0f - vzz);

    }

    return result;
}

/* vectors */

VECTOR VectorToUnit(VECTOR v)
{
    float len = sqrtf( v.x * v.x + v.y * v.y + v.z * v.z);

    if(len == 0.0) len = 1.0f;

    v.x /= len;
    v.y /= len;
    v.z /= len;

    return v;
}

VECTOR VectorNormalPlane(VECTOR v1, VECTOR v2, VECTOR v3)
{
    VECTOR v, va, vb;

    va.x = v1.x - v2.x;
    va.y = v1.y - v2.y;
    va.z = v1.z - v2.z;

    vb.x = v2.x - v3.x;
    vb.y = v2.y - v3.y;
    vb.z = v2.z - v3.z;

    v.x = va.y * vb.z - va.z * vb.y;

    v.y = va.z * vb.x - va.x * vb.z;

    v.z = va.x * vb.y - va.y * vb.z;

    v = VectorToUnit(v);

    return v;
}


   
MATRIX MakeLookAtMatrix(VECTOR eye, VECTOR center, VECTOR up)
{
  double x[3], y[3], z[3], mag;

  MATRIX mat;
  
  float *m = (float *) &mat;

  /* Difference eye and center vectors to make Z vector. */
  z[0] = eye.x - center.x;
  z[1] = eye.y - center.y;
  z[2] = eye.z - center.z;

  /* Normalize Z. */
  mag = sqrt(z[0]*z[0] + z[1]*z[1] + z[2]*z[2]);
  if (mag) {
    z[0] /= mag;
    z[1] /= mag;
    z[2] /= mag;
  }

  /* Up vector makes Y vector. */
  y[0] = up.x;
  y[1] = up.y;
  y[2] = up.z;

  /* X vector = Y cross Z. */
  x[0] =  y[1] * z[2] - y[2] * z[1];
  x[1] = -y[0] * z[2] + y[2] * z[0];
  x[2] =  y[0] * z[1] - y[1] * z[0];

  /* Recompute Y = Z cross X. */
  y[0] =  z[1] * x[2] - z[2] * x[1];
  y[1] = -z[0] * x[2] + z[2] * x[0];
  y[2] =  z[0] * x[1] - z[1] * x[0];

  /* Normalize X. */
  mag = sqrt(x[0]*x[0] + x[1]*x[1] + x[2]*x[2]);
  if (mag) {
    x[0] /= mag;
    x[1] /= mag;
    x[2] /= mag;
  }

  /* Normalize Y. */
  mag = sqrt(y[0]*y[0] + y[1]*y[1] + y[2]*y[2]);
  if (mag) {
    y[0] /= mag;
    y[1] /= mag;
    y[2] /= mag;
  }

  /* Build resulting view matrix. */
  m[0] = (float) x[0];  m[4] = (float) x[1];
  m[8] = (float) x[2];  m[12] = (float) (-x[0]*eye.x + -x[1]*eye.y + -x[2]*eye.z);

  m[1] = (float) y[0];  m[5] = (float) y[1];
  m[9] = (float) y[2];  m[13] = (float) (-y[0]*eye.x + -y[1]*eye.y + -y[2]*eye.z);

  m[2] = (float) z[0];  m[6] = (float) z[1];
  m[10] = (float) z[2]; m[14] = (float) (-z[0]*eye.x + -z[1]*eye.y + -z[2]*eye.z);

  m[3] = 0.0;   m[7] = 0.0;  m[11] = 0.0;  m[15] = 1.0;

  return mat;
}
