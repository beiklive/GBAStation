/* 
   TINY3D (c) 2010 Hermes  <www.elotrolado.net>
   This work is based in ElSemi, Phire, AerialX and may be, others PSL1GHT contributors

*/

#ifndef MATRIX_H
#define MATRIX_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {

    float data[4][4];

} MATRIX;

typedef struct {

    float x, y, z;

} VECTOR;

MATRIX MakeLookAtMatrix(VECTOR eye, VECTOR center, VECTOR up);

MATRIX MatrixProjPerspective(float fov, float aspect, float near, float far);

VECTOR MatrixVectorMultiply(MATRIX mat, VECTOR vec);

VECTOR MatrixVectorMultiply3x3(MATRIX mat, VECTOR vec);

MATRIX MatrixMultiply(MATRIX old_matrix, MATRIX new_matrix);

MATRIX MatrixIdentity(void);

MATRIX MatrixTranslation(float x, float y, float z);

MATRIX MatrixTranspose(MATRIX src);

MATRIX MatrixScale(float x, float y, float z);

MATRIX MatrixRotationX(float angle);

MATRIX MatrixRotationY(float angle);

MATRIX MatrixRotationZ(float angle);

MATRIX MatrixRotationAxis(float angle, VECTOR v);

/* vectors */

VECTOR VectorToUnit(VECTOR v);

VECTOR VectorNormalPlane(VECTOR v1, VECTOR v2, VECTOR v3);

#ifdef __cplusplus
}
#endif

#endif