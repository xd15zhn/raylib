#ifndef RAYMATH_H
#define RAYMATH_H

#ifndef PI
    #define PI 3.14159265358979323846f
#endif
#ifndef DEG2RAD
    #define DEG2RAD (PI/180.0f)
#endif

typedef struct {
    float x;
    float y;
} Vector2;
typedef struct {
    float x;
    float y;
    float z;
} Vector3;
typedef struct {
    float x;
    float y;
    float z;
    float w;
} Vector4;
typedef Vector4 Quaternion;

// Matrix type (OpenGL style 4x4 - right handed, column major)
typedef struct {
    float m0, m4, m8, m12;      // Matrix first row (4 components)
    float m1, m5, m9, m13;      // Matrix second row (4 components)
    float m2, m6, m10, m14;     // Matrix third row (4 components)
    float m3, m7, m11, m15;     // Matrix fourth row (4 components)
} Matrix;

// NOTE: Helper types to be used instead of array return types for *ToFloat functions
typedef struct {
    float v[3];
} float3;

typedef struct {
    float v[16];
} float16;

// Get float vector for Matrix
#define MatrixToFloat(mat) (MatrixToFloatV(mat).v)
// Get float vector for Vector3
#define Vector3ToFloat(vec) (Vector3ToFloatV(vec).v)

#if defined(__cplusplus)
extern "C" {
#endif

Vector2 Vector2Zero(void);
Vector3 Vector3Min(Vector3 v1, Vector3 v2);
Vector3 Vector3Max(Vector3 v1, Vector3 v2);
Vector3 Vector3Add(Vector3 v1, Vector3 v2);
Vector3 Vector3Subtract(Vector3 v1, Vector3 v2);
Vector3 Vector3Scale(Vector3 v, float scalar);
Vector3 Vector3Multiply(Vector3 v1, Vector3 v2);
float Vector3DotProduct(Vector3 v1, Vector3 v2);
Vector3 Vector3CrossProduct(Vector3 v1, Vector3 v2);
float Vector3Length(const Vector3 v);
float Vector3LengthSqr(const Vector3 v);

Vector3 Vector3Perpendicular(Vector3 v);  // Calculate one vector perpendicular vector
Vector3 Vector3Normalize(Vector3 v);

// Orthonormalize provided vectors
// Makes vectors normalized and orthogonal to each other
// Gram-Schmidt function implementation
void Vector3OrthoNormalize(Vector3 *v1, Vector3 *v2);

// Transforms a Vector3 by a given Matrix
Vector3 Vector3Transform(Vector3 v, Matrix mat);

// Transform a vector by quaternion rotation
Vector3 Vector3RotateByQuaternion(Vector3 v, Quaternion q);

// Get Vector3 as float array
float3 Vector3ToFloatV(Vector3 v);

//----------------------------------------------------------------------------------
// Module Functions Definition - Matrix math
//----------------------------------------------------------------------------------

// Transposes provided matrix
Matrix MatrixTranspose(Matrix mat);

// Invert provided matrix
Matrix MatrixInvert(Matrix mat);

// Get identity matrix
Matrix MatrixIdentity(void);

// Add two matrices
Matrix MatrixAdd(Matrix left, Matrix right);

// Subtract two matrices (left - right)
Matrix MatrixSubtract(Matrix left, Matrix right);

// Get two matrix multiplication
// NOTE: When multiplying matrices... the order matters!
Matrix MatrixMultiply(Matrix left, Matrix right);

// Get translation matrix
Matrix MatrixTranslate(float x, float y, float z);

// Create rotation matrix from axis and angle
// NOTE: Angle should be provided in radians
Matrix MatrixRotate(Vector3 axis, float angle);

// Get x-rotation matrix (angle in radians)
Matrix MatrixRotateX(float angle);

// Get y-rotation matrix (angle in radians)
Matrix MatrixRotateY(float angle);

// Get z-rotation matrix (angle in radians)
Matrix MatrixRotateZ(float angle);


// Get xyz-rotation matrix (angles in radians)
Matrix MatrixRotateXYZ(Vector3 ang);

// Get zyx-rotation matrix (angles in radians)
Matrix MatrixRotateZYX(Vector3 ang);

// Get scaling matrix
Matrix MatrixScale(float x, float y, float z);

// Get perspective projection matrix
Matrix MatrixFrustum(double left, double right, double bottom, double top, double near, double far);

// Get perspective projection matrix
// NOTE: Angle should be provided in radians
Matrix MatrixPerspective(double fovy, double aspect, double near, double far);

// Get orthographic projection matrix
Matrix MatrixOrtho(double left, double right, double bottom, double top, double near, double far);

// Get camera look-at matrix (view matrix)
Matrix MatrixLookAt(Vector3 position, Vector3 target, Vector3 up);

// Get float array of matrix data
float16 MatrixToFloatV(Matrix mat);

//----------------------------------------------------------------------------------
// Module Functions Definition - Quaternion math
//----------------------------------------------------------------------------------

// Add two quaternions
Quaternion QuaternionAdd(Quaternion q1, Quaternion q2);

// Add quaternion and float value
Quaternion QuaternionAddValue(Quaternion q, float add);

// Subtract two quaternions
Quaternion QuaternionSubtract(Quaternion q1, Quaternion q2);

// Subtract quaternion and float value
Quaternion QuaternionSubtractValue(Quaternion q, float sub);

// Get identity quaternion
Quaternion QuaternionIdentity(void);

// Computes the length of a quaternion
float QuaternionLength(Quaternion q);

// Normalize provided quaternion
Quaternion QuaternionNormalize(Quaternion q);

// Invert provided quaternion
Quaternion QuaternionInvert(Quaternion q);

// Calculate two quaternion multiplication
Quaternion QuaternionMultiply(Quaternion q1, Quaternion q2);

// Scale quaternion by float value
Quaternion QuaternionScale(Quaternion q, float mul);

// Calculate slerp-optimized interpolation between two quaternions
Quaternion QuaternionNlerp(Quaternion q1, Quaternion q2, float amount);

// Calculates spherical linear interpolation between two quaternions
Quaternion QuaternionSlerp(Quaternion q1, Quaternion q2, float amount);

// Calculate quaternion based on the rotation from one vector to another
Quaternion QuaternionFromVector3ToVector3(Vector3 from, Vector3 to);

// Get a quaternion for a given rotation matrix
Quaternion QuaternionFromMatrix(Matrix mat);

// Get a matrix for a given quaternion
Matrix QuaternionToMatrix(Quaternion q);

// Get rotation quaternion for an angle and axis
// NOTE: angle must be provided in radians
Quaternion QuaternionFromAxisAngle(Vector3 axis, float angle);

// Get the rotation angle and axis for a given quaternion
void QuaternionToAxisAngle(Quaternion q, Vector3 *outAxis, float *outAngle);

// Get the quaternion equivalent to Euler angles
// NOTE: Rotation order is ZYX
Quaternion QuaternionFromEuler(float pitch, float yaw, float roll);

// Get the Euler angles equivalent to quaternion (roll, pitch, yaw)
// NOTE: Angles are returned in a Vector3 struct in radians
Vector3 QuaternionToEuler(Quaternion q);

// Transform a quaternion given a transformation matrix
Quaternion QuaternionTransform(Quaternion q, Matrix mat);

#if defined(__cplusplus)
}
#endif
#endif // RAYMATH_H
