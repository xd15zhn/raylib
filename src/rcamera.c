#include "raylib.h"
#include <math.h>

#define VAL_LIMIT(x, min, max)           (((x)<=(min) ? (min) : ((x)>=(max) ? (max) : (x))))

typedef enum {
    MOVE_FRONT = 0,
    MOVE_BACK,
    MOVE_RIGHT,
    MOVE_LEFT,
    MOVE_UP,
    MOVE_DOWN
} KeyMoves;

Vector2 angle;
Vector2 previousMousePosition;
int moveControl[6] = { 'W', 'S', 'D', 'A', 'E', 'Q' };
int smoothZoomControl;
float MouseMoveSensitivity = 0.003f;
float MouseScrolSensitivity = 1.5f;
float PlayerMoveSensitivity = 8.0f;

void Init_Camera(Camera *camera)
{
    camera->position = (Vector3){ 0.0f, 25.0f, 25.0f };
    camera->target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera->up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera->fovy = 45.0f;
    camera->target = Vector3Subtract(camera->position, camera->target);
    camera->target = Vector3Add(camera->position, camera->target);
    Vector3 v1 = camera->position;
    Vector3 v2 = camera->target;
    float dx = v2.x - v1.x;
    float dy = v2.y - v1.y;
    float dz = v2.z - v1.z;
    angle.x = atan2f(dx, dz);
    angle.y = atan2f(dy, sqrtf(dx*dx + dz*dz));
    previousMousePosition = GetMousePosition();
    DisableCursor();
}

void Update_Camera(Camera *camera)
{
    Vector2 mousePositionDelta = { 0.0f, 0.0f };
    Vector2 mousePosition = GetMousePosition();
    float mouseWheelMove = GetMouseWheelMove();
    bool direction[6] = { IsKeyDown(moveControl[MOVE_FRONT]),
                          IsKeyDown(moveControl[MOVE_BACK]),
                          IsKeyDown(moveControl[MOVE_RIGHT]),
                          IsKeyDown(moveControl[MOVE_LEFT]),
                          IsKeyDown(moveControl[MOVE_UP]),
                          IsKeyDown(moveControl[MOVE_DOWN]) };
    mousePositionDelta.x = mousePosition.x - previousMousePosition.x;
    mousePositionDelta.y = mousePosition.y - previousMousePosition.y;
    previousMousePosition = mousePosition;
    camera->position.x += (sinf(angle.x)*direction[MOVE_BACK] -
                            sinf(angle.x)*direction[MOVE_FRONT] -
                            cosf(angle.x)*direction[MOVE_LEFT] +
                            cosf(angle.x)*direction[MOVE_RIGHT])/PlayerMoveSensitivity;
    camera->position.y += (sinf(angle.y)*direction[MOVE_BACK] -
                            sinf(angle.y)*direction[MOVE_FRONT] +
                            direction[MOVE_UP] - direction[MOVE_DOWN])/PlayerMoveSensitivity;
    camera->position.z += (cosf(angle.x)*direction[MOVE_BACK] -
                            cosf(angle.x)*direction[MOVE_FRONT] +
                            sinf(angle.x)*direction[MOVE_LEFT] -
                            sinf(angle.x)*direction[MOVE_RIGHT])/PlayerMoveSensitivity;
    angle.x -= (MouseMoveSensitivity * mousePositionDelta.x);
    angle.y += (MouseMoveSensitivity * mousePositionDelta.y);
    angle.y = VAL_LIMIT(angle.y, -1.57f, 1.57f);
    camera->target.x = camera->position.x - cosf(angle.y)*sinf(angle.x);
    camera->target.y = camera->position.y - sinf(angle.y);
    camera->target.z = camera->position.z - cosf(angle.y)*cosf(angle.x);
}
