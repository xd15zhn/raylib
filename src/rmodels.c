#include "utils.h"
#include "raylib.h"                 // Declares module functions
#include "rlgl.h"                   // OpenGL abstraction layer to OpenGL 1.1, 3.3+ or ES2
#include "tracelog.h"
#include <stdio.h>          // Required for: sprintf()
#include <stdlib.h>         // Required for: malloc(), free()
#include <string.h>         // Required for: memcmp(), strlen()
#include <math.h>           // Required for: sinf(), cosf(), sqrtf(), fabsf()

// Selected desired model fileformats to be supported for loading
#define SUPPORT_FILEFORMAT_OBJ      1

// Support procedural mesh generation functions, uses external par_shapes.h library
// NOTE: Some generated meshes DO NOT include generated texture coordinates
#define SUPPORT_MESH_GENERATION     1

#define MAX_MATERIAL_MAPS               12      // Maximum number of shader maps supported
#define MAX_MESH_VERTEX_BUFFERS          7      // Maximum vertex buffers (VBO) per mesh

#define TINYOBJ_MALLOC RL_MALLOC
#define TINYOBJ_CALLOC RL_CALLOC
#define TINYOBJ_REALLOC RL_REALLOC
#define TINYOBJ_FREE RL_FREE

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "external/tinyobj_loader_c.h"      // OBJ/MTL file formats loading

#if defined(SUPPORT_MESH_GENERATION)
    #define PAR_MALLOC(T, N) ((T*)RL_MALLOC(N*sizeof(T)))
    #define PAR_CALLOC(T, N) ((T*)RL_CALLOC(N*sizeof(T), 1))
    #define PAR_REALLOC(T, BUF, N) ((T*)RL_REALLOC(BUF, sizeof(T)*(N)))
    #define PAR_FREE RL_FREE
    #define PAR_SHAPES_IMPLEMENTATION
    #include "external/par_shapes.h"    // Shapes 3d parametric generation
#endif

static Model LoadOBJ(const char *fileName);     // Load OBJ mesh data

// Draw a line in 3D world space
void DrawLine3D(Vector3 startPos, Vector3 endPos, Color color)
{
    // WARNING: Be careful with internal buffer vertex alignment
    // when using RL_LINES or RL_TRIANGLES, data is aligned to fit
    // lines-triangles-quads in the same indexed buffers!!!
    rlCheckRenderBatchLimit(8);

    rlBegin(RL_LINES);
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(startPos.x, startPos.y, startPos.z);
        rlVertex3f(endPos.x, endPos.y, endPos.z);
    rlEnd();
}

// Draw a point in 3D space, actually a small line
void DrawPoint3D(Vector3 position, Color color)
{
    rlCheckRenderBatchLimit(8);

    rlPushMatrix();
        rlTranslatef(position.x, position.y, position.z);
        rlBegin(RL_LINES);
            rlColor4ub(color.r, color.g, color.b, color.a);
            rlVertex3f(0.0f, 0.0f, 0.0f);
            rlVertex3f(0.0f, 0.0f, 0.1f);
        rlEnd();
    rlPopMatrix();
}

// Draw a circle in 3D world space
void DrawCircle3D(Vector3 center, float radius, Vector3 rotationAxis, float rotationAngle, Color color)
{
    rlCheckRenderBatchLimit(2*36);

    rlPushMatrix();
        rlTranslatef(center.x, center.y, center.z);
        rlRotatef(rotationAngle, rotationAxis.x, rotationAxis.y, rotationAxis.z);

        rlBegin(RL_LINES);
            for (int i = 0; i < 360; i += 10)
            {
                rlColor4ub(color.r, color.g, color.b, color.a);

                rlVertex3f(sinf(DEG2RAD*i)*radius, cosf(DEG2RAD*i)*radius, 0.0f);
                rlVertex3f(sinf(DEG2RAD*(i + 10))*radius, cosf(DEG2RAD*(i + 10))*radius, 0.0f);
            }
        rlEnd();
    rlPopMatrix();
}

// Draw a color-filled triangle (vertex in counter-clockwise order!)
void DrawTriangle3D(Vector3 v1, Vector3 v2, Vector3 v3, Color color)
{
    rlCheckRenderBatchLimit(3);

    rlBegin(RL_TRIANGLES);
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(v1.x, v1.y, v1.z);
        rlVertex3f(v2.x, v2.y, v2.z);
        rlVertex3f(v3.x, v3.y, v3.z);
    rlEnd();
}

// Draw a triangle strip defined by points
void DrawTriangleStrip3D(Vector3 *points, int pointCount, Color color)
{
    if (pointCount >= 3)
    {
        rlCheckRenderBatchLimit(3*(pointCount - 2));

        rlBegin(RL_TRIANGLES);
            rlColor4ub(color.r, color.g, color.b, color.a);

            for (int i = 2; i < pointCount; i++)
            {
                if ((i%2) == 0)
                {
                    rlVertex3f(points[i].x, points[i].y, points[i].z);
                    rlVertex3f(points[i - 2].x, points[i - 2].y, points[i - 2].z);
                    rlVertex3f(points[i - 1].x, points[i - 1].y, points[i - 1].z);
                }
                else
                {
                    rlVertex3f(points[i].x, points[i].y, points[i].z);
                    rlVertex3f(points[i - 1].x, points[i - 1].y, points[i - 1].z);
                    rlVertex3f(points[i - 2].x, points[i - 2].y, points[i - 2].z);
                }
            }
        rlEnd();
    }
}

// Draw cube
// NOTE: Cube position is the center position
void DrawCube(Vector3 position, float width, float height, float length, Color color)
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    rlCheckRenderBatchLimit(36);

    rlPushMatrix();
        // NOTE: Transformation is applied in inverse order (scale -> rotate -> translate)
        rlTranslatef(position.x, position.y, position.z);
        //rlRotatef(45, 0, 1, 0);
        //rlScalef(1.0f, 1.0f, 1.0f);   // NOTE: Vertices are directly scaled on definition

        rlBegin(RL_TRIANGLES);
            rlColor4ub(color.r, color.g, color.b, color.a);

            // Front face
            rlVertex3f(x - width/2, y - height/2, z + length/2);  // Bottom Left
            rlVertex3f(x + width/2, y - height/2, z + length/2);  // Bottom Right
            rlVertex3f(x - width/2, y + height/2, z + length/2);  // Top Left

            rlVertex3f(x + width/2, y + height/2, z + length/2);  // Top Right
            rlVertex3f(x - width/2, y + height/2, z + length/2);  // Top Left
            rlVertex3f(x + width/2, y - height/2, z + length/2);  // Bottom Right

            // Back face
            rlVertex3f(x - width/2, y - height/2, z - length/2);  // Bottom Left
            rlVertex3f(x - width/2, y + height/2, z - length/2);  // Top Left
            rlVertex3f(x + width/2, y - height/2, z - length/2);  // Bottom Right

            rlVertex3f(x + width/2, y + height/2, z - length/2);  // Top Right
            rlVertex3f(x + width/2, y - height/2, z - length/2);  // Bottom Right
            rlVertex3f(x - width/2, y + height/2, z - length/2);  // Top Left

            // Top face
            rlVertex3f(x - width/2, y + height/2, z - length/2);  // Top Left
            rlVertex3f(x - width/2, y + height/2, z + length/2);  // Bottom Left
            rlVertex3f(x + width/2, y + height/2, z + length/2);  // Bottom Right

            rlVertex3f(x + width/2, y + height/2, z - length/2);  // Top Right
            rlVertex3f(x - width/2, y + height/2, z - length/2);  // Top Left
            rlVertex3f(x + width/2, y + height/2, z + length/2);  // Bottom Right

            // Bottom face
            rlVertex3f(x - width/2, y - height/2, z - length/2);  // Top Left
            rlVertex3f(x + width/2, y - height/2, z + length/2);  // Bottom Right
            rlVertex3f(x - width/2, y - height/2, z + length/2);  // Bottom Left

            rlVertex3f(x + width/2, y - height/2, z - length/2);  // Top Right
            rlVertex3f(x + width/2, y - height/2, z + length/2);  // Bottom Right
            rlVertex3f(x - width/2, y - height/2, z - length/2);  // Top Left

            // Right face
            rlVertex3f(x + width/2, y - height/2, z - length/2);  // Bottom Right
            rlVertex3f(x + width/2, y + height/2, z - length/2);  // Top Right
            rlVertex3f(x + width/2, y + height/2, z + length/2);  // Top Left

            rlVertex3f(x + width/2, y - height/2, z + length/2);  // Bottom Left
            rlVertex3f(x + width/2, y - height/2, z - length/2);  // Bottom Right
            rlVertex3f(x + width/2, y + height/2, z + length/2);  // Top Left

            // Left face
            rlVertex3f(x - width/2, y - height/2, z - length/2);  // Bottom Right
            rlVertex3f(x - width/2, y + height/2, z + length/2);  // Top Left
            rlVertex3f(x - width/2, y + height/2, z - length/2);  // Top Right

            rlVertex3f(x - width/2, y - height/2, z + length/2);  // Bottom Left
            rlVertex3f(x - width/2, y + height/2, z + length/2);  // Top Left
            rlVertex3f(x - width/2, y - height/2, z - length/2);  // Bottom Right
        rlEnd();
    rlPopMatrix();
}

// Draw cube (Vector version)
void DrawCubeV(Vector3 position, Vector3 size, Color color)
{
    DrawCube(position, size.x, size.y, size.z, color);
}

// Draw cube wires
void DrawCubeWires(Vector3 position, float width, float height, float length, Color color)
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    rlCheckRenderBatchLimit(36);

    rlPushMatrix();
        rlTranslatef(position.x, position.y, position.z);

        rlBegin(RL_LINES);
            rlColor4ub(color.r, color.g, color.b, color.a);

            // Front face -----------------------------------------------------
            // Bottom line
            rlVertex3f(x-width/2, y-height/2, z+length/2);  // Bottom left
            rlVertex3f(x+width/2, y-height/2, z+length/2);  // Bottom right

            // Left line
            rlVertex3f(x+width/2, y-height/2, z+length/2);  // Bottom right
            rlVertex3f(x+width/2, y+height/2, z+length/2);  // Top right

            // Top line
            rlVertex3f(x+width/2, y+height/2, z+length/2);  // Top right
            rlVertex3f(x-width/2, y+height/2, z+length/2);  // Top left

            // Right line
            rlVertex3f(x-width/2, y+height/2, z+length/2);  // Top left
            rlVertex3f(x-width/2, y-height/2, z+length/2);  // Bottom left

            // Back face ------------------------------------------------------
            // Bottom line
            rlVertex3f(x-width/2, y-height/2, z-length/2);  // Bottom left
            rlVertex3f(x+width/2, y-height/2, z-length/2);  // Bottom right

            // Left line
            rlVertex3f(x+width/2, y-height/2, z-length/2);  // Bottom right
            rlVertex3f(x+width/2, y+height/2, z-length/2);  // Top right

            // Top line
            rlVertex3f(x+width/2, y+height/2, z-length/2);  // Top right
            rlVertex3f(x-width/2, y+height/2, z-length/2);  // Top left

            // Right line
            rlVertex3f(x-width/2, y+height/2, z-length/2);  // Top left
            rlVertex3f(x-width/2, y-height/2, z-length/2);  // Bottom left

            // Top face -------------------------------------------------------
            // Left line
            rlVertex3f(x-width/2, y+height/2, z+length/2);  // Top left front
            rlVertex3f(x-width/2, y+height/2, z-length/2);  // Top left back

            // Right line
            rlVertex3f(x+width/2, y+height/2, z+length/2);  // Top right front
            rlVertex3f(x+width/2, y+height/2, z-length/2);  // Top right back

            // Bottom face  ---------------------------------------------------
            // Left line
            rlVertex3f(x-width/2, y-height/2, z+length/2);  // Top left front
            rlVertex3f(x-width/2, y-height/2, z-length/2);  // Top left back

            // Right line
            rlVertex3f(x+width/2, y-height/2, z+length/2);  // Top right front
            rlVertex3f(x+width/2, y-height/2, z-length/2);  // Top right back
        rlEnd();
    rlPopMatrix();
}

// Draw cube wires (vector version)
void DrawCubeWiresV(Vector3 position, Vector3 size, Color color)
{
    DrawCubeWires(position, size.x, size.y, size.z, color);
}

// Draw cube
// NOTE: Cube position is the center position
void DrawCubeTexture(Texture2D texture, Vector3 position, float width, float height, float length, Color color)
{
    float x = position.x;
    float y = position.y;
    float z = position.z;

    rlCheckRenderBatchLimit(36);

    rlSetTexture(texture.id);

    //rlPushMatrix();
        // NOTE: Transformation is applied in inverse order (scale -> rotate -> translate)
        //rlTranslatef(2.0f, 0.0f, 0.0f);
        //rlRotatef(45, 0, 1, 0);
        //rlScalef(2.0f, 2.0f, 2.0f);

        rlBegin(RL_QUADS);
            rlColor4ub(color.r, color.g, color.b, color.a);
            // Front Face
            rlNormal3f(0.0f, 0.0f, 1.0f);                  // Normal Pointing Towards Viewer
            rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x - width/2, y - height/2, z + length/2);  // Bottom Left Of The Texture and Quad
            rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x + width/2, y - height/2, z + length/2);  // Bottom Right Of The Texture and Quad
            rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x + width/2, y + height/2, z + length/2);  // Top Right Of The Texture and Quad
            rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x - width/2, y + height/2, z + length/2);  // Top Left Of The Texture and Quad
            // Back Face
            rlNormal3f(0.0f, 0.0f, - 1.0f);                  // Normal Pointing Away From Viewer
            rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x - width/2, y - height/2, z - length/2);  // Bottom Right Of The Texture and Quad
            rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x - width/2, y + height/2, z - length/2);  // Top Right Of The Texture and Quad
            rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x + width/2, y + height/2, z - length/2);  // Top Left Of The Texture and Quad
            rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x + width/2, y - height/2, z - length/2);  // Bottom Left Of The Texture and Quad
            // Top Face
            rlNormal3f(0.0f, 1.0f, 0.0f);                  // Normal Pointing Up
            rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x - width/2, y + height/2, z - length/2);  // Top Left Of The Texture and Quad
            rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x - width/2, y + height/2, z + length/2);  // Bottom Left Of The Texture and Quad
            rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x + width/2, y + height/2, z + length/2);  // Bottom Right Of The Texture and Quad
            rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x + width/2, y + height/2, z - length/2);  // Top Right Of The Texture and Quad
            // Bottom Face
            rlNormal3f(0.0f, - 1.0f, 0.0f);                  // Normal Pointing Down
            rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x - width/2, y - height/2, z - length/2);  // Top Right Of The Texture and Quad
            rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x + width/2, y - height/2, z - length/2);  // Top Left Of The Texture and Quad
            rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x + width/2, y - height/2, z + length/2);  // Bottom Left Of The Texture and Quad
            rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x - width/2, y - height/2, z + length/2);  // Bottom Right Of The Texture and Quad
            // Right face
            rlNormal3f(1.0f, 0.0f, 0.0f);                  // Normal Pointing Right
            rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x + width/2, y - height/2, z - length/2);  // Bottom Right Of The Texture and Quad
            rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x + width/2, y + height/2, z - length/2);  // Top Right Of The Texture and Quad
            rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x + width/2, y + height/2, z + length/2);  // Top Left Of The Texture and Quad
            rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x + width/2, y - height/2, z + length/2);  // Bottom Left Of The Texture and Quad
            // Left Face
            rlNormal3f( - 1.0f, 0.0f, 0.0f);                  // Normal Pointing Left
            rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x - width/2, y - height/2, z - length/2);  // Bottom Left Of The Texture and Quad
            rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x - width/2, y - height/2, z + length/2);  // Bottom Right Of The Texture and Quad
            rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x - width/2, y + height/2, z + length/2);  // Top Right Of The Texture and Quad
            rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x - width/2, y + height/2, z - length/2);  // Top Left Of The Texture and Quad
        rlEnd();
    //rlPopMatrix();

    rlSetTexture(0);
}

// Draw cube with texture piece applied to all faces
void DrawCubeTextureRec(Texture2D texture, rayRect source, Vector3 position, float width, float height, float length, Color color)
{
    float x = position.x;
    float y = position.y;
    float z = position.z;
    float texWidth = (float)texture.width;
    float texHeight = (float)texture.height;

    rlCheckRenderBatchLimit(36);

    rlSetTexture(texture.id);

    rlBegin(RL_QUADS);
        rlColor4ub(color.r, color.g, color.b, color.a);

        // Front face
        rlNormal3f(0.0f, 0.0f, 1.0f);
        rlTexCoord2f(source.x/texWidth, (source.y + source.height)/texHeight);
        rlVertex3f(x - width/2, y - height/2, z + length/2);
        rlTexCoord2f((source.x + source.width)/texWidth, (source.y + source.height)/texHeight);
        rlVertex3f(x + width/2, y - height/2, z + length/2);
        rlTexCoord2f((source.x + source.width)/texWidth, source.y/texHeight);
        rlVertex3f(x + width/2, y + height/2, z + length/2);
        rlTexCoord2f(source.x/texWidth, source.y/texHeight);
        rlVertex3f(x - width/2, y + height/2, z + length/2);

        // Back face
        rlNormal3f(0.0f, 0.0f, - 1.0f);
        rlTexCoord2f((source.x + source.width)/texWidth, (source.y + source.height)/texHeight);
        rlVertex3f(x - width/2, y - height/2, z - length/2);
        rlTexCoord2f((source.x + source.width)/texWidth, source.y/texHeight);
        rlVertex3f(x - width/2, y + height/2, z - length/2);
        rlTexCoord2f(source.x/texWidth, source.y/texHeight);
        rlVertex3f(x + width/2, y + height/2, z - length/2);
        rlTexCoord2f(source.x/texWidth, (source.y + source.height)/texHeight);
        rlVertex3f(x + width/2, y - height/2, z - length/2);

        // Top face
        rlNormal3f(0.0f, 1.0f, 0.0f);
        rlTexCoord2f(source.x/texWidth, source.y/texHeight);
        rlVertex3f(x - width/2, y + height/2, z - length/2);
        rlTexCoord2f(source.x/texWidth, (source.y + source.height)/texHeight);
        rlVertex3f(x - width/2, y + height/2, z + length/2);
        rlTexCoord2f((source.x + source.width)/texWidth, (source.y + source.height)/texHeight);
        rlVertex3f(x + width/2, y + height/2, z + length/2);
        rlTexCoord2f((source.x + source.width)/texWidth, source.y/texHeight);
        rlVertex3f(x + width/2, y + height/2, z - length/2);

        // Bottom face
        rlNormal3f(0.0f, - 1.0f, 0.0f);
        rlTexCoord2f((source.x + source.width)/texWidth, source.y/texHeight);
        rlVertex3f(x - width/2, y - height/2, z - length/2);
        rlTexCoord2f(source.x/texWidth, source.y/texHeight);
        rlVertex3f(x + width/2, y - height/2, z - length/2);
        rlTexCoord2f(source.x/texWidth, (source.y + source.height)/texHeight);
        rlVertex3f(x + width/2, y - height/2, z + length/2);
        rlTexCoord2f((source.x + source.width)/texWidth, (source.y + source.height)/texHeight);
        rlVertex3f(x - width/2, y - height/2, z + length/2);

        // Right face
        rlNormal3f(1.0f, 0.0f, 0.0f);
        rlTexCoord2f((source.x + source.width)/texWidth, (source.y + source.height)/texHeight);
        rlVertex3f(x + width/2, y - height/2, z - length/2);
        rlTexCoord2f((source.x + source.width)/texWidth, source.y/texHeight);
        rlVertex3f(x + width/2, y + height/2, z - length/2);
        rlTexCoord2f(source.x/texWidth, source.y/texHeight);
        rlVertex3f(x + width/2, y + height/2, z + length/2);
        rlTexCoord2f(source.x/texWidth, (source.y + source.height)/texHeight);
        rlVertex3f(x + width/2, y - height/2, z + length/2);

        // Left face
        rlNormal3f( - 1.0f, 0.0f, 0.0f);
        rlTexCoord2f(source.x/texWidth, (source.y + source.height)/texHeight);
        rlVertex3f(x - width/2, y - height/2, z - length/2);
        rlTexCoord2f((source.x + source.width)/texWidth, (source.y + source.height)/texHeight);
        rlVertex3f(x - width/2, y - height/2, z + length/2);
        rlTexCoord2f((source.x + source.width)/texWidth, source.y/texHeight);
        rlVertex3f(x - width/2, y + height/2, z + length/2);
        rlTexCoord2f(source.x/texWidth, source.y/texHeight);
        rlVertex3f(x - width/2, y + height/2, z - length/2);

    rlEnd();

    rlSetTexture(0);
}

// Draw sphere
void DrawSphere(Vector3 centerPos, float radius, Color color)
{
    DrawSphereEx(centerPos, radius, 16, 16, color);
}

// Draw sphere with extended parameters
void DrawSphereEx(Vector3 centerPos, float radius, int rings, int slices, Color color)
{
    int numVertex = (rings + 2)*slices*6;
    rlCheckRenderBatchLimit(numVertex);

    rlPushMatrix();
        // NOTE: Transformation is applied in inverse order (scale -> translate)
        rlTranslatef(centerPos.x, centerPos.y, centerPos.z);
        rlScalef(radius, radius, radius);

        rlBegin(RL_TRIANGLES);
            rlColor4ub(color.r, color.g, color.b, color.a);

            for (int i = 0; i < (rings + 2); i++)
            {
                for (int j = 0; j < slices; j++)
                {
                    rlVertex3f(cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*i))*sinf(DEG2RAD*(360.0f*j/slices)),
                               sinf(DEG2RAD*(270 + (180.0f/(rings + 1))*i)),
                               cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*i))*cosf(DEG2RAD*(360.0f*j/slices)));
                    rlVertex3f(cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1)))*sinf(DEG2RAD*(360.0f*(j + 1)/slices)),
                               sinf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1))),
                               cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1)))*cosf(DEG2RAD*(360.0f*(j + 1)/slices)));
                    rlVertex3f(cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1)))*sinf(DEG2RAD*(360.0f*j/slices)),
                               sinf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1))),
                               cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1)))*cosf(DEG2RAD*(360.0f*j/slices)));

                    rlVertex3f(cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*i))*sinf(DEG2RAD*(360.0f*j/slices)),
                               sinf(DEG2RAD*(270 + (180.0f/(rings + 1))*i)),
                               cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*i))*cosf(DEG2RAD*(360.0f*j/slices)));
                    rlVertex3f(cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i)))*sinf(DEG2RAD*(360.0f*(j + 1)/slices)),
                               sinf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i))),
                               cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i)))*cosf(DEG2RAD*(360.0f*(j + 1)/slices)));
                    rlVertex3f(cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1)))*sinf(DEG2RAD*(360.0f*(j + 1)/slices)),
                               sinf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1))),
                               cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1)))*cosf(DEG2RAD*(360.0f*(j + 1)/slices)));
                }
            }
        rlEnd();
    rlPopMatrix();
}

// Draw sphere wires
void DrawSphereWires(Vector3 centerPos, float radius, int rings, int slices, Color color)
{
    int numVertex = (rings + 2)*slices*6;
    rlCheckRenderBatchLimit(numVertex);

    rlPushMatrix();
        // NOTE: Transformation is applied in inverse order (scale -> translate)
        rlTranslatef(centerPos.x, centerPos.y, centerPos.z);
        rlScalef(radius, radius, radius);

        rlBegin(RL_LINES);
            rlColor4ub(color.r, color.g, color.b, color.a);

            for (int i = 0; i < (rings + 2); i++)
            {
                for (int j = 0; j < slices; j++)
                {
                    rlVertex3f(cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*i))*sinf(DEG2RAD*(360.0f*j/slices)),
                               sinf(DEG2RAD*(270 + (180.0f/(rings + 1))*i)),
                               cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*i))*cosf(DEG2RAD*(360.0f*j/slices)));
                    rlVertex3f(cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1)))*sinf(DEG2RAD*(360.0f*(j + 1)/slices)),
                               sinf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1))),
                               cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1)))*cosf(DEG2RAD*(360.0f*(j + 1)/slices)));

                    rlVertex3f(cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1)))*sinf(DEG2RAD*(360.0f*(j + 1)/slices)),
                               sinf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1))),
                               cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1)))*cosf(DEG2RAD*(360.0f*(j + 1)/slices)));
                    rlVertex3f(cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1)))*sinf(DEG2RAD*(360.0f*j/slices)),
                               sinf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1))),
                               cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1)))*cosf(DEG2RAD*(360.0f*j/slices)));

                    rlVertex3f(cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1)))*sinf(DEG2RAD*(360.0f*j/slices)),
                               sinf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1))),
                               cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*(i + 1)))*cosf(DEG2RAD*(360.0f*j/slices)));
                    rlVertex3f(cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*i))*sinf(DEG2RAD*(360.0f*j/slices)),
                               sinf(DEG2RAD*(270 + (180.0f/(rings + 1))*i)),
                               cosf(DEG2RAD*(270 + (180.0f/(rings + 1))*i))*cosf(DEG2RAD*(360.0f*j/slices)));
                }
            }
        rlEnd();
    rlPopMatrix();
}

// Draw a cylinder
// NOTE: It could be also used for pyramid and cone
void DrawCylinder(Vector3 position, float radiusTop, float radiusBottom, float height, int sides, Color color)
{
    if (sides < 3) sides = 3;

    int numVertex = sides*6;
    rlCheckRenderBatchLimit(numVertex);

    rlPushMatrix();
        rlTranslatef(position.x, position.y, position.z);

        rlBegin(RL_TRIANGLES);
            rlColor4ub(color.r, color.g, color.b, color.a);

            if (radiusTop > 0)
            {
                // Draw Body -------------------------------------------------------------------------------------
                for (int i = 0; i < 360; i += 360/sides)
                {
                    rlVertex3f(sinf(DEG2RAD*i)*radiusBottom, 0, cosf(DEG2RAD*i)*radiusBottom); //Bottom Left
                    rlVertex3f(sinf(DEG2RAD*(i + 360.0f/sides))*radiusBottom, 0, cosf(DEG2RAD*(i + 360.0f/sides))*radiusBottom); //Bottom Right
                    rlVertex3f(sinf(DEG2RAD*(i + 360.0f/sides))*radiusTop, height, cosf(DEG2RAD*(i + 360.0f/sides))*radiusTop); //Top Right

                    rlVertex3f(sinf(DEG2RAD*i)*radiusTop, height, cosf(DEG2RAD*i)*radiusTop); //Top Left
                    rlVertex3f(sinf(DEG2RAD*i)*radiusBottom, 0, cosf(DEG2RAD*i)*radiusBottom); //Bottom Left
                    rlVertex3f(sinf(DEG2RAD*(i + 360.0f/sides))*radiusTop, height, cosf(DEG2RAD*(i + 360.0f/sides))*radiusTop); //Top Right
                }

                // Draw Cap --------------------------------------------------------------------------------------
                for (int i = 0; i < 360; i += 360/sides)
                {
                    rlVertex3f(0, height, 0);
                    rlVertex3f(sinf(DEG2RAD*i)*radiusTop, height, cosf(DEG2RAD*i)*radiusTop);
                    rlVertex3f(sinf(DEG2RAD*(i + 360.0f/sides))*radiusTop, height, cosf(DEG2RAD*(i + 360.0f/sides))*radiusTop);
                }
            }
            else
            {
                // Draw Cone -------------------------------------------------------------------------------------
                for (int i = 0; i < 360; i += 360/sides)
                {
                    rlVertex3f(0, height, 0);
                    rlVertex3f(sinf(DEG2RAD*i)*radiusBottom, 0, cosf(DEG2RAD*i)*radiusBottom);
                    rlVertex3f(sinf(DEG2RAD*(i + 360.0f/sides))*radiusBottom, 0, cosf(DEG2RAD*(i + 360.0f/sides))*radiusBottom);
                }
            }

            // Draw Base -----------------------------------------------------------------------------------------
            for (int i = 0; i < 360; i += 360/sides)
            {
                rlVertex3f(0, 0, 0);
                rlVertex3f(sinf(DEG2RAD*(i + 360.0f/sides))*radiusBottom, 0, cosf(DEG2RAD*(i + 360.0f/sides))*radiusBottom);
                rlVertex3f(sinf(DEG2RAD*i)*radiusBottom, 0, cosf(DEG2RAD*i)*radiusBottom);
            }
        rlEnd();
    rlPopMatrix();
}

// Draw a cylinder with base at startPos and top at endPos
// NOTE: It could be also used for pyramid and cone
void DrawCylinderEx(Vector3 startPos, Vector3 endPos, float startRadius, float endRadius, int sides, Color color)
{
    if (sides < 3) sides = 3;

    int numVertex = sides*6;
    rlCheckRenderBatchLimit(numVertex);

    Vector3 direction = { endPos.x - startPos.x, endPos.y - startPos.y, endPos.z - startPos.z };
    if ((direction.x == 0) && (direction.y == 0) && (direction.z == 0)) return;

    // Construct a basis of the base and the top face:
    Vector3 b1 = Vector3Normalize(Vector3Perpendicular(direction));
    Vector3 b2 = Vector3Normalize(Vector3CrossProduct(b1, direction));

    float baseAngle = (2.0f*PI)/sides;

    rlBegin(RL_TRIANGLES);
        rlColor4ub(color.r, color.g, color.b, color.a);

        for (int i = 0; i < sides; i++) {
            // compute the four vertices
            float s1 = sinf(baseAngle*(i + 0))*startRadius;
            float c1 = cosf(baseAngle*(i + 0))*startRadius;
            Vector3 w1 = { startPos.x + s1*b1.x + c1*b2.x, startPos.y + s1*b1.y + c1*b2.y, startPos.z + s1*b1.z + c1*b2.z };
            float s2 = sinf(baseAngle*(i + 1))*startRadius;
            float c2 = cosf(baseAngle*(i + 1))*startRadius;
            Vector3 w2 = { startPos.x + s2*b1.x + c2*b2.x, startPos.y + s2*b1.y + c2*b2.y, startPos.z + s2*b1.z + c2*b2.z };
            float s3 = sinf(baseAngle*(i + 0))*endRadius;
            float c3 = cosf(baseAngle*(i + 0))*endRadius;
            Vector3 w3 = { endPos.x + s3*b1.x + c3*b2.x, endPos.y + s3*b1.y + c3*b2.y, endPos.z + s3*b1.z + c3*b2.z };
            float s4 = sinf(baseAngle*(i + 1))*endRadius;
            float c4 = cosf(baseAngle*(i + 1))*endRadius;
            Vector3 w4 = { endPos.x + s4*b1.x + c4*b2.x, endPos.y + s4*b1.y + c4*b2.y, endPos.z + s4*b1.z + c4*b2.z };

            if (startRadius > 0) {                              //
                rlVertex3f(startPos.x, startPos.y, startPos.z); // |
                rlVertex3f(w2.x, w2.y, w2.z);                   // T0
                rlVertex3f(w1.x, w1.y, w1.z);                   // |
            }                                                   //
                                                                //          w2 x.-----------x startPos
            rlVertex3f(w1.x, w1.y, w1.z);                       // |           |\'.  T0    /
            rlVertex3f(w2.x, w2.y, w2.z);                       // T1          | \ '.     /
            rlVertex3f(w3.x, w3.y, w3.z);                       // |           |T \  '.  /
                                                                //             | 2 \ T 'x w1
            rlVertex3f(w2.x, w2.y, w2.z);                       // |        w4 x.---\-1-|---x endPos
            rlVertex3f(w4.x, w4.y, w4.z);                       // T2            '.  \  |T3/
            rlVertex3f(w3.x, w3.y, w3.z);                       // |               '. \ | /
                                                                //                   '.\|/
            if (endRadius > 0) {                                //                     'x w3
                rlVertex3f(endPos.x, endPos.y, endPos.z);       // |
                rlVertex3f(w3.x, w3.y, w3.z);                   // T3
                rlVertex3f(w4.x, w4.y, w4.z);                   // |
            }                                                   //
        }
    rlEnd();
}

// Draw a wired cylinder
// NOTE: It could be also used for pyramid and cone
void DrawCylinderWires(Vector3 position, float radiusTop, float radiusBottom, float height, int sides, Color color)
{
    if (sides < 3) sides = 3;

    int numVertex = sides*8;
    rlCheckRenderBatchLimit(numVertex);

    rlPushMatrix();
        rlTranslatef(position.x, position.y, position.z);

        rlBegin(RL_LINES);
            rlColor4ub(color.r, color.g, color.b, color.a);

            for (int i = 0; i < 360; i += 360/sides)
            {
                rlVertex3f(sinf(DEG2RAD*i)*radiusBottom, 0, cosf(DEG2RAD*i)*radiusBottom);
                rlVertex3f(sinf(DEG2RAD*(i + 360.0f/sides))*radiusBottom, 0, cosf(DEG2RAD*(i + 360.0f/sides))*radiusBottom);

                rlVertex3f(sinf(DEG2RAD*(i + 360.0f/sides))*radiusBottom, 0, cosf(DEG2RAD*(i + 360.0f/sides))*radiusBottom);
                rlVertex3f(sinf(DEG2RAD*(i + 360.0f/sides))*radiusTop, height, cosf(DEG2RAD*(i + 360.0f/sides))*radiusTop);

                rlVertex3f(sinf(DEG2RAD*(i + 360.0f/sides))*radiusTop, height, cosf(DEG2RAD*(i + 360.0f/sides))*radiusTop);
                rlVertex3f(sinf(DEG2RAD*i)*radiusTop, height, cosf(DEG2RAD*i)*radiusTop);

                rlVertex3f(sinf(DEG2RAD*i)*radiusTop, height, cosf(DEG2RAD*i)*radiusTop);
                rlVertex3f(sinf(DEG2RAD*i)*radiusBottom, 0, cosf(DEG2RAD*i)*radiusBottom);
            }
        rlEnd();
    rlPopMatrix();
}


// Draw a wired cylinder with base at startPos and top at endPos
// NOTE: It could be also used for pyramid and cone
void DrawCylinderWiresEx(Vector3 startPos, Vector3 endPos, float startRadius, float endRadius, int sides, Color color)
{
    if (sides < 3) sides = 3;

    int numVertex = sides*6;
    rlCheckRenderBatchLimit(numVertex);

    Vector3 direction = { endPos.x - startPos.x, endPos.y - startPos.y, endPos.z - startPos.z };
    if ((direction.x == 0) && (direction.y == 0) && (direction.z == 0))return;

    // Construct a basis of the base and the top face:
    Vector3 b1 = Vector3Normalize(Vector3Perpendicular(direction));
    Vector3 b2 = Vector3Normalize(Vector3CrossProduct(b1, direction));

    float baseAngle = (2.0f*PI)/sides;

    rlBegin(RL_LINES);
        rlColor4ub(color.r, color.g, color.b, color.a);

        for (int i = 0; i < sides; i++) {
            // compute the four vertices
            float s1 = sinf(baseAngle*(i + 0))*startRadius;
            float c1 = cosf(baseAngle*(i + 0))*startRadius;
            Vector3 w1 = { startPos.x + s1*b1.x + c1*b2.x, startPos.y + s1*b1.y + c1*b2.y, startPos.z + s1*b1.z + c1*b2.z };
            float s2 = sinf(baseAngle*(i + 1))*startRadius;
            float c2 = cosf(baseAngle*(i + 1))*startRadius;
            Vector3 w2 = { startPos.x + s2*b1.x + c2*b2.x, startPos.y + s2*b1.y + c2*b2.y, startPos.z + s2*b1.z + c2*b2.z };
            float s3 = sinf(baseAngle*(i + 0))*endRadius;
            float c3 = cosf(baseAngle*(i + 0))*endRadius;
            Vector3 w3 = { endPos.x + s3*b1.x + c3*b2.x, endPos.y + s3*b1.y + c3*b2.y, endPos.z + s3*b1.z + c3*b2.z };
            float s4 = sinf(baseAngle*(i + 1))*endRadius;
            float c4 = cosf(baseAngle*(i + 1))*endRadius;
            Vector3 w4 = { endPos.x + s4*b1.x + c4*b2.x, endPos.y + s4*b1.y + c4*b2.y, endPos.z + s4*b1.z + c4*b2.z };

            rlVertex3f(w1.x, w1.y, w1.z);
            rlVertex3f(w2.x, w2.y, w2.z);

            rlVertex3f(w1.x, w1.y, w1.z);
            rlVertex3f(w3.x, w3.y, w3.z);

            rlVertex3f(w3.x, w3.y, w3.z);
            rlVertex3f(w4.x, w4.y, w4.z);
        }
    rlEnd();
}


// Draw a plane
void DrawPlane(Vector3 centerPos, Vector2 size, Color color)
{
    rlCheckRenderBatchLimit(4);

    // NOTE: Plane is always created on XZ ground
    rlPushMatrix();
        rlTranslatef(centerPos.x, centerPos.y, centerPos.z);
        rlScalef(size.x, 1.0f, size.y);

        rlBegin(RL_QUADS);
            rlColor4ub(color.r, color.g, color.b, color.a);
            rlNormal3f(0.0f, 1.0f, 0.0f);

            rlVertex3f(-0.5f, 0.0f, -0.5f);
            rlVertex3f(-0.5f, 0.0f, 0.5f);
            rlVertex3f(0.5f, 0.0f, 0.5f);
            rlVertex3f(0.5f, 0.0f, -0.5f);
        rlEnd();
    rlPopMatrix();
}

// Draw a grid centered at (0, 0, 0)
void DrawGrid(int slices, float spacing)
{
    int halfSlices = slices/2;

    rlCheckRenderBatchLimit((slices + 2)*4);

    rlBegin(RL_LINES);
        for (int i = -halfSlices; i <= halfSlices; i++)
        {
            if (i == 0)
            {
                rlColor3f(0.5f, 0.5f, 0.5f);
                rlColor3f(0.5f, 0.5f, 0.5f);
                rlColor3f(0.5f, 0.5f, 0.5f);
                rlColor3f(0.5f, 0.5f, 0.5f);
            }
            else
            {
                rlColor3f(0.75f, 0.75f, 0.75f);
                rlColor3f(0.75f, 0.75f, 0.75f);
                rlColor3f(0.75f, 0.75f, 0.75f);
                rlColor3f(0.75f, 0.75f, 0.75f);
            }

            rlVertex3f((float)i*spacing, 0.0f, (float)-halfSlices*spacing);
            rlVertex3f((float)i*spacing, 0.0f, (float)halfSlices*spacing);

            rlVertex3f((float)-halfSlices*spacing, 0.0f, (float)i*spacing);
            rlVertex3f((float)halfSlices*spacing, 0.0f, (float)i*spacing);
        }
    rlEnd();
}

// Load model from files (mesh and material)
Model LoadModel(const char *fileName)
{
    Model model = { 0 };
    if (IsFileExtension(fileName, ".obj"))
        model = LoadOBJ(fileName);
    // Make sure model transform is set to identity matrix!
    model.transform = MatrixIdentity();
    if (model.meshCount == 0)
    {
        model.meshCount = 1;
        model.meshes = (Mesh *)RL_CALLOC(model.meshCount, sizeof(Mesh));
#if defined(SUPPORT_MESH_GENERATION)
        TRACELOG(LOG_WARNING, "MESH: [%s] Failed to load mesh data, default to cube mesh", fileName);
        model.meshes[0] = GenMeshCube(1.0f, 1.0f, 1.0f);
#else
        TRACELOG(LOG_WARNING, "MESH: [%s] Failed to load mesh data", fileName);
#endif
    }
    else
    {
        // Upload vertex data to GPU (static mesh)
        for (int i = 0; i < model.meshCount; i++) UploadMesh(&model.meshes[i], false);
    }

    if (model.materialCount == 0)
    {
        TRACELOG(LOG_WARNING, "MATERIAL: [%s] Failed to load material data, default to white material", fileName);

        model.materialCount = 1;
        model.materials = (Material *)RL_CALLOC(model.materialCount, sizeof(Material));
        model.materials[0] = LoadMaterialDefault();

        if (model.meshMaterial == NULL) model.meshMaterial = (int *)RL_CALLOC(model.meshCount, sizeof(int));
    }

    return model;
}

// Load model from generated mesh
// WARNING: A shallow copy of mesh is generated, passed by value,
// as long as struct contains pointers to data and some values, we get a copy
// of mesh pointing to same data as original version... be careful!
Model LoadModelFromMesh(Mesh mesh)
{
    Model model = { 0 };

    model.transform = MatrixIdentity();

    model.meshCount = 1;
    model.meshes = (Mesh *)RL_CALLOC(model.meshCount, sizeof(Mesh));
    model.meshes[0] = mesh;

    model.materialCount = 1;
    model.materials = (Material *)RL_CALLOC(model.materialCount, sizeof(Material));
    model.materials[0] = LoadMaterialDefault();

    model.meshMaterial = (int *)RL_CALLOC(model.meshCount, sizeof(int));
    model.meshMaterial[0] = 0;  // First material index

    return model;
}

// Unload model (meshes/materials) from memory (RAM and/or VRAM)
// NOTE: This function takes care of all model elements, for a detailed control
// over them, use UnloadMesh() and UnloadMaterial()
void UnloadModel(Model model)
{
    // Unload meshes
    for (int i = 0; i < model.meshCount; i++) UnloadMesh(model.meshes[i]);

    // Unload materials maps
    // NOTE: As the user could be sharing shaders and textures between models,
    // we don't unload the material but just free it's maps,
    // the user is responsible for freeing models shaders and textures
    for (int i = 0; i < model.materialCount; i++) RL_FREE(model.materials[i].maps);

    // Unload arrays
    RL_FREE(model.meshes);
    RL_FREE(model.materials);
    RL_FREE(model.meshMaterial);
    TRACELOG(LOG_INFO, "MODEL: Unloaded model (and meshes) from RAM and VRAM");
}

// Unload model (but not meshes) from memory (RAM and/or VRAM)
void UnloadModelKeepMeshes(Model model)
{
    // Unload materials maps
    // NOTE: As the user could be sharing shaders and textures between models,
    // we don't unload the material but just free it's maps,
    // the user is responsible for freeing models shaders and textures
    for (int i = 0; i < model.materialCount; i++) RL_FREE(model.materials[i].maps);

    // Unload arrays
    RL_FREE(model.meshes);
    RL_FREE(model.materials);
    RL_FREE(model.meshMaterial);
    TRACELOG(LOG_INFO, "MODEL: Unloaded model (but not meshes) from RAM and VRAM");
}

// Upload vertex data into a VAO (if supported) and VBO
void UploadMesh(Mesh *mesh, bool dynamic)
{
    if (mesh->vaoId > 0) {
        // Check if mesh has already been loaded in GPU
        TRACELOG(LOG_WARNING, "VAO: [ID %i] Trying to re-load an already loaded mesh", mesh->vaoId);
        return;
    }
    mesh->vboId = (unsigned int *)RL_CALLOC(MAX_MESH_VERTEX_BUFFERS, sizeof(unsigned int));
    mesh->vaoId = 0;        // Vertex Array Object
    mesh->vboId[0] = 0;     // Vertex buffer: positions
    mesh->vboId[1] = 0;     // Vertex buffer: texcoords
    mesh->vboId[2] = 0;     // Vertex buffer: normals
    mesh->vboId[3] = 0;     // Vertex buffer: colors
    mesh->vboId[4] = 0;     // Vertex buffer: tangents
    mesh->vboId[5] = 0;     // Vertex buffer: texcoords2
    mesh->vboId[6] = 0;     // Vertex buffer: indices
    mesh->vaoId = rlLoadVertexArray();
    rlEnableVertexArray(mesh->vaoId);
    // NOTE: Attributes must be uploaded considering default locations points
    // Enable vertex attributes: position (shader-location = 0)
    void *vertices = mesh->animVertices != NULL ? mesh->animVertices : mesh->vertices;
    mesh->vboId[0] = rlLoadVertexBuffer(vertices, mesh->vertexCount*3*sizeof(float), dynamic);
    rlSetVertexAttribute(0, 3, RL_FLOAT, 0, 0, 0);
    rlEnableVertexAttribute(0);
    // Enable vertex attributes: texcoords (shader-location = 1)
    mesh->vboId[1] = rlLoadVertexBuffer(mesh->texcoords, mesh->vertexCount*2*sizeof(float), dynamic);
    rlSetVertexAttribute(1, 2, RL_FLOAT, 0, 0, 0);
    rlEnableVertexAttribute(1);
    if (mesh->normals != NULL) {
        // Enable vertex attributes: normals (shader-location = 2)
        void *normals = mesh->animNormals != NULL ? mesh->animNormals : mesh->normals;
        mesh->vboId[2] = rlLoadVertexBuffer(normals, mesh->vertexCount*3*sizeof(float), dynamic);
        rlSetVertexAttribute(2, 3, RL_FLOAT, 0, 0, 0);
        rlEnableVertexAttribute(2);
    } else {
        // Default color vertex attribute set to WHITE
        float value[3] = { 1.0f, 1.0f, 1.0f };
        rlSetVertexAttributeDefault(2, value, SHADER_ATTRIB_VEC3, 3);
        rlDisableVertexAttribute(2);
    }
    if (mesh->colors != NULL) {
        // Enable vertex attribute: color (shader-location = 3)
        mesh->vboId[3] = rlLoadVertexBuffer(mesh->colors, mesh->vertexCount*4*sizeof(unsigned char), dynamic);
        rlSetVertexAttribute(3, 4, RL_UNSIGNED_BYTE, 1, 0, 0);
        rlEnableVertexAttribute(3);
    } else {
        // Default color vertex attribute set to WHITE
        float value[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        rlSetVertexAttributeDefault(3, value, SHADER_ATTRIB_VEC4, 4);
        rlDisableVertexAttribute(3);
    }
    if (mesh->tangents != NULL) {
        // Enable vertex attribute: tangent (shader-location = 4)
        mesh->vboId[4] = rlLoadVertexBuffer(mesh->tangents, mesh->vertexCount*4*sizeof(float), dynamic);
        rlSetVertexAttribute(4, 4, RL_FLOAT, 0, 0, 0);
        rlEnableVertexAttribute(4);
    } else {
        // Default tangents vertex attribute
        float value[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        rlSetVertexAttributeDefault(4, value, SHADER_ATTRIB_VEC4, 4);
        rlDisableVertexAttribute(4);
    }
    if (mesh->texcoords2 != NULL) {
        // Enable vertex attribute: texcoord2 (shader-location = 5)
        mesh->vboId[5] = rlLoadVertexBuffer(mesh->texcoords2, mesh->vertexCount*2*sizeof(float), dynamic);
        rlSetVertexAttribute(5, 2, RL_FLOAT, 0, 0, 0);
        rlEnableVertexAttribute(5);
    } else {
        // Default texcoord2 vertex attribute
        float value[2] = { 0.0f, 0.0f };
        rlSetVertexAttributeDefault(5, value, SHADER_ATTRIB_VEC2, 2);
        rlDisableVertexAttribute(5);
    }
    if (mesh->indices != NULL)
        mesh->vboId[6] = rlLoadVertexBufferElement(mesh->indices, mesh->triangleCount*3*sizeof(unsigned short), dynamic);
    if (mesh->vaoId > 0)
        TRACELOG(LOG_INFO, "VAO: [ID %i] Mesh uploaded successfully to VRAM (GPU)", mesh->vaoId);
    else TRACELOG(LOG_INFO, "VBO: Mesh uploaded successfully to VRAM (GPU)");
    rlDisableVertexArray();
}

// Update mesh vertex data in GPU for a specific buffer index
void UpdateMeshBuffer(Mesh mesh, int index, void *data, int dataSize, int offset)
{
    rlUpdateVertexBuffer(mesh.vboId[index], data, dataSize, offset);
}

// Draw a 3d mesh with material and transform
void DrawMesh(Mesh mesh, Material material, Matrix transform)
{
    // Bind shader program
    rlEnableShader(material.shader.id);

    // Send required data to shader (matrices, values)
    //-----------------------------------------------------
    // Upload to shader material.colDiffuse
    if (material.shader.locs[SHADER_LOC_COLOR_DIFFUSE] != -1)
    {
        float values[4] = {
            (float)material.maps[MATERIAL_MAP_DIFFUSE].color.r/255.0f,
            (float)material.maps[MATERIAL_MAP_DIFFUSE].color.g/255.0f,
            (float)material.maps[MATERIAL_MAP_DIFFUSE].color.b/255.0f,
            (float)material.maps[MATERIAL_MAP_DIFFUSE].color.a/255.0f
        };

        rlSetUniform(material.shader.locs[SHADER_LOC_COLOR_DIFFUSE], values, SHADER_UNIFORM_VEC4, 1);
    }

    // Upload to shader material.colSpecular (if location available)
    if (material.shader.locs[SHADER_LOC_COLOR_SPECULAR] != -1)
    {
        float values[4] = {
            (float)material.maps[SHADER_LOC_COLOR_SPECULAR].color.r/255.0f,
            (float)material.maps[SHADER_LOC_COLOR_SPECULAR].color.g/255.0f,
            (float)material.maps[SHADER_LOC_COLOR_SPECULAR].color.b/255.0f,
            (float)material.maps[SHADER_LOC_COLOR_SPECULAR].color.a/255.0f
        };

        rlSetUniform(material.shader.locs[SHADER_LOC_COLOR_SPECULAR], values, SHADER_UNIFORM_VEC4, 1);
    }

    // Get a copy of current matrices to work with,
    // just in case stereo render is required and we need to modify them
    // NOTE: At this point the modelview matrix just contains the view matrix (camera)
    // That's because BeginMode3D() sets it and there is no model-drawing function
    // that modifies it, all use rlPushMatrix() and rlPopMatrix()
    Matrix matModel = MatrixIdentity();
    Matrix matView = rlGetMatrixModelview();
    Matrix matModelView = MatrixIdentity();
    Matrix matProjection = rlGetMatrixProjection();

    // Upload view and projection matrices (if locations available)
    if (material.shader.locs[SHADER_LOC_MATRIX_VIEW] != -1) rlSetUniformMatrix(material.shader.locs[SHADER_LOC_MATRIX_VIEW], matView);
    if (material.shader.locs[SHADER_LOC_MATRIX_PROJECTION] != -1) rlSetUniformMatrix(material.shader.locs[SHADER_LOC_MATRIX_PROJECTION], matProjection);

    // Model transformation matrix is send to shader uniform location: SHADER_LOC_MATRIX_MODEL
    if (material.shader.locs[SHADER_LOC_MATRIX_MODEL] != -1) rlSetUniformMatrix(material.shader.locs[SHADER_LOC_MATRIX_MODEL], transform);

    // Accumulate several model transformations:
    //    transform: model transformation provided (includes DrawModel() params combined with model.transform)
    //    rlGetMatrixTransform(): rlgl internal transform matrix due to push/pop matrix stack
    matModel = MatrixMultiply(transform, rlGetMatrixTransform());

    // Get model-view matrix
    matModelView = MatrixMultiply(matModel, matView);

    // Upload model normal matrix (if locations available)
    if (material.shader.locs[SHADER_LOC_MATRIX_NORMAL] != -1) rlSetUniformMatrix(material.shader.locs[SHADER_LOC_MATRIX_NORMAL], MatrixTranspose(MatrixInvert(matModel)));
    //-----------------------------------------------------

    // Bind active texture maps (if available)
    for (int i = 0; i < MAX_MATERIAL_MAPS; i++)
    {
        if (material.maps[i].texture.id > 0)
        {
            // Select current shader texture slot
            rlActiveTextureSlot(i);

            // Enable texture for active slot
            if ((i == MATERIAL_MAP_IRRADIANCE) ||
                (i == MATERIAL_MAP_PREFILTER) ||
                (i == MATERIAL_MAP_CUBEMAP)) rlEnableTextureCubemap(material.maps[i].texture.id);
            else rlEnableTexture(material.maps[i].texture.id);

            rlSetUniform(material.shader.locs[SHADER_LOC_MAP_DIFFUSE + i], &i, SHADER_UNIFORM_INT, 1);
        }
    }

    // Try binding vertex array objects (VAO)
    // or use VBOs if not possible
    if (!rlEnableVertexArray(mesh.vaoId))
    {
        // Bind mesh VBO data: vertex position (shader-location = 0)
        rlEnableVertexBuffer(mesh.vboId[0]);
        rlSetVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_POSITION], 3, RL_FLOAT, 0, 0, 0);
        rlEnableVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_POSITION]);

        // Bind mesh VBO data: vertex texcoords (shader-location = 1)
        rlEnableVertexBuffer(mesh.vboId[1]);
        rlSetVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_TEXCOORD01], 2, RL_FLOAT, 0, 0, 0);
        rlEnableVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_TEXCOORD01]);

        if (material.shader.locs[SHADER_LOC_VERTEX_NORMAL] != -1)
        {
            // Bind mesh VBO data: vertex normals (shader-location = 2)
            rlEnableVertexBuffer(mesh.vboId[2]);
            rlSetVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_NORMAL], 3, RL_FLOAT, 0, 0, 0);
            rlEnableVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_NORMAL]);
        }

        // Bind mesh VBO data: vertex colors (shader-location = 3, if available)
        if (material.shader.locs[SHADER_LOC_VERTEX_COLOR] != -1)
        {
            if (mesh.vboId[3] != 0)
            {
                rlEnableVertexBuffer(mesh.vboId[3]);
                rlSetVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_COLOR], 4, RL_UNSIGNED_BYTE, 1, 0, 0);
                rlEnableVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_COLOR]);
            }
            else
            {
                // Set default value for unused attribute
                // NOTE: Required when using default shader and no VAO support
                float value[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
                rlSetVertexAttributeDefault(material.shader.locs[SHADER_LOC_VERTEX_COLOR], value, SHADER_ATTRIB_VEC2, 4);
                rlDisableVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_COLOR]);
            }
        }

        // Bind mesh VBO data: vertex tangents (shader-location = 4, if available)
        if (material.shader.locs[SHADER_LOC_VERTEX_TANGENT] != -1)
        {
            rlEnableVertexBuffer(mesh.vboId[4]);
            rlSetVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_TANGENT], 4, RL_FLOAT, 0, 0, 0);
            rlEnableVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_TANGENT]);
        }

        // Bind mesh VBO data: vertex texcoords2 (shader-location = 5, if available)
        if (material.shader.locs[SHADER_LOC_VERTEX_TEXCOORD02] != -1)
        {
            rlEnableVertexBuffer(mesh.vboId[5]);
            rlSetVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_TEXCOORD02], 2, RL_FLOAT, 0, 0, 0);
            rlEnableVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_TEXCOORD02]);
        }

        if (mesh.indices != NULL) rlEnableVertexBufferElement(mesh.vboId[6]);
    }

    int eyeCount = 1;
    if (rlIsStereoRenderEnabled()) eyeCount = 2;

    for (int eye = 0; eye < eyeCount; eye++)
    {
        // Calculate model-view-projection matrix (MVP)
        Matrix matModelViewProjection = MatrixIdentity();
        if (eyeCount == 1) matModelViewProjection = MatrixMultiply(matModelView, matProjection);
        else
        {
            // Setup current eye viewport (half screen width)
            rlViewport(eye*rlGetFramebufferWidth()/2, 0, rlGetFramebufferWidth()/2, rlGetFramebufferHeight());
            matModelViewProjection = MatrixMultiply(MatrixMultiply(matModelView, rlGetMatrixViewOffsetStereo(eye)), rlGetMatrixProjectionStereo(eye));
        }

        // Send combined model-view-projection matrix to shader
        rlSetUniformMatrix(material.shader.locs[SHADER_LOC_MATRIX_MVP], matModelViewProjection);

        // Draw mesh
        if (mesh.indices != NULL) rlDrawVertexArrayElements(0, mesh.triangleCount*3, 0);
        else rlDrawVertexArray(0, mesh.vertexCount);
    }

    // Unbind all binded texture maps
    for (int i = 0; i < MAX_MATERIAL_MAPS; i++)
    {
        // Select current shader texture slot
        rlActiveTextureSlot(i);

        // Disable texture for active slot
        if ((i == MATERIAL_MAP_IRRADIANCE) ||
            (i == MATERIAL_MAP_PREFILTER) ||
            (i == MATERIAL_MAP_CUBEMAP)) rlDisableTextureCubemap();
        else rlDisableTexture();
    }

    // Disable all possible vertex array objects (or VBOs)
    rlDisableVertexArray();
    rlDisableVertexBuffer();
    rlDisableVertexBufferElement();

    // Disable shader program
    rlDisableShader();

    // Restore rlgl internal modelview and projection matrices
    rlSetMatrixModelview(matView);
    rlSetMatrixProjection(matProjection);
}

// Draw multiple mesh instances with material and different transforms
void DrawMeshInstanced(Mesh mesh, Material material, Matrix *transforms, int instances)
{
    // Instancing required variables
    float16 *instanceTransforms = NULL;
    unsigned int instancesVboId = 0;

    // Bind shader program
    rlEnableShader(material.shader.id);

    // Send required data to shader (matrices, values)
    //-----------------------------------------------------
    // Upload to shader material.colDiffuse
    if (material.shader.locs[SHADER_LOC_COLOR_DIFFUSE] != -1)
    {
        float values[4] = {
            (float)material.maps[MATERIAL_MAP_DIFFUSE].color.r/255.0f,
            (float)material.maps[MATERIAL_MAP_DIFFUSE].color.g/255.0f,
            (float)material.maps[MATERIAL_MAP_DIFFUSE].color.b/255.0f,
            (float)material.maps[MATERIAL_MAP_DIFFUSE].color.a/255.0f
        };

        rlSetUniform(material.shader.locs[SHADER_LOC_COLOR_DIFFUSE], values, SHADER_UNIFORM_VEC4, 1);
    }

    // Upload to shader material.colSpecular (if location available)
    if (material.shader.locs[SHADER_LOC_COLOR_SPECULAR] != -1)
    {
        float values[4] = {
            (float)material.maps[SHADER_LOC_COLOR_SPECULAR].color.r/255.0f,
            (float)material.maps[SHADER_LOC_COLOR_SPECULAR].color.g/255.0f,
            (float)material.maps[SHADER_LOC_COLOR_SPECULAR].color.b/255.0f,
            (float)material.maps[SHADER_LOC_COLOR_SPECULAR].color.a/255.0f
        };

        rlSetUniform(material.shader.locs[SHADER_LOC_COLOR_SPECULAR], values, SHADER_UNIFORM_VEC4, 1);
    }

    // Get a copy of current matrices to work with,
    // just in case stereo render is required and we need to modify them
    // NOTE: At this point the modelview matrix just contains the view matrix (camera)
    // That's because BeginMode3D() sets it and there is no model-drawing function
    // that modifies it, all use rlPushMatrix() and rlPopMatrix()
    Matrix matModel = MatrixIdentity();
    Matrix matView = rlGetMatrixModelview();
    Matrix matModelView = MatrixIdentity();
    Matrix matProjection = rlGetMatrixProjection();

    // Upload view and projection matrices (if locations available)
    if (material.shader.locs[SHADER_LOC_MATRIX_VIEW] != -1) rlSetUniformMatrix(material.shader.locs[SHADER_LOC_MATRIX_VIEW], matView);
    if (material.shader.locs[SHADER_LOC_MATRIX_PROJECTION] != -1) rlSetUniformMatrix(material.shader.locs[SHADER_LOC_MATRIX_PROJECTION], matProjection);

    // Create instances buffer
    instanceTransforms = (float16 *)RL_MALLOC(instances*sizeof(float16));

    // Fill buffer with instances transformations as float16 arrays
    for (int i = 0; i < instances; i++) instanceTransforms[i] = MatrixToFloatV(transforms[i]);

    // Enable mesh VAO to attach new buffer
    rlEnableVertexArray(mesh.vaoId);

    // This could alternatively use a static VBO and either glMapBuffer() or glBufferSubData().
    // It isn't clear which would be reliably faster in all cases and on all platforms,
    // anecdotally glMapBuffer() seems very slow (syncs) while glBufferSubData() seems
    // no faster, since we're transferring all the transform matrices anyway
    instancesVboId = rlLoadVertexBuffer(instanceTransforms, instances*sizeof(float16), false);

    // Instances transformation matrices are send to shader attribute location: SHADER_LOC_MATRIX_MODEL
    for (unsigned int i = 0; i < 4; i++)
    {
        rlEnableVertexAttribute(material.shader.locs[SHADER_LOC_MATRIX_MODEL] + i);
        rlSetVertexAttribute(material.shader.locs[SHADER_LOC_MATRIX_MODEL] + i, 4, RL_FLOAT, 0, sizeof(Matrix), (void *)(i*sizeof(Vector4)));
        rlSetVertexAttributeDivisor(material.shader.locs[SHADER_LOC_MATRIX_MODEL] + i, 1);
    }

    rlDisableVertexBuffer();
    rlDisableVertexArray();

    // Accumulate internal matrix transform (push/pop) and view matrix
    // NOTE: In this case, model instance transformation must be computed in the shader
    matModelView = MatrixMultiply(rlGetMatrixTransform(), matView);

    // Upload model normal matrix (if locations available)
    if (material.shader.locs[SHADER_LOC_MATRIX_NORMAL] != -1) rlSetUniformMatrix(material.shader.locs[SHADER_LOC_MATRIX_NORMAL], MatrixTranspose(MatrixInvert(matModel)));
    //-----------------------------------------------------

    // Bind active texture maps (if available)
    for (int i = 0; i < MAX_MATERIAL_MAPS; i++)
    {
        if (material.maps[i].texture.id > 0)
        {
            // Select current shader texture slot
            rlActiveTextureSlot(i);

            // Enable texture for active slot
            if ((i == MATERIAL_MAP_IRRADIANCE) ||
                (i == MATERIAL_MAP_PREFILTER) ||
                (i == MATERIAL_MAP_CUBEMAP)) rlEnableTextureCubemap(material.maps[i].texture.id);
            else rlEnableTexture(material.maps[i].texture.id);

            rlSetUniform(material.shader.locs[SHADER_LOC_MAP_DIFFUSE + i], &i, SHADER_UNIFORM_INT, 1);
        }
    }

    // Try binding vertex array objects (VAO)
    // or use VBOs if not possible
    if (!rlEnableVertexArray(mesh.vaoId))
    {
        // Bind mesh VBO data: vertex position (shader-location = 0)
        rlEnableVertexBuffer(mesh.vboId[0]);
        rlSetVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_POSITION], 3, RL_FLOAT, 0, 0, 0);
        rlEnableVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_POSITION]);

        // Bind mesh VBO data: vertex texcoords (shader-location = 1)
        rlEnableVertexBuffer(mesh.vboId[1]);
        rlSetVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_TEXCOORD01], 2, RL_FLOAT, 0, 0, 0);
        rlEnableVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_TEXCOORD01]);

        if (material.shader.locs[SHADER_LOC_VERTEX_NORMAL] != -1)
        {
            // Bind mesh VBO data: vertex normals (shader-location = 2)
            rlEnableVertexBuffer(mesh.vboId[2]);
            rlSetVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_NORMAL], 3, RL_FLOAT, 0, 0, 0);
            rlEnableVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_NORMAL]);
        }

        // Bind mesh VBO data: vertex colors (shader-location = 3, if available)
        if (material.shader.locs[SHADER_LOC_VERTEX_COLOR] != -1)
        {
            if (mesh.vboId[3] != 0)
            {
                rlEnableVertexBuffer(mesh.vboId[3]);
                rlSetVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_COLOR], 4, RL_UNSIGNED_BYTE, 1, 0, 0);
                rlEnableVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_COLOR]);
            }
            else
            {
                // Set default value for unused attribute
                // NOTE: Required when using default shader and no VAO support
                float value[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
                rlSetVertexAttributeDefault(material.shader.locs[SHADER_LOC_VERTEX_COLOR], value, SHADER_ATTRIB_VEC2, 4);
                rlDisableVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_COLOR]);
            }
        }

        // Bind mesh VBO data: vertex tangents (shader-location = 4, if available)
        if (material.shader.locs[SHADER_LOC_VERTEX_TANGENT] != -1)
        {
            rlEnableVertexBuffer(mesh.vboId[4]);
            rlSetVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_TANGENT], 4, RL_FLOAT, 0, 0, 0);
            rlEnableVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_TANGENT]);
        }

        // Bind mesh VBO data: vertex texcoords2 (shader-location = 5, if available)
        if (material.shader.locs[SHADER_LOC_VERTEX_TEXCOORD02] != -1)
        {
            rlEnableVertexBuffer(mesh.vboId[5]);
            rlSetVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_TEXCOORD02], 2, RL_FLOAT, 0, 0, 0);
            rlEnableVertexAttribute(material.shader.locs[SHADER_LOC_VERTEX_TEXCOORD02]);
        }

        if (mesh.indices != NULL) rlEnableVertexBufferElement(mesh.vboId[6]);
    }

    int eyeCount = 1;
    if (rlIsStereoRenderEnabled()) eyeCount = 2;

    for (int eye = 0; eye < eyeCount; eye++)
    {
        // Calculate model-view-projection matrix (MVP)
        Matrix matModelViewProjection = MatrixIdentity();
        if (eyeCount == 1) matModelViewProjection = MatrixMultiply(matModelView, matProjection);
        else
        {
            // Setup current eye viewport (half screen width)
            rlViewport(eye*rlGetFramebufferWidth()/2, 0, rlGetFramebufferWidth()/2, rlGetFramebufferHeight());
            matModelViewProjection = MatrixMultiply(MatrixMultiply(matModelView, rlGetMatrixViewOffsetStereo(eye)), rlGetMatrixProjectionStereo(eye));
        }

        // Send combined model-view-projection matrix to shader
        rlSetUniformMatrix(material.shader.locs[SHADER_LOC_MATRIX_MVP], matModelViewProjection);

        // Draw mesh instanced
        if (mesh.indices != NULL) rlDrawVertexArrayElementsInstanced(0, mesh.triangleCount*3, 0, instances);
        else rlDrawVertexArrayInstanced(0, mesh.vertexCount, instances);
    }

    // Unbind all binded texture maps
    for (int i = 0; i < MAX_MATERIAL_MAPS; i++)
    {
        // Select current shader texture slot
        rlActiveTextureSlot(i);

        // Disable texture for active slot
        if ((i == MATERIAL_MAP_IRRADIANCE) ||
            (i == MATERIAL_MAP_PREFILTER) ||
            (i == MATERIAL_MAP_CUBEMAP)) rlDisableTextureCubemap();
        else rlDisableTexture();
    }

    // Disable all possible vertex array objects (or VBOs)
    rlDisableVertexArray();
    rlDisableVertexBuffer();
    rlDisableVertexBufferElement();

    // Disable shader program
    rlDisableShader();

    // Remove instance transforms buffer
    rlUnloadVertexBuffer(instancesVboId);
    RL_FREE(instanceTransforms);
}

// Unload mesh from memory (RAM and VRAM)
void UnloadMesh(Mesh mesh)
{
    // Unload rlgl mesh vboId data
    rlUnloadVertexArray(mesh.vaoId);

    for (int i = 0; i < MAX_MESH_VERTEX_BUFFERS; i++) rlUnloadVertexBuffer(mesh.vboId[i]);
    RL_FREE(mesh.vboId);

    RL_FREE(mesh.vertices);
    RL_FREE(mesh.texcoords);
    RL_FREE(mesh.normals);
    RL_FREE(mesh.colors);
    RL_FREE(mesh.tangents);
    RL_FREE(mesh.texcoords2);
    RL_FREE(mesh.indices);

    RL_FREE(mesh.animVertices);
    RL_FREE(mesh.animNormals);
    RL_FREE(mesh.boneWeights);
    RL_FREE(mesh.boneIds);
}

// Export mesh data to file
bool ExportMesh(Mesh mesh, const char *fileName)
{
    bool success = false;

    if (IsFileExtension(fileName, ".obj"))
    {
        // Estimated data size, it should be enough...
        int dataSize = mesh.vertexCount/3* (int)strlen("v 0000.00f 0000.00f 0000.00f") +
                       mesh.vertexCount/2* (int)strlen("vt 0.000f 0.00f") +
                       mesh.vertexCount/3* (int)strlen("vn 0.000f 0.00f 0.00f") +
                       mesh.triangleCount/3* (int)strlen("f 00000/00000/00000 00000/00000/00000 00000/00000/00000");

        // NOTE: Text data buffer size is estimated considering mesh data size
        char *txtData = (char *)RL_CALLOC(dataSize + 2000, sizeof(char));

        int byteCount = 0;
        byteCount += sprintf(txtData + byteCount, "# //////////////////////////////////////////////////////////////////////////////////\n");
        byteCount += sprintf(txtData + byteCount, "# //                                                                              //\n");
        byteCount += sprintf(txtData + byteCount, "# // rMeshOBJ exporter v1.0 - Mesh exported as triangle faces and not optimized   //\n");
        byteCount += sprintf(txtData + byteCount, "# //                                                                              //\n");
        byteCount += sprintf(txtData + byteCount, "# // more info and bugs-report:  github.com/raysan5/raylib                        //\n");
        byteCount += sprintf(txtData + byteCount, "# // feedback and support:       ray[at]raylib.com                                //\n");
        byteCount += sprintf(txtData + byteCount, "# //                                                                              //\n");
        byteCount += sprintf(txtData + byteCount, "# // Copyright (c) 2018 Ramon Santamaria (@raysan5)                               //\n");
        byteCount += sprintf(txtData + byteCount, "# //                                                                              //\n");
        byteCount += sprintf(txtData + byteCount, "# //////////////////////////////////////////////////////////////////////////////////\n\n");
        byteCount += sprintf(txtData + byteCount, "# Vertex Count:     %i\n", mesh.vertexCount);
        byteCount += sprintf(txtData + byteCount, "# Triangle Count:   %i\n\n", mesh.triangleCount);

        byteCount += sprintf(txtData + byteCount, "g mesh\n");

        for (int i = 0, v = 0; i < mesh.vertexCount; i++, v += 3)
        {
            byteCount += sprintf(txtData + byteCount, "v %.2f %.2f %.2f\n", mesh.vertices[v], mesh.vertices[v + 1], mesh.vertices[v + 2]);
        }

        for (int i = 0, v = 0; i < mesh.vertexCount; i++, v += 2)
        {
            byteCount += sprintf(txtData + byteCount, "vt %.3f %.3f\n", mesh.texcoords[v], mesh.texcoords[v + 1]);
        }

        for (int i = 0, v = 0; i < mesh.vertexCount; i++, v += 3)
        {
            byteCount += sprintf(txtData + byteCount, "vn %.3f %.3f %.3f\n", mesh.normals[v], mesh.normals[v + 1], mesh.normals[v + 2]);
        }

        for (int i = 0; i < mesh.triangleCount; i += 3)
        {
            byteCount += sprintf(txtData + byteCount, "f %i/%i/%i %i/%i/%i %i/%i/%i\n", i, i, i, i + 1, i + 1, i + 1, i + 2, i + 2, i + 2);
        }

        byteCount += sprintf(txtData + byteCount, "\n");

        // NOTE: Text data length exported is determined by '\0' (NULL) character
        success = SaveFileText(fileName, txtData);

        RL_FREE(txtData);
    }
    else if (IsFileExtension(fileName, ".raw"))
    {
        // TODO: Support additional file formats to export mesh vertex data
    }

    return success;
}

// Load default material (Supports: DIFFUSE, SPECULAR, NORMAL maps)
Material LoadMaterialDefault(void)
{
    Material material = { 0 };
    material.maps = (MaterialMap *)RL_CALLOC(MAX_MATERIAL_MAPS, sizeof(MaterialMap));

    // Using rlgl default shader
    material.shader.id = rlGetShaderIdDefault();
    material.shader.locs = rlGetShaderLocsDefault();

    // Using rlgl default texture (1x1 pixel, UNCOMPRESSED_R8G8B8A8, 1 mipmap)
    material.maps[MATERIAL_MAP_DIFFUSE].texture = (Texture2D){ rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
    //material.maps[MATERIAL_MAP_NORMAL].texture;         // NOTE: By default, not set
    //material.maps[MATERIAL_MAP_SPECULAR].texture;       // NOTE: By default, not set

    material.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;    // Diffuse color
    material.maps[MATERIAL_MAP_SPECULAR].color = WHITE;   // Specular color

    return material;
}

// Set texture for a material map type (MATERIAL_MAP_DIFFUSE, MATERIAL_MAP_SPECULAR...)
// NOTE: Previous texture should be manually unloaded
void SetMaterialTexture(Material *material, int mapType, Texture2D texture)
{
    material->maps[mapType].texture = texture;
}

// Set the material for a mesh
void SetModelMeshMaterial(Model *model, int meshId, int materialId)
{
    if (meshId >= model->meshCount) TRACELOG(LOG_WARNING, "MESH: Id greater than mesh count");
    else if (materialId >= model->materialCount) TRACELOG(LOG_WARNING, "MATERIAL: Id greater than material count");
    else  model->meshMaterial[meshId] = materialId;
}

#if defined(SUPPORT_MESH_GENERATION)
// Generate polygonal mesh
Mesh GenMeshPoly(int sides, float radius)
{
    Mesh mesh = { 0 };

    if (sides < 3) return mesh;

    int vertexCount = sides*3;

    // Vertices definition
    Vector3 *vertices = (Vector3 *)RL_MALLOC(vertexCount*sizeof(Vector3));

    float d = 0.0f, dStep = 360.0f/sides;
    for (int v = 0; v < vertexCount; v += 3)
    {
        vertices[v] = (Vector3){ 0.0f, 0.0f, 0.0f };
        vertices[v + 1] = (Vector3){ sinf(DEG2RAD*d)*radius, 0.0f, cosf(DEG2RAD*d)*radius };
        vertices[v + 2] = (Vector3){sinf(DEG2RAD*(d+dStep))*radius, 0.0f, cosf(DEG2RAD*(d+dStep))*radius };
        d += dStep;
    }

    // Normals definition
    Vector3 *normals = (Vector3 *)RL_MALLOC(vertexCount*sizeof(Vector3));
    for (int n = 0; n < vertexCount; n++) normals[n] = (Vector3){ 0.0f, 1.0f, 0.0f };   // Vector3.up;

    // TexCoords definition
    Vector2 *texcoords = (Vector2 *)RL_MALLOC(vertexCount*sizeof(Vector2));
    for (int n = 0; n < vertexCount; n++) texcoords[n] = (Vector2){ 0.0f, 0.0f };

    mesh.vertexCount = vertexCount;
    mesh.triangleCount = sides;
    mesh.vertices = (float *)RL_MALLOC(mesh.vertexCount*3*sizeof(float));
    mesh.texcoords = (float *)RL_MALLOC(mesh.vertexCount*2*sizeof(float));
    mesh.normals = (float *)RL_MALLOC(mesh.vertexCount*3*sizeof(float));

    // Mesh vertices position array
    for (int i = 0; i < mesh.vertexCount; i++)
    {
        mesh.vertices[3*i] = vertices[i].x;
        mesh.vertices[3*i + 1] = vertices[i].y;
        mesh.vertices[3*i + 2] = vertices[i].z;
    }

    // Mesh texcoords array
    for (int i = 0; i < mesh.vertexCount; i++)
    {
        mesh.texcoords[2*i] = texcoords[i].x;
        mesh.texcoords[2*i + 1] = texcoords[i].y;
    }

    // Mesh normals array
    for (int i = 0; i < mesh.vertexCount; i++)
    {
        mesh.normals[3*i] = normals[i].x;
        mesh.normals[3*i + 1] = normals[i].y;
        mesh.normals[3*i + 2] = normals[i].z;
    }

    RL_FREE(vertices);
    RL_FREE(normals);
    RL_FREE(texcoords);

    // Upload vertex data to GPU (static mesh)
    // NOTE: mesh.vboId array is allocated inside UploadMesh()
    UploadMesh(&mesh, false);

    return mesh;
}

// Generate plane mesh (with subdivisions)
Mesh GenMeshPlane(float width, float length, int resX, int resZ)
{
    Mesh mesh = { 0 };

#define CUSTOM_MESH_GEN_PLANE
#if defined(CUSTOM_MESH_GEN_PLANE)
    resX++;
    resZ++;

    // Vertices definition
    int vertexCount = resX*resZ; // vertices get reused for the faces

    Vector3 *vertices = (Vector3 *)RL_MALLOC(vertexCount*sizeof(Vector3));
    for (int z = 0; z < resZ; z++)
    {
        // [-length/2, length/2]
        float zPos = ((float)z/(resZ - 1) - 0.5f)*length;
        for (int x = 0; x < resX; x++)
        {
            // [-width/2, width/2]
            float xPos = ((float)x/(resX - 1) - 0.5f)*width;
            vertices[x + z*resX] = (Vector3){ xPos, 0.0f, zPos };
        }
    }

    // Normals definition
    Vector3 *normals = (Vector3 *)RL_MALLOC(vertexCount*sizeof(Vector3));
    for (int n = 0; n < vertexCount; n++) normals[n] = (Vector3){ 0.0f, 1.0f, 0.0f };   // Vector3.up;

    // TexCoords definition
    Vector2 *texcoords = (Vector2 *)RL_MALLOC(vertexCount*sizeof(Vector2));
    for (int v = 0; v < resZ; v++)
    {
        for (int u = 0; u < resX; u++)
        {
            texcoords[u + v*resX] = (Vector2){ (float)u/(resX - 1), (float)v/(resZ - 1) };
        }
    }

    // Triangles definition (indices)
    int numFaces = (resX - 1)*(resZ - 1);
    int *triangles = (int *)RL_MALLOC(numFaces*6*sizeof(int));
    int t = 0;
    for (int face = 0; face < numFaces; face++)
    {
        // Retrieve lower left corner from face ind
        int i = face % (resX - 1) + (face/(resZ - 1)*resX);

        triangles[t++] = i + resX;
        triangles[t++] = i + 1;
        triangles[t++] = i;

        triangles[t++] = i + resX;
        triangles[t++] = i + resX + 1;
        triangles[t++] = i + 1;
    }

    mesh.vertexCount = vertexCount;
    mesh.triangleCount = numFaces*2;
    mesh.vertices = (float *)RL_MALLOC(mesh.vertexCount*3*sizeof(float));
    mesh.texcoords = (float *)RL_MALLOC(mesh.vertexCount*2*sizeof(float));
    mesh.normals = (float *)RL_MALLOC(mesh.vertexCount*3*sizeof(float));
    mesh.indices = (unsigned short *)RL_MALLOC(mesh.triangleCount*3*sizeof(unsigned short));

    // Mesh vertices position array
    for (int i = 0; i < mesh.vertexCount; i++)
    {
        mesh.vertices[3*i] = vertices[i].x;
        mesh.vertices[3*i + 1] = vertices[i].y;
        mesh.vertices[3*i + 2] = vertices[i].z;
    }

    // Mesh texcoords array
    for (int i = 0; i < mesh.vertexCount; i++)
    {
        mesh.texcoords[2*i] = texcoords[i].x;
        mesh.texcoords[2*i + 1] = texcoords[i].y;
    }

    // Mesh normals array
    for (int i = 0; i < mesh.vertexCount; i++)
    {
        mesh.normals[3*i] = normals[i].x;
        mesh.normals[3*i + 1] = normals[i].y;
        mesh.normals[3*i + 2] = normals[i].z;
    }

    // Mesh indices array initialization
    for (int i = 0; i < mesh.triangleCount*3; i++) mesh.indices[i] = triangles[i];

    RL_FREE(vertices);
    RL_FREE(normals);
    RL_FREE(texcoords);
    RL_FREE(triangles);

#else       // Use par_shapes library to generate plane mesh

    par_shapes_mesh *plane = par_shapes_create_plane(resX, resZ);   // No normals/texcoords generated!!!
    par_shapes_scale(plane, width, length, 1.0f);
    par_shapes_rotate(plane, -PI/2.0f, (float[]){ 1, 0, 0 });
    par_shapes_translate(plane, -width/2, 0.0f, length/2);

    mesh.vertices = (float *)RL_MALLOC(plane->ntriangles*3*3*sizeof(float));
    mesh.texcoords = (float *)RL_MALLOC(plane->ntriangles*3*2*sizeof(float));
    mesh.normals = (float *)RL_MALLOC(plane->ntriangles*3*3*sizeof(float));

    mesh.vertexCount = plane->ntriangles*3;
    mesh.triangleCount = plane->ntriangles;

    for (int k = 0; k < mesh.vertexCount; k++)
    {
        mesh.vertices[k*3] = plane->points[plane->triangles[k]*3];
        mesh.vertices[k*3 + 1] = plane->points[plane->triangles[k]*3 + 1];
        mesh.vertices[k*3 + 2] = plane->points[plane->triangles[k]*3 + 2];

        mesh.normals[k*3] = plane->normals[plane->triangles[k]*3];
        mesh.normals[k*3 + 1] = plane->normals[plane->triangles[k]*3 + 1];
        mesh.normals[k*3 + 2] = plane->normals[plane->triangles[k]*3 + 2];

        mesh.texcoords[k*2] = plane->tcoords[plane->triangles[k]*2];
        mesh.texcoords[k*2 + 1] = plane->tcoords[plane->triangles[k]*2 + 1];
    }

    par_shapes_free_mesh(plane);
#endif

    // Upload vertex data to GPU (static mesh)
    UploadMesh(&mesh, false);

    return mesh;
}

// Generated cuboid mesh
Mesh GenMeshCube(float width, float height, float length)
{
    Mesh mesh = { 0 };

#define CUSTOM_MESH_GEN_CUBE
#if defined(CUSTOM_MESH_GEN_CUBE)
    float vertices[] = {
        -width/2, -height/2, length/2,
        width/2, -height/2, length/2,
        width/2, height/2, length/2,
        -width/2, height/2, length/2,
        -width/2, -height/2, -length/2,
        -width/2, height/2, -length/2,
        width/2, height/2, -length/2,
        width/2, -height/2, -length/2,
        -width/2, height/2, -length/2,
        -width/2, height/2, length/2,
        width/2, height/2, length/2,
        width/2, height/2, -length/2,
        -width/2, -height/2, -length/2,
        width/2, -height/2, -length/2,
        width/2, -height/2, length/2,
        -width/2, -height/2, length/2,
        width/2, -height/2, -length/2,
        width/2, height/2, -length/2,
        width/2, height/2, length/2,
        width/2, -height/2, length/2,
        -width/2, -height/2, -length/2,
        -width/2, -height/2, length/2,
        -width/2, height/2, length/2,
        -width/2, height/2, -length/2
    };

    float texcoords[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    float normals[] = {
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f,-1.0f,
        0.0f, 0.0f,-1.0f,
        0.0f, 0.0f,-1.0f,
        0.0f, 0.0f,-1.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f
    };

    mesh.vertices = (float *)RL_MALLOC(24*3*sizeof(float));
    memcpy(mesh.vertices, vertices, 24*3*sizeof(float));

    mesh.texcoords = (float *)RL_MALLOC(24*2*sizeof(float));
    memcpy(mesh.texcoords, texcoords, 24*2*sizeof(float));

    mesh.normals = (float *)RL_MALLOC(24*3*sizeof(float));
    memcpy(mesh.normals, normals, 24*3*sizeof(float));

    mesh.indices = (unsigned short *)RL_MALLOC(36*sizeof(unsigned short));

    int k = 0;

    // Indices can be initialized right now
    for (int i = 0; i < 36; i+=6)
    {
        mesh.indices[i] = 4*k;
        mesh.indices[i+1] = 4*k+1;
        mesh.indices[i+2] = 4*k+2;
        mesh.indices[i+3] = 4*k;
        mesh.indices[i+4] = 4*k+2;
        mesh.indices[i+5] = 4*k+3;

        k++;
    }

    mesh.vertexCount = 24;
    mesh.triangleCount = 12;

#else               // Use par_shapes library to generate cube mesh
/*
// Platonic solids:
par_shapes_mesh* par_shapes_create_tetrahedron();       // 4 sides polyhedron (pyramid)
par_shapes_mesh* par_shapes_create_cube();              // 6 sides polyhedron (cube)
par_shapes_mesh* par_shapes_create_octahedron();        // 8 sides polyhedron (dyamond)
par_shapes_mesh* par_shapes_create_dodecahedron();      // 12 sides polyhedron
par_shapes_mesh* par_shapes_create_icosahedron();       // 20 sides polyhedron
*/
    // Platonic solid generation: cube (6 sides)
    // NOTE: No normals/texcoords generated by default
    par_shapes_mesh *cube = par_shapes_create_cube();
    cube->tcoords = PAR_MALLOC(float, 2*cube->npoints);
    for (int i = 0; i < 2*cube->npoints; i++) cube->tcoords[i] = 0.0f;
    par_shapes_scale(cube, width, height, length);
    par_shapes_translate(cube, -width/2, 0.0f, -length/2);
    par_shapes_compute_normals(cube);

    mesh.vertices = (float *)RL_MALLOC(cube->ntriangles*3*3*sizeof(float));
    mesh.texcoords = (float *)RL_MALLOC(cube->ntriangles*3*2*sizeof(float));
    mesh.normals = (float *)RL_MALLOC(cube->ntriangles*3*3*sizeof(float));

    mesh.vertexCount = cube->ntriangles*3;
    mesh.triangleCount = cube->ntriangles;

    for (int k = 0; k < mesh.vertexCount; k++)
    {
        mesh.vertices[k*3] = cube->points[cube->triangles[k]*3];
        mesh.vertices[k*3 + 1] = cube->points[cube->triangles[k]*3 + 1];
        mesh.vertices[k*3 + 2] = cube->points[cube->triangles[k]*3 + 2];

        mesh.normals[k*3] = cube->normals[cube->triangles[k]*3];
        mesh.normals[k*3 + 1] = cube->normals[cube->triangles[k]*3 + 1];
        mesh.normals[k*3 + 2] = cube->normals[cube->triangles[k]*3 + 2];

        mesh.texcoords[k*2] = cube->tcoords[cube->triangles[k]*2];
        mesh.texcoords[k*2 + 1] = cube->tcoords[cube->triangles[k]*2 + 1];
    }

    par_shapes_free_mesh(cube);
#endif

    // Upload vertex data to GPU (static mesh)
    UploadMesh(&mesh, false);

    return mesh;
}

// Generate sphere mesh (standard sphere)
Mesh GenMeshSphere(float radius, int rings, int slices)
{
    Mesh mesh = { 0 };

    if ((rings >= 3) && (slices >= 3))
    {
        par_shapes_mesh *sphere = par_shapes_create_parametric_sphere(slices, rings);
        par_shapes_scale(sphere, radius, radius, radius);
        // NOTE: Soft normals are computed internally

        mesh.vertices = (float *)RL_MALLOC(sphere->ntriangles*3*3*sizeof(float));
        mesh.texcoords = (float *)RL_MALLOC(sphere->ntriangles*3*2*sizeof(float));
        mesh.normals = (float *)RL_MALLOC(sphere->ntriangles*3*3*sizeof(float));

        mesh.vertexCount = sphere->ntriangles*3;
        mesh.triangleCount = sphere->ntriangles;

        for (int k = 0; k < mesh.vertexCount; k++)
        {
            mesh.vertices[k*3] = sphere->points[sphere->triangles[k]*3];
            mesh.vertices[k*3 + 1] = sphere->points[sphere->triangles[k]*3 + 1];
            mesh.vertices[k*3 + 2] = sphere->points[sphere->triangles[k]*3 + 2];

            mesh.normals[k*3] = sphere->normals[sphere->triangles[k]*3];
            mesh.normals[k*3 + 1] = sphere->normals[sphere->triangles[k]*3 + 1];
            mesh.normals[k*3 + 2] = sphere->normals[sphere->triangles[k]*3 + 2];

            mesh.texcoords[k*2] = sphere->tcoords[sphere->triangles[k]*2];
            mesh.texcoords[k*2 + 1] = sphere->tcoords[sphere->triangles[k]*2 + 1];
        }

        par_shapes_free_mesh(sphere);

        // Upload vertex data to GPU (static mesh)
        UploadMesh(&mesh, false);
    }
    else TRACELOG(LOG_WARNING, "MESH: Failed to generate mesh: sphere");

    return mesh;
}

// Generate hemi-sphere mesh (half sphere, no bottom cap)
Mesh GenMeshHemiSphere(float radius, int rings, int slices)
{
    Mesh mesh = { 0 };

    if ((rings >= 3) && (slices >= 3))
    {
        if (radius < 0.0f) radius = 0.0f;

        par_shapes_mesh *sphere = par_shapes_create_hemisphere(slices, rings);
        par_shapes_scale(sphere, radius, radius, radius);
        // NOTE: Soft normals are computed internally

        mesh.vertices = (float *)RL_MALLOC(sphere->ntriangles*3*3*sizeof(float));
        mesh.texcoords = (float *)RL_MALLOC(sphere->ntriangles*3*2*sizeof(float));
        mesh.normals = (float *)RL_MALLOC(sphere->ntriangles*3*3*sizeof(float));

        mesh.vertexCount = sphere->ntriangles*3;
        mesh.triangleCount = sphere->ntriangles;

        for (int k = 0; k < mesh.vertexCount; k++)
        {
            mesh.vertices[k*3] = sphere->points[sphere->triangles[k]*3];
            mesh.vertices[k*3 + 1] = sphere->points[sphere->triangles[k]*3 + 1];
            mesh.vertices[k*3 + 2] = sphere->points[sphere->triangles[k]*3 + 2];

            mesh.normals[k*3] = sphere->normals[sphere->triangles[k]*3];
            mesh.normals[k*3 + 1] = sphere->normals[sphere->triangles[k]*3 + 1];
            mesh.normals[k*3 + 2] = sphere->normals[sphere->triangles[k]*3 + 2];

            mesh.texcoords[k*2] = sphere->tcoords[sphere->triangles[k]*2];
            mesh.texcoords[k*2 + 1] = sphere->tcoords[sphere->triangles[k]*2 + 1];
        }

        par_shapes_free_mesh(sphere);

        // Upload vertex data to GPU (static mesh)
        UploadMesh(&mesh, false);
    }
    else TRACELOG(LOG_WARNING, "MESH: Failed to generate mesh: hemisphere");

    return mesh;
}

// Generate cylinder mesh
Mesh GenMeshCylinder(float radius, float height, int slices)
{
    Mesh mesh = { 0 };

    if (slices >= 3)
    {
        // Instance a cylinder that sits on the Z=0 plane using the given tessellation
        // levels across the UV domain.  Think of "slices" like a number of pizza
        // slices, and "stacks" like a number of stacked rings.
        // Height and radius are both 1.0, but they can easily be changed with par_shapes_scale
        par_shapes_mesh *cylinder = par_shapes_create_cylinder(slices, 8);
        par_shapes_scale(cylinder, radius, radius, height);
        par_shapes_rotate(cylinder, -PI/2.0f, (float[]){ 1, 0, 0 });
        par_shapes_rotate(cylinder, PI/2.0f, (float[]){ 0, 1, 0 });

        // Generate an orientable disk shape (top cap)
        par_shapes_mesh *capTop = par_shapes_create_disk(radius, slices, (float[]){ 0, 0, 0 }, (float[]){ 0, 0, 1 });
        capTop->tcoords = PAR_MALLOC(float, 2*capTop->npoints);
        for (int i = 0; i < 2*capTop->npoints; i++) capTop->tcoords[i] = 0.0f;
        par_shapes_rotate(capTop, -PI/2.0f, (float[]){ 1, 0, 0 });
        par_shapes_translate(capTop, 0, height, 0);

        // Generate an orientable disk shape (bottom cap)
        par_shapes_mesh *capBottom = par_shapes_create_disk(radius, slices, (float[]){ 0, 0, 0 }, (float[]){ 0, 0, -1 });
        capBottom->tcoords = PAR_MALLOC(float, 2*capBottom->npoints);
        for (int i = 0; i < 2*capBottom->npoints; i++) capBottom->tcoords[i] = 0.95f;
        par_shapes_rotate(capBottom, PI/2.0f, (float[]){ 1, 0, 0 });

        par_shapes_merge_and_free(cylinder, capTop);
        par_shapes_merge_and_free(cylinder, capBottom);

        mesh.vertices = (float *)RL_MALLOC(cylinder->ntriangles*3*3*sizeof(float));
        mesh.texcoords = (float *)RL_MALLOC(cylinder->ntriangles*3*2*sizeof(float));
        mesh.normals = (float *)RL_MALLOC(cylinder->ntriangles*3*3*sizeof(float));

        mesh.vertexCount = cylinder->ntriangles*3;
        mesh.triangleCount = cylinder->ntriangles;

        for (int k = 0; k < mesh.vertexCount; k++)
        {
            mesh.vertices[k*3] = cylinder->points[cylinder->triangles[k]*3];
            mesh.vertices[k*3 + 1] = cylinder->points[cylinder->triangles[k]*3 + 1];
            mesh.vertices[k*3 + 2] = cylinder->points[cylinder->triangles[k]*3 + 2];

            mesh.normals[k*3] = cylinder->normals[cylinder->triangles[k]*3];
            mesh.normals[k*3 + 1] = cylinder->normals[cylinder->triangles[k]*3 + 1];
            mesh.normals[k*3 + 2] = cylinder->normals[cylinder->triangles[k]*3 + 2];

            mesh.texcoords[k*2] = cylinder->tcoords[cylinder->triangles[k]*2];
            mesh.texcoords[k*2 + 1] = cylinder->tcoords[cylinder->triangles[k]*2 + 1];
        }

        par_shapes_free_mesh(cylinder);

        // Upload vertex data to GPU (static mesh)
        UploadMesh(&mesh, false);
    }
    else TRACELOG(LOG_WARNING, "MESH: Failed to generate mesh: cylinder");

    return mesh;
}

// Generate cone/pyramid mesh
Mesh GenMeshCone(float radius, float height, int slices)
{
    Mesh mesh = { 0 };

    if (slices >= 3)
    {
        // Instance a cone that sits on the Z=0 plane using the given tessellation
        // levels across the UV domain.  Think of "slices" like a number of pizza
        // slices, and "stacks" like a number of stacked rings.
        // Height and radius are both 1.0, but they can easily be changed with par_shapes_scale
        par_shapes_mesh *cone = par_shapes_create_cone(slices, 8);
        par_shapes_scale(cone, radius, radius, height);
        par_shapes_rotate(cone, -PI/2.0f, (float[]){ 1, 0, 0 });
        par_shapes_rotate(cone, PI/2.0f, (float[]){ 0, 1, 0 });

        // Generate an orientable disk shape (bottom cap)
        par_shapes_mesh *capBottom = par_shapes_create_disk(radius, slices, (float[]){ 0, 0, 0 }, (float[]){ 0, 0, -1 });
        capBottom->tcoords = PAR_MALLOC(float, 2*capBottom->npoints);
        for (int i = 0; i < 2*capBottom->npoints; i++) capBottom->tcoords[i] = 0.95f;
        par_shapes_rotate(capBottom, PI/2.0f, (float[]){ 1, 0, 0 });

        par_shapes_merge_and_free(cone, capBottom);

        mesh.vertices = (float *)RL_MALLOC(cone->ntriangles*3*3*sizeof(float));
        mesh.texcoords = (float *)RL_MALLOC(cone->ntriangles*3*2*sizeof(float));
        mesh.normals = (float *)RL_MALLOC(cone->ntriangles*3*3*sizeof(float));

        mesh.vertexCount = cone->ntriangles*3;
        mesh.triangleCount = cone->ntriangles;

        for (int k = 0; k < mesh.vertexCount; k++)
        {
            mesh.vertices[k*3] = cone->points[cone->triangles[k]*3];
            mesh.vertices[k*3 + 1] = cone->points[cone->triangles[k]*3 + 1];
            mesh.vertices[k*3 + 2] = cone->points[cone->triangles[k]*3 + 2];

            mesh.normals[k*3] = cone->normals[cone->triangles[k]*3];
            mesh.normals[k*3 + 1] = cone->normals[cone->triangles[k]*3 + 1];
            mesh.normals[k*3 + 2] = cone->normals[cone->triangles[k]*3 + 2];

            mesh.texcoords[k*2] = cone->tcoords[cone->triangles[k]*2];
            mesh.texcoords[k*2 + 1] = cone->tcoords[cone->triangles[k]*2 + 1];
        }

        par_shapes_free_mesh(cone);

        // Upload vertex data to GPU (static mesh)
        UploadMesh(&mesh, false);
    }
    else TRACELOG(LOG_WARNING, "MESH: Failed to generate mesh: cone");

    return mesh;
}

// Generate torus mesh
Mesh GenMeshTorus(float radius, float size, int radSeg, int sides)
{
    Mesh mesh = { 0 };

    if ((sides >= 3) && (radSeg >= 3))
    {
        if (radius > 1.0f) radius = 1.0f;
        else if (radius < 0.1f) radius = 0.1f;

        // Create a donut that sits on the Z=0 plane with the specified inner radius
        // The outer radius can be controlled with par_shapes_scale
        par_shapes_mesh *torus = par_shapes_create_torus(radSeg, sides, radius);
        par_shapes_scale(torus, size/2, size/2, size/2);

        mesh.vertices = (float *)RL_MALLOC(torus->ntriangles*3*3*sizeof(float));
        mesh.texcoords = (float *)RL_MALLOC(torus->ntriangles*3*2*sizeof(float));
        mesh.normals = (float *)RL_MALLOC(torus->ntriangles*3*3*sizeof(float));

        mesh.vertexCount = torus->ntriangles*3;
        mesh.triangleCount = torus->ntriangles;

        for (int k = 0; k < mesh.vertexCount; k++)
        {
            mesh.vertices[k*3] = torus->points[torus->triangles[k]*3];
            mesh.vertices[k*3 + 1] = torus->points[torus->triangles[k]*3 + 1];
            mesh.vertices[k*3 + 2] = torus->points[torus->triangles[k]*3 + 2];

            mesh.normals[k*3] = torus->normals[torus->triangles[k]*3];
            mesh.normals[k*3 + 1] = torus->normals[torus->triangles[k]*3 + 1];
            mesh.normals[k*3 + 2] = torus->normals[torus->triangles[k]*3 + 2];

            mesh.texcoords[k*2] = torus->tcoords[torus->triangles[k]*2];
            mesh.texcoords[k*2 + 1] = torus->tcoords[torus->triangles[k]*2 + 1];
        }

        par_shapes_free_mesh(torus);

        // Upload vertex data to GPU (static mesh)
        UploadMesh(&mesh, false);
    }
    else TRACELOG(LOG_WARNING, "MESH: Failed to generate mesh: torus");

    return mesh;
}

// Generate trefoil knot mesh
Mesh GenMeshKnot(float radius, float size, int radSeg, int sides)
{
    Mesh mesh = { 0 };

    if ((sides >= 3) && (radSeg >= 3))
    {
        if (radius > 3.0f) radius = 3.0f;
        else if (radius < 0.5f) radius = 0.5f;

        par_shapes_mesh *knot = par_shapes_create_trefoil_knot(radSeg, sides, radius);
        par_shapes_scale(knot, size, size, size);

        mesh.vertices = (float *)RL_MALLOC(knot->ntriangles*3*3*sizeof(float));
        mesh.texcoords = (float *)RL_MALLOC(knot->ntriangles*3*2*sizeof(float));
        mesh.normals = (float *)RL_MALLOC(knot->ntriangles*3*3*sizeof(float));

        mesh.vertexCount = knot->ntriangles*3;
        mesh.triangleCount = knot->ntriangles;

        for (int k = 0; k < mesh.vertexCount; k++)
        {
            mesh.vertices[k*3] = knot->points[knot->triangles[k]*3];
            mesh.vertices[k*3 + 1] = knot->points[knot->triangles[k]*3 + 1];
            mesh.vertices[k*3 + 2] = knot->points[knot->triangles[k]*3 + 2];

            mesh.normals[k*3] = knot->normals[knot->triangles[k]*3];
            mesh.normals[k*3 + 1] = knot->normals[knot->triangles[k]*3 + 1];
            mesh.normals[k*3 + 2] = knot->normals[knot->triangles[k]*3 + 2];

            mesh.texcoords[k*2] = knot->tcoords[knot->triangles[k]*2];
            mesh.texcoords[k*2 + 1] = knot->tcoords[knot->triangles[k]*2 + 1];
        }

        par_shapes_free_mesh(knot);

        // Upload vertex data to GPU (static mesh)
        UploadMesh(&mesh, false);
    }
    else TRACELOG(LOG_WARNING, "MESH: Failed to generate mesh: knot");

    return mesh;
}

// Generate a mesh from heightmap
// NOTE: Vertex data is uploaded to GPU
Mesh GenMeshHeightmap(Image heightmap, Vector3 size)
{
    #define GRAY_VALUE(c) ((c.r+c.g+c.b)/3)

    Mesh mesh = { 0 };

    int mapX = heightmap.width;
    int mapZ = heightmap.height;

    Color *pixels = LoadImageColors(heightmap);

    // NOTE: One vertex per pixel
    mesh.triangleCount = (mapX-1)*(mapZ-1)*2;    // One quad every four pixels

    mesh.vertexCount = mesh.triangleCount*3;

    mesh.vertices = (float *)RL_MALLOC(mesh.vertexCount*3*sizeof(float));
    mesh.normals = (float *)RL_MALLOC(mesh.vertexCount*3*sizeof(float));
    mesh.texcoords = (float *)RL_MALLOC(mesh.vertexCount*2*sizeof(float));
    mesh.colors = NULL;

    int vCounter = 0;       // Used to count vertices float by float
    int tcCounter = 0;      // Used to count texcoords float by float
    int nCounter = 0;       // Used to count normals float by float

    int trisCounter = 0;

    Vector3 scaleFactor = { size.x/mapX, size.y/255.0f, size.z/mapZ };

    Vector3 vA = { 0 };
    Vector3 vB = { 0 };
    Vector3 vC = { 0 };
    Vector3 vN = { 0 };

    for (int z = 0; z < mapZ-1; z++)
    {
        for (int x = 0; x < mapX-1; x++)
        {
            // Fill vertices array with data
            //----------------------------------------------------------

            // one triangle - 3 vertex
            mesh.vertices[vCounter] = (float)x*scaleFactor.x;
            mesh.vertices[vCounter + 1] = (float)GRAY_VALUE(pixels[x + z*mapX])*scaleFactor.y;
            mesh.vertices[vCounter + 2] = (float)z*scaleFactor.z;

            mesh.vertices[vCounter + 3] = (float)x*scaleFactor.x;
            mesh.vertices[vCounter + 4] = (float)GRAY_VALUE(pixels[x + (z + 1)*mapX])*scaleFactor.y;
            mesh.vertices[vCounter + 5] = (float)(z + 1)*scaleFactor.z;

            mesh.vertices[vCounter + 6] = (float)(x + 1)*scaleFactor.x;
            mesh.vertices[vCounter + 7] = (float)GRAY_VALUE(pixels[(x + 1) + z*mapX])*scaleFactor.y;
            mesh.vertices[vCounter + 8] = (float)z*scaleFactor.z;

            // another triangle - 3 vertex
            mesh.vertices[vCounter + 9] = mesh.vertices[vCounter + 6];
            mesh.vertices[vCounter + 10] = mesh.vertices[vCounter + 7];
            mesh.vertices[vCounter + 11] = mesh.vertices[vCounter + 8];

            mesh.vertices[vCounter + 12] = mesh.vertices[vCounter + 3];
            mesh.vertices[vCounter + 13] = mesh.vertices[vCounter + 4];
            mesh.vertices[vCounter + 14] = mesh.vertices[vCounter + 5];

            mesh.vertices[vCounter + 15] = (float)(x + 1)*scaleFactor.x;
            mesh.vertices[vCounter + 16] = (float)GRAY_VALUE(pixels[(x + 1) + (z + 1)*mapX])*scaleFactor.y;
            mesh.vertices[vCounter + 17] = (float)(z + 1)*scaleFactor.z;
            vCounter += 18;     // 6 vertex, 18 floats

            // Fill texcoords array with data
            //--------------------------------------------------------------
            mesh.texcoords[tcCounter] = (float)x/(mapX - 1);
            mesh.texcoords[tcCounter + 1] = (float)z/(mapZ - 1);

            mesh.texcoords[tcCounter + 2] = (float)x/(mapX - 1);
            mesh.texcoords[tcCounter + 3] = (float)(z + 1)/(mapZ - 1);

            mesh.texcoords[tcCounter + 4] = (float)(x + 1)/(mapX - 1);
            mesh.texcoords[tcCounter + 5] = (float)z/(mapZ - 1);

            mesh.texcoords[tcCounter + 6] = mesh.texcoords[tcCounter + 4];
            mesh.texcoords[tcCounter + 7] = mesh.texcoords[tcCounter + 5];

            mesh.texcoords[tcCounter + 8] = mesh.texcoords[tcCounter + 2];
            mesh.texcoords[tcCounter + 9] = mesh.texcoords[tcCounter + 3];

            mesh.texcoords[tcCounter + 10] = (float)(x + 1)/(mapX - 1);
            mesh.texcoords[tcCounter + 11] = (float)(z + 1)/(mapZ - 1);
            tcCounter += 12;    // 6 texcoords, 12 floats

            // Fill normals array with data
            //--------------------------------------------------------------
            for (int i = 0; i < 18; i += 9)
            {
                vA.x = mesh.vertices[nCounter + i];
                vA.y = mesh.vertices[nCounter + i + 1];
                vA.z = mesh.vertices[nCounter + i + 2];

                vB.x = mesh.vertices[nCounter + i + 3];
                vB.y = mesh.vertices[nCounter + i + 4];
                vB.z = mesh.vertices[nCounter + i + 5];

                vC.x = mesh.vertices[nCounter + i + 6];
                vC.y = mesh.vertices[nCounter + i + 7];
                vC.z = mesh.vertices[nCounter + i + 8];

                vN = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(vB, vA), Vector3Subtract(vC, vA)));

                mesh.normals[nCounter + i] = vN.x;
                mesh.normals[nCounter + i + 1] = vN.y;
                mesh.normals[nCounter + i + 2] = vN.z;

                mesh.normals[nCounter + i + 3] = vN.x;
                mesh.normals[nCounter + i + 4] = vN.y;
                mesh.normals[nCounter + i + 5] = vN.z;

                mesh.normals[nCounter + i + 6] = vN.x;
                mesh.normals[nCounter + i + 7] = vN.y;
                mesh.normals[nCounter + i + 8] = vN.z;
            }

            nCounter += 18;     // 6 vertex, 18 floats
            trisCounter += 2;
        }
    }

    UnloadImageColors(pixels);  // Unload pixels color data

    // Upload vertex data to GPU (static mesh)
    UploadMesh(&mesh, false);

    return mesh;
}

// Generate a cubes mesh from pixel data
// NOTE: Vertex data is uploaded to GPU
Mesh GenMeshCubicmap(Image cubicmap, Vector3 cubeSize)
{
    #define COLOR_EQUAL(col1, col2) ((col1.r == col2.r)&&(col1.g == col2.g)&&(col1.b == col2.b)&&(col1.a == col2.a))

    Mesh mesh = { 0 };

    Color *pixels = LoadImageColors(cubicmap);

    int mapWidth = cubicmap.width;
    int mapHeight = cubicmap.height;

    // NOTE: Max possible number of triangles numCubes*(12 triangles by cube)
    int maxTriangles = cubicmap.width*cubicmap.height*12;

    int vCounter = 0;       // Used to count vertices
    int tcCounter = 0;      // Used to count texcoords
    int nCounter = 0;       // Used to count normals

    float w = cubeSize.x;
    float h = cubeSize.z;
    float h2 = cubeSize.y;

    Vector3 *mapVertices = (Vector3 *)RL_MALLOC(maxTriangles*3*sizeof(Vector3));
    Vector2 *mapTexcoords = (Vector2 *)RL_MALLOC(maxTriangles*3*sizeof(Vector2));
    Vector3 *mapNormals = (Vector3 *)RL_MALLOC(maxTriangles*3*sizeof(Vector3));

    // Define the 6 normals of the cube, we will combine them accordingly later...
    Vector3 n1 = { 1.0f, 0.0f, 0.0f };
    Vector3 n2 = { -1.0f, 0.0f, 0.0f };
    Vector3 n3 = { 0.0f, 1.0f, 0.0f };
    Vector3 n4 = { 0.0f, -1.0f, 0.0f };
    Vector3 n5 = { 0.0f, 0.0f, -1.0f };
    Vector3 n6 = { 0.0f, 0.0f, 1.0f };

    // NOTE: We use texture rectangles to define different textures for top-bottom-front-back-right-left (6)
    typedef struct RectangleF {
        float x;
        float y;
        float width;
        float height;
    } RectangleF;

    RectangleF rightTexUV = { 0.0f, 0.0f, 0.5f, 0.5f };
    RectangleF leftTexUV = { 0.5f, 0.0f, 0.5f, 0.5f };
    RectangleF frontTexUV = { 0.0f, 0.0f, 0.5f, 0.5f };
    RectangleF backTexUV = { 0.5f, 0.0f, 0.5f, 0.5f };
    RectangleF topTexUV = { 0.0f, 0.5f, 0.5f, 0.5f };
    RectangleF bottomTexUV = { 0.5f, 0.5f, 0.5f, 0.5f };

    for (int z = 0; z < mapHeight; ++z)
    {
        for (int x = 0; x < mapWidth; ++x)
        {
            // Define the 8 vertex of the cube, we will combine them accordingly later...
            Vector3 v1 = { w*(x - 0.5f), h2, h*(z - 0.5f) };
            Vector3 v2 = { w*(x - 0.5f), h2, h*(z + 0.5f) };
            Vector3 v3 = { w*(x + 0.5f), h2, h*(z + 0.5f) };
            Vector3 v4 = { w*(x + 0.5f), h2, h*(z - 0.5f) };
            Vector3 v5 = { w*(x + 0.5f), 0, h*(z - 0.5f) };
            Vector3 v6 = { w*(x - 0.5f), 0, h*(z - 0.5f) };
            Vector3 v7 = { w*(x - 0.5f), 0, h*(z + 0.5f) };
            Vector3 v8 = { w*(x + 0.5f), 0, h*(z + 0.5f) };

            // We check pixel color to be WHITE -> draw full cube
            if (COLOR_EQUAL(pixels[z*cubicmap.width + x], WHITE))
            {
                // Define triangles and checking collateral cubes
                //------------------------------------------------

                // Define top triangles (2 tris, 6 vertex --> v1-v2-v3, v1-v3-v4)
                // WARNING: Not required for a WHITE cubes, created to allow seeing the map from outside
                mapVertices[vCounter] = v1;
                mapVertices[vCounter + 1] = v2;
                mapVertices[vCounter + 2] = v3;
                mapVertices[vCounter + 3] = v1;
                mapVertices[vCounter + 4] = v3;
                mapVertices[vCounter + 5] = v4;
                vCounter += 6;

                mapNormals[nCounter] = n3;
                mapNormals[nCounter + 1] = n3;
                mapNormals[nCounter + 2] = n3;
                mapNormals[nCounter + 3] = n3;
                mapNormals[nCounter + 4] = n3;
                mapNormals[nCounter + 5] = n3;
                nCounter += 6;

                mapTexcoords[tcCounter] = (Vector2){ topTexUV.x, topTexUV.y };
                mapTexcoords[tcCounter + 1] = (Vector2){ topTexUV.x, topTexUV.y + topTexUV.height };
                mapTexcoords[tcCounter + 2] = (Vector2){ topTexUV.x + topTexUV.width, topTexUV.y + topTexUV.height };
                mapTexcoords[tcCounter + 3] = (Vector2){ topTexUV.x, topTexUV.y };
                mapTexcoords[tcCounter + 4] = (Vector2){ topTexUV.x + topTexUV.width, topTexUV.y + topTexUV.height };
                mapTexcoords[tcCounter + 5] = (Vector2){ topTexUV.x + topTexUV.width, topTexUV.y };
                tcCounter += 6;

                // Define bottom triangles (2 tris, 6 vertex --> v6-v8-v7, v6-v5-v8)
                mapVertices[vCounter] = v6;
                mapVertices[vCounter + 1] = v8;
                mapVertices[vCounter + 2] = v7;
                mapVertices[vCounter + 3] = v6;
                mapVertices[vCounter + 4] = v5;
                mapVertices[vCounter + 5] = v8;
                vCounter += 6;

                mapNormals[nCounter] = n4;
                mapNormals[nCounter + 1] = n4;
                mapNormals[nCounter + 2] = n4;
                mapNormals[nCounter + 3] = n4;
                mapNormals[nCounter + 4] = n4;
                mapNormals[nCounter + 5] = n4;
                nCounter += 6;

                mapTexcoords[tcCounter] = (Vector2){ bottomTexUV.x + bottomTexUV.width, bottomTexUV.y };
                mapTexcoords[tcCounter + 1] = (Vector2){ bottomTexUV.x, bottomTexUV.y + bottomTexUV.height };
                mapTexcoords[tcCounter + 2] = (Vector2){ bottomTexUV.x + bottomTexUV.width, bottomTexUV.y + bottomTexUV.height };
                mapTexcoords[tcCounter + 3] = (Vector2){ bottomTexUV.x + bottomTexUV.width, bottomTexUV.y };
                mapTexcoords[tcCounter + 4] = (Vector2){ bottomTexUV.x, bottomTexUV.y };
                mapTexcoords[tcCounter + 5] = (Vector2){ bottomTexUV.x, bottomTexUV.y + bottomTexUV.height };
                tcCounter += 6;

                // Checking cube on bottom of current cube
                if (((z < cubicmap.height - 1) && COLOR_EQUAL(pixels[(z + 1)*cubicmap.width + x], BLACK)) || (z == cubicmap.height - 1))
                {
                    // Define front triangles (2 tris, 6 vertex) --> v2 v7 v3, v3 v7 v8
                    // NOTE: Collateral occluded faces are not generated
                    mapVertices[vCounter] = v2;
                    mapVertices[vCounter + 1] = v7;
                    mapVertices[vCounter + 2] = v3;
                    mapVertices[vCounter + 3] = v3;
                    mapVertices[vCounter + 4] = v7;
                    mapVertices[vCounter + 5] = v8;
                    vCounter += 6;

                    mapNormals[nCounter] = n6;
                    mapNormals[nCounter + 1] = n6;
                    mapNormals[nCounter + 2] = n6;
                    mapNormals[nCounter + 3] = n6;
                    mapNormals[nCounter + 4] = n6;
                    mapNormals[nCounter + 5] = n6;
                    nCounter += 6;

                    mapTexcoords[tcCounter] = (Vector2){ frontTexUV.x, frontTexUV.y };
                    mapTexcoords[tcCounter + 1] = (Vector2){ frontTexUV.x, frontTexUV.y + frontTexUV.height };
                    mapTexcoords[tcCounter + 2] = (Vector2){ frontTexUV.x + frontTexUV.width, frontTexUV.y };
                    mapTexcoords[tcCounter + 3] = (Vector2){ frontTexUV.x + frontTexUV.width, frontTexUV.y };
                    mapTexcoords[tcCounter + 4] = (Vector2){ frontTexUV.x, frontTexUV.y + frontTexUV.height };
                    mapTexcoords[tcCounter + 5] = (Vector2){ frontTexUV.x + frontTexUV.width, frontTexUV.y + frontTexUV.height };
                    tcCounter += 6;
                }

                // Checking cube on top of current cube
                if (((z > 0) && COLOR_EQUAL(pixels[(z - 1)*cubicmap.width + x], BLACK)) || (z == 0))
                {
                    // Define back triangles (2 tris, 6 vertex) --> v1 v5 v6, v1 v4 v5
                    // NOTE: Collateral occluded faces are not generated
                    mapVertices[vCounter] = v1;
                    mapVertices[vCounter + 1] = v5;
                    mapVertices[vCounter + 2] = v6;
                    mapVertices[vCounter + 3] = v1;
                    mapVertices[vCounter + 4] = v4;
                    mapVertices[vCounter + 5] = v5;
                    vCounter += 6;

                    mapNormals[nCounter] = n5;
                    mapNormals[nCounter + 1] = n5;
                    mapNormals[nCounter + 2] = n5;
                    mapNormals[nCounter + 3] = n5;
                    mapNormals[nCounter + 4] = n5;
                    mapNormals[nCounter + 5] = n5;
                    nCounter += 6;

                    mapTexcoords[tcCounter] = (Vector2){ backTexUV.x + backTexUV.width, backTexUV.y };
                    mapTexcoords[tcCounter + 1] = (Vector2){ backTexUV.x, backTexUV.y + backTexUV.height };
                    mapTexcoords[tcCounter + 2] = (Vector2){ backTexUV.x + backTexUV.width, backTexUV.y + backTexUV.height };
                    mapTexcoords[tcCounter + 3] = (Vector2){ backTexUV.x + backTexUV.width, backTexUV.y };
                    mapTexcoords[tcCounter + 4] = (Vector2){ backTexUV.x, backTexUV.y };
                    mapTexcoords[tcCounter + 5] = (Vector2){ backTexUV.x, backTexUV.y + backTexUV.height };
                    tcCounter += 6;
                }

                // Checking cube on right of current cube
                if (((x < cubicmap.width - 1) && COLOR_EQUAL(pixels[z*cubicmap.width + (x + 1)], BLACK)) || (x == cubicmap.width - 1))
                {
                    // Define right triangles (2 tris, 6 vertex) --> v3 v8 v4, v4 v8 v5
                    // NOTE: Collateral occluded faces are not generated
                    mapVertices[vCounter] = v3;
                    mapVertices[vCounter + 1] = v8;
                    mapVertices[vCounter + 2] = v4;
                    mapVertices[vCounter + 3] = v4;
                    mapVertices[vCounter + 4] = v8;
                    mapVertices[vCounter + 5] = v5;
                    vCounter += 6;

                    mapNormals[nCounter] = n1;
                    mapNormals[nCounter + 1] = n1;
                    mapNormals[nCounter + 2] = n1;
                    mapNormals[nCounter + 3] = n1;
                    mapNormals[nCounter + 4] = n1;
                    mapNormals[nCounter + 5] = n1;
                    nCounter += 6;

                    mapTexcoords[tcCounter] = (Vector2){ rightTexUV.x, rightTexUV.y };
                    mapTexcoords[tcCounter + 1] = (Vector2){ rightTexUV.x, rightTexUV.y + rightTexUV.height };
                    mapTexcoords[tcCounter + 2] = (Vector2){ rightTexUV.x + rightTexUV.width, rightTexUV.y };
                    mapTexcoords[tcCounter + 3] = (Vector2){ rightTexUV.x + rightTexUV.width, rightTexUV.y };
                    mapTexcoords[tcCounter + 4] = (Vector2){ rightTexUV.x, rightTexUV.y + rightTexUV.height };
                    mapTexcoords[tcCounter + 5] = (Vector2){ rightTexUV.x + rightTexUV.width, rightTexUV.y + rightTexUV.height };
                    tcCounter += 6;
                }

                // Checking cube on left of current cube
                if (((x > 0) && COLOR_EQUAL(pixels[z*cubicmap.width + (x - 1)], BLACK)) || (x == 0))
                {
                    // Define left triangles (2 tris, 6 vertex) --> v1 v7 v2, v1 v6 v7
                    // NOTE: Collateral occluded faces are not generated
                    mapVertices[vCounter] = v1;
                    mapVertices[vCounter + 1] = v7;
                    mapVertices[vCounter + 2] = v2;
                    mapVertices[vCounter + 3] = v1;
                    mapVertices[vCounter + 4] = v6;
                    mapVertices[vCounter + 5] = v7;
                    vCounter += 6;

                    mapNormals[nCounter] = n2;
                    mapNormals[nCounter + 1] = n2;
                    mapNormals[nCounter + 2] = n2;
                    mapNormals[nCounter + 3] = n2;
                    mapNormals[nCounter + 4] = n2;
                    mapNormals[nCounter + 5] = n2;
                    nCounter += 6;

                    mapTexcoords[tcCounter] = (Vector2){ leftTexUV.x, leftTexUV.y };
                    mapTexcoords[tcCounter + 1] = (Vector2){ leftTexUV.x + leftTexUV.width, leftTexUV.y + leftTexUV.height };
                    mapTexcoords[tcCounter + 2] = (Vector2){ leftTexUV.x + leftTexUV.width, leftTexUV.y };
                    mapTexcoords[tcCounter + 3] = (Vector2){ leftTexUV.x, leftTexUV.y };
                    mapTexcoords[tcCounter + 4] = (Vector2){ leftTexUV.x, leftTexUV.y + leftTexUV.height };
                    mapTexcoords[tcCounter + 5] = (Vector2){ leftTexUV.x + leftTexUV.width, leftTexUV.y + leftTexUV.height };
                    tcCounter += 6;
                }
            }
            // We check pixel color to be BLACK, we will only draw floor and roof
            else if (COLOR_EQUAL(pixels[z*cubicmap.width + x], BLACK))
            {
                // Define top triangles (2 tris, 6 vertex --> v1-v2-v3, v1-v3-v4)
                mapVertices[vCounter] = v1;
                mapVertices[vCounter + 1] = v3;
                mapVertices[vCounter + 2] = v2;
                mapVertices[vCounter + 3] = v1;
                mapVertices[vCounter + 4] = v4;
                mapVertices[vCounter + 5] = v3;
                vCounter += 6;

                mapNormals[nCounter] = n4;
                mapNormals[nCounter + 1] = n4;
                mapNormals[nCounter + 2] = n4;
                mapNormals[nCounter + 3] = n4;
                mapNormals[nCounter + 4] = n4;
                mapNormals[nCounter + 5] = n4;
                nCounter += 6;

                mapTexcoords[tcCounter] = (Vector2){ topTexUV.x, topTexUV.y };
                mapTexcoords[tcCounter + 1] = (Vector2){ topTexUV.x + topTexUV.width, topTexUV.y + topTexUV.height };
                mapTexcoords[tcCounter + 2] = (Vector2){ topTexUV.x, topTexUV.y + topTexUV.height };
                mapTexcoords[tcCounter + 3] = (Vector2){ topTexUV.x, topTexUV.y };
                mapTexcoords[tcCounter + 4] = (Vector2){ topTexUV.x + topTexUV.width, topTexUV.y };
                mapTexcoords[tcCounter + 5] = (Vector2){ topTexUV.x + topTexUV.width, topTexUV.y + topTexUV.height };
                tcCounter += 6;

                // Define bottom triangles (2 tris, 6 vertex --> v6-v8-v7, v6-v5-v8)
                mapVertices[vCounter] = v6;
                mapVertices[vCounter + 1] = v7;
                mapVertices[vCounter + 2] = v8;
                mapVertices[vCounter + 3] = v6;
                mapVertices[vCounter + 4] = v8;
                mapVertices[vCounter + 5] = v5;
                vCounter += 6;

                mapNormals[nCounter] = n3;
                mapNormals[nCounter + 1] = n3;
                mapNormals[nCounter + 2] = n3;
                mapNormals[nCounter + 3] = n3;
                mapNormals[nCounter + 4] = n3;
                mapNormals[nCounter + 5] = n3;
                nCounter += 6;

                mapTexcoords[tcCounter] = (Vector2){ bottomTexUV.x + bottomTexUV.width, bottomTexUV.y };
                mapTexcoords[tcCounter + 1] = (Vector2){ bottomTexUV.x + bottomTexUV.width, bottomTexUV.y + bottomTexUV.height };
                mapTexcoords[tcCounter + 2] = (Vector2){ bottomTexUV.x, bottomTexUV.y + bottomTexUV.height };
                mapTexcoords[tcCounter + 3] = (Vector2){ bottomTexUV.x + bottomTexUV.width, bottomTexUV.y };
                mapTexcoords[tcCounter + 4] = (Vector2){ bottomTexUV.x, bottomTexUV.y + bottomTexUV.height };
                mapTexcoords[tcCounter + 5] = (Vector2){ bottomTexUV.x, bottomTexUV.y };
                tcCounter += 6;
            }
        }
    }

    // Move data from mapVertices temp arays to vertices float array
    mesh.vertexCount = vCounter;
    mesh.triangleCount = vCounter/3;

    mesh.vertices = (float *)RL_MALLOC(mesh.vertexCount*3*sizeof(float));
    mesh.normals = (float *)RL_MALLOC(mesh.vertexCount*3*sizeof(float));
    mesh.texcoords = (float *)RL_MALLOC(mesh.vertexCount*2*sizeof(float));
    mesh.colors = NULL;

    int fCounter = 0;

    // Move vertices data
    for (int i = 0; i < vCounter; i++)
    {
        mesh.vertices[fCounter] = mapVertices[i].x;
        mesh.vertices[fCounter + 1] = mapVertices[i].y;
        mesh.vertices[fCounter + 2] = mapVertices[i].z;
        fCounter += 3;
    }

    fCounter = 0;

    // Move normals data
    for (int i = 0; i < nCounter; i++)
    {
        mesh.normals[fCounter] = mapNormals[i].x;
        mesh.normals[fCounter + 1] = mapNormals[i].y;
        mesh.normals[fCounter + 2] = mapNormals[i].z;
        fCounter += 3;
    }

    fCounter = 0;

    // Move texcoords data
    for (int i = 0; i < tcCounter; i++)
    {
        mesh.texcoords[fCounter] = mapTexcoords[i].x;
        mesh.texcoords[fCounter + 1] = mapTexcoords[i].y;
        fCounter += 2;
    }

    RL_FREE(mapVertices);
    RL_FREE(mapNormals);
    RL_FREE(mapTexcoords);

    UnloadImageColors(pixels);   // Unload pixels color data

    // Upload vertex data to GPU (static mesh)
    UploadMesh(&mesh, false);

    return mesh;
}
#endif      // SUPPORT_MESH_GENERATION

// Compute mesh tangents
// NOTE: To calculate mesh tangents and binormals we need mesh vertex positions and texture coordinates
// Implementation base don: https://answers.unity.com/questions/7789/calculating-tangents-vector4.html
void GenMeshTangents(Mesh *mesh)
{
    if (mesh->tangents == NULL) mesh->tangents = (float *)RL_MALLOC(mesh->vertexCount*4*sizeof(float));
    else
    {
        RL_FREE(mesh->tangents);
        mesh->tangents = (float *)RL_MALLOC(mesh->vertexCount*4*sizeof(float));
    }

    Vector3 *tan1 = (Vector3 *)RL_MALLOC(mesh->vertexCount*sizeof(Vector3));
    Vector3 *tan2 = (Vector3 *)RL_MALLOC(mesh->vertexCount*sizeof(Vector3));

    for (int i = 0; i < mesh->vertexCount; i += 3)
    {
        // Get triangle vertices
        Vector3 v1 = { mesh->vertices[(i + 0)*3 + 0], mesh->vertices[(i + 0)*3 + 1], mesh->vertices[(i + 0)*3 + 2] };
        Vector3 v2 = { mesh->vertices[(i + 1)*3 + 0], mesh->vertices[(i + 1)*3 + 1], mesh->vertices[(i + 1)*3 + 2] };
        Vector3 v3 = { mesh->vertices[(i + 2)*3 + 0], mesh->vertices[(i + 2)*3 + 1], mesh->vertices[(i + 2)*3 + 2] };

        // Get triangle texcoords
        Vector2 uv1 = { mesh->texcoords[(i + 0)*2 + 0], mesh->texcoords[(i + 0)*2 + 1] };
        Vector2 uv2 = { mesh->texcoords[(i + 1)*2 + 0], mesh->texcoords[(i + 1)*2 + 1] };
        Vector2 uv3 = { mesh->texcoords[(i + 2)*2 + 0], mesh->texcoords[(i + 2)*2 + 1] };

        float x1 = v2.x - v1.x;
        float y1 = v2.y - v1.y;
        float z1 = v2.z - v1.z;
        float x2 = v3.x - v1.x;
        float y2 = v3.y - v1.y;
        float z2 = v3.z - v1.z;

        float s1 = uv2.x - uv1.x;
        float t1 = uv2.y - uv1.y;
        float s2 = uv3.x - uv1.x;
        float t2 = uv3.y - uv1.y;

        float div = s1*t2 - s2*t1;
        float r = (div == 0.0f)? 0.0f : 1.0f/div;

        Vector3 sdir = { (t2*x1 - t1*x2)*r, (t2*y1 - t1*y2)*r, (t2*z1 - t1*z2)*r };
        Vector3 tdir = { (s1*x2 - s2*x1)*r, (s1*y2 - s2*y1)*r, (s1*z2 - s2*z1)*r };

        tan1[i + 0] = sdir;
        tan1[i + 1] = sdir;
        tan1[i + 2] = sdir;

        tan2[i + 0] = tdir;
        tan2[i + 1] = tdir;
        tan2[i + 2] = tdir;
    }

    // Compute tangents considering normals
    for (int i = 0; i < mesh->vertexCount; i++)
    {
        Vector3 normal = { mesh->normals[i*3 + 0], mesh->normals[i*3 + 1], mesh->normals[i*3 + 2] };
        Vector3 tangent = tan1[i];

        // TODO: Review, not sure if tangent computation is right, just used reference proposed maths...
#if defined(COMPUTE_TANGENTS_METHOD_01)
        Vector3 tmp = Vector3Subtract(tangent, Vector3Scale(normal, Vector3DotProduct(normal, tangent)));
        tmp = Vector3Normalize(tmp);
        mesh->tangents[i*4 + 0] = tmp.x;
        mesh->tangents[i*4 + 1] = tmp.y;
        mesh->tangents[i*4 + 2] = tmp.z;
        mesh->tangents[i*4 + 3] = 1.0f;
#else
        Vector3OrthoNormalize(&normal, &tangent);
        mesh->tangents[i*4 + 0] = tangent.x;
        mesh->tangents[i*4 + 1] = tangent.y;
        mesh->tangents[i*4 + 2] = tangent.z;
        mesh->tangents[i*4 + 3] = (Vector3DotProduct(Vector3CrossProduct(normal, tangent), tan2[i]) < 0.0f)? -1.0f : 1.0f;
#endif
    }

    RL_FREE(tan1);
    RL_FREE(tan2);

    if (mesh->vboId != NULL)
    {
        if (mesh->vboId[SHADER_LOC_VERTEX_TANGENT] != 0)
        {
            // Upate existing vertex buffer
            rlUpdateVertexBuffer(mesh->vboId[SHADER_LOC_VERTEX_TANGENT], mesh->tangents, mesh->vertexCount*4*sizeof(float), 0);
        }
        else
        {
            // Load a new tangent attributes buffer
            mesh->vboId[SHADER_LOC_VERTEX_TANGENT] = rlLoadVertexBuffer(mesh->tangents, mesh->vertexCount*4*sizeof(float), false);
        }

        rlEnableVertexArray(mesh->vaoId);
        rlSetVertexAttribute(4, 4, RL_FLOAT, 0, 0, 0);
        rlEnableVertexAttribute(4);
        rlDisableVertexArray();
    }

    TRACELOG(LOG_INFO, "MESH: Tangents data computed and uploaded for provided mesh");
}

// Compute mesh binormals (aka bitangent)
void GenMeshBinormals(Mesh *mesh)
{
    for (int i = 0; i < mesh->vertexCount; i++)
    {
        //Vector3 normal = { mesh->normals[i*3 + 0], mesh->normals[i*3 + 1], mesh->normals[i*3 + 2] };
        //Vector3 tangent = { mesh->tangents[i*4 + 0], mesh->tangents[i*4 + 1], mesh->tangents[i*4 + 2] };
        //Vector3 binormal = Vector3Scale(Vector3CrossProduct(normal, tangent), mesh->tangents[i*4 + 3]);

        // TODO: Register computed binormal in mesh->binormal?
    }
}

// Draw a model (with texture if set)
void DrawModel(Model model, Vector3 position, float scale, Color tint)
{
    Vector3 vScale = { scale, scale, scale };
    Vector3 rotationAxis = { 0.0f, 1.0f, 0.0f };
    DrawModelEx(model, position, rotationAxis, 0.0f, vScale, tint);
}

// Draw a model with extended parameters
void DrawModelEx(Model model, Vector3 position, Vector3 rotationAxis, float rotationAngle, Vector3 scale, Color tint)
{
    // Calculate transformation matrix from function parameters
    Matrix matScale = MatrixScale(scale.x, scale.y, scale.z);
    Matrix matRotation = MatrixRotate(rotationAxis, rotationAngle*DEG2RAD);
    Matrix matTranslation = MatrixTranslate(position.x, position.y, position.z);
    Matrix matTransform = MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);
    // Combine model transformation matrix (model.transform) with matrix generated by function parameters (matTransform)
    model.transform = MatrixMultiply(model.transform, matTransform);
    for (int i = 0; i < model.meshCount; i++) {
        Color color = model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color;
        Color colorTint = WHITE;
        colorTint.r = (unsigned char)((((float)color.r/255.0)*((float)tint.r/255.0))*255.0f);
        colorTint.g = (unsigned char)((((float)color.g/255.0)*((float)tint.g/255.0))*255.0f);
        colorTint.b = (unsigned char)((((float)color.b/255.0)*((float)tint.b/255.0))*255.0f);
        colorTint.a = (unsigned char)((((float)color.a/255.0)*((float)tint.a/255.0))*255.0f);
        model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color = colorTint;
        DrawMesh(model.meshes[i], model.materials[model.meshMaterial[i]], model.transform);
        model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color = color;
    }
}

// Draw a model wires (with texture if set)
void DrawModelWires(Model model, Vector3 position, float scale, Color tint)
{
    rlEnableWireMode();
    DrawModel(model, position, scale, tint);
    rlDisableWireMode();
}

// Draw a model wires (with texture if set) with extended parameters
void DrawModelWiresEx(Model model, Vector3 position, Vector3 rotationAxis, float rotationAngle, Vector3 scale, Color tint)
{
    rlEnableWireMode();
    DrawModelEx(model, position, rotationAxis, rotationAngle, scale, tint);
    rlDisableWireMode();
}

// Load OBJ mesh data
// Keep the following information in mind when reading this
//  - A mesh is created for every material present in the obj file
//  - the model.meshCount is therefore the materialCount returned from tinyobj
//  - the mesh is automatically triangulated by tinyobj
static Model LoadOBJ(const char *fileName)
{
    Model model = { 0 };
    tinyobj_attrib_t attrib = { 0 };
    tinyobj_shape_t *meshes = NULL;
    unsigned int meshCount = 0;
    tinyobj_material_t *materials = NULL;
    unsigned int materialCount = 0;
    char *fileText = LoadFileText(fileName);
    if (fileText != NULL) {
        unsigned int dataSize = (unsigned int)strlen(fileText);
        char currentDir[1024] = { 0 };
        strcpy(currentDir, GetWorkingDirectory());
        const char *workingDir = GetDirectoryPath(fileName);
        if (CHDIR(workingDir) != 0)
            TRACELOG(LOG_WARNING, "MODEL: [%s] Failed to change working directory", workingDir);
        unsigned int flags = TINYOBJ_FLAG_TRIANGULATE;
        int ret = tinyobj_parse_obj(&attrib, &meshes, &meshCount, &materials, &materialCount, fileText, dataSize, flags);
        if (ret != TINYOBJ_SUCCESS) TRACELOG(LOG_WARNING, "MODEL: [%s] Failed to load OBJ data", fileName);
        else TRACELOG(LOG_INFO, "MODEL: [%s] OBJ data loaded successfully: %i meshes/%i materials", fileName, meshCount, materialCount);
        model.meshCount = materialCount;
        // Init model materials array
        if (materialCount > 0) {
            model.materialCount = materialCount;
            model.materials = (Material *)RL_CALLOC(model.materialCount, sizeof(Material));
            TraceLog(LOG_INFO, "MODEL: model has %i material meshes", materialCount);
        } else {
            model.meshCount = 1;
            TraceLog(LOG_INFO, "MODEL: No materials, putting all meshes in a default material");
        }
        model.meshes = (Mesh *)RL_CALLOC(model.meshCount, sizeof(Mesh));
        model.meshMaterial = (int *)RL_CALLOC(model.meshCount, sizeof(int));
        // Count the faces for each material
        int *matFaces = RL_CALLOC(model.meshCount, sizeof(int));
        // iff no materials are present use all faces on one mesh
        if (materialCount > 0) {
            for (unsigned int fi = 0; fi < attrib.num_faces; fi++) {
                //tinyobj_vertex_index_t face = attrib.faces[fi];
                int idx = attrib.material_ids[fi];
                matFaces[idx]++;
            }
        } else {
            matFaces[0] = attrib.num_faces;
        }

        /* Create the material meshes */
        // Running counts/indexes for each material mesh as we are
        // building them at the same time
        int *vCount = RL_CALLOC(model.meshCount, sizeof(int));
        int *vtCount = RL_CALLOC(model.meshCount, sizeof(int));
        int *vnCount = RL_CALLOC(model.meshCount, sizeof(int));
        int *faceCount = RL_CALLOC(model.meshCount, sizeof(int));
        // Allocate space for each of the material meshes
        for (int mi = 0; mi < model.meshCount; mi++) {
            model.meshes[mi].vertexCount = matFaces[mi]*3;
            model.meshes[mi].triangleCount = matFaces[mi];
            model.meshes[mi].vertices = (float *)RL_CALLOC(model.meshes[mi].vertexCount*3, sizeof(float));
            model.meshes[mi].texcoords = (float *)RL_CALLOC(model.meshes[mi].vertexCount*2, sizeof(float));
            model.meshes[mi].normals = (float *)RL_CALLOC(model.meshes[mi].vertexCount*3, sizeof(float));
            model.meshMaterial[mi] = mi;
        }
        // Scan through the combined sub meshes and pick out each material mesh
        for (unsigned int af = 0; af < attrib.num_faces; af++) {
            int mm = attrib.material_ids[af];   // mesh material for this face
            if (mm == -1) { mm = 0; }           // no material object..
            // Get indices for the face
            tinyobj_vertex_index_t idx0 = attrib.faces[3*af + 0];
            tinyobj_vertex_index_t idx1 = attrib.faces[3*af + 1];
            tinyobj_vertex_index_t idx2 = attrib.faces[3*af + 2];
            // Fill vertices buffer (float) using vertex index of the face
            for (int v = 0; v < 3; v++) { model.meshes[mm].vertices[vCount[mm] + v] = attrib.vertices[idx0.v_idx*3 + v]; } vCount[mm] +=3;
            for (int v = 0; v < 3; v++) { model.meshes[mm].vertices[vCount[mm] + v] = attrib.vertices[idx1.v_idx*3 + v]; } vCount[mm] +=3;
            for (int v = 0; v < 3; v++) { model.meshes[mm].vertices[vCount[mm] + v] = attrib.vertices[idx2.v_idx*3 + v]; } vCount[mm] +=3;
            if (attrib.num_texcoords > 0) {
                // Fill texcoords buffer (float) using vertex index of the face
                // NOTE: Y-coordinate must be flipped upside-down to account for
                // raylib's upside down textures...
                model.meshes[mm].texcoords[vtCount[mm] + 0] = attrib.texcoords[idx0.vt_idx*2 + 0];
                model.meshes[mm].texcoords[vtCount[mm] + 1] = 1.0f - attrib.texcoords[idx0.vt_idx*2 + 1]; vtCount[mm] += 2;
                model.meshes[mm].texcoords[vtCount[mm] + 0] = attrib.texcoords[idx1.vt_idx*2 + 0];
                model.meshes[mm].texcoords[vtCount[mm] + 1] = 1.0f - attrib.texcoords[idx1.vt_idx*2 + 1]; vtCount[mm] += 2;
                model.meshes[mm].texcoords[vtCount[mm] + 0] = attrib.texcoords[idx2.vt_idx*2 + 0];
                model.meshes[mm].texcoords[vtCount[mm] + 1] = 1.0f - attrib.texcoords[idx2.vt_idx*2 + 1]; vtCount[mm] += 2;
            }
            if (attrib.num_normals > 0) {
                // Fill normals buffer (float) using vertex index of the face
                for (int v = 0; v < 3; v++) { model.meshes[mm].normals[vnCount[mm] + v] = attrib.normals[idx0.vn_idx*3 + v]; } vnCount[mm] +=3;
                for (int v = 0; v < 3; v++) { model.meshes[mm].normals[vnCount[mm] + v] = attrib.normals[idx1.vn_idx*3 + v]; } vnCount[mm] +=3;
                for (int v = 0; v < 3; v++) { model.meshes[mm].normals[vnCount[mm] + v] = attrib.normals[idx2.vn_idx*3 + v]; } vnCount[mm] +=3;
            }
        }
        // Init model materials
        for (unsigned int m = 0; m < materialCount; m++) {
            // Init material to default
            // NOTE: Uses default shader, which only supports MATERIAL_MAP_DIFFUSE
            model.materials[m] = LoadMaterialDefault();
            // Get default texture, in case no texture is defined
            // NOTE: rlgl default texture is a 1x1 pixel UNCOMPRESSED_R8G8B8A8
            model.materials[m].maps[MATERIAL_MAP_DIFFUSE].texture = (Texture2D){ rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
            if (materials[m].diffuse_texname != NULL) model.materials[m].maps[MATERIAL_MAP_DIFFUSE].texture = LoadTexture(materials[m].diffuse_texname);  //char *diffuse_texname; // map_Kd
            model.materials[m].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ (unsigned char)(materials[m].diffuse[0]*255.0f), (unsigned char)(materials[m].diffuse[1]*255.0f), (unsigned char)(materials[m].diffuse[2]*255.0f), 255 }; //float diffuse[3];
            model.materials[m].maps[MATERIAL_MAP_DIFFUSE].value = 0.0f;
            if (materials[m].specular_texname != NULL) model.materials[m].maps[MATERIAL_MAP_SPECULAR].texture = LoadTexture(materials[m].specular_texname);  //char *specular_texname; // map_Ks
            model.materials[m].maps[MATERIAL_MAP_SPECULAR].color = (Color){ (unsigned char)(materials[m].specular[0]*255.0f), (unsigned char)(materials[m].specular[1]*255.0f), (unsigned char)(materials[m].specular[2]*255.0f), 255 }; //float specular[3];
            model.materials[m].maps[MATERIAL_MAP_SPECULAR].value = 0.0f;
            if (materials[m].bump_texname != NULL) model.materials[m].maps[MATERIAL_MAP_NORMAL].texture = LoadTexture(materials[m].bump_texname);  //char *bump_texname; // map_bump, bump
            model.materials[m].maps[MATERIAL_MAP_NORMAL].color = WHITE;
            model.materials[m].maps[MATERIAL_MAP_NORMAL].value = materials[m].shininess;
            model.materials[m].maps[MATERIAL_MAP_EMISSION].color = (Color){ (unsigned char)(materials[m].emission[0]*255.0f), (unsigned char)(materials[m].emission[1]*255.0f), (unsigned char)(materials[m].emission[2]*255.0f), 255 }; //float emission[3];
            if (materials[m].displacement_texname != NULL) model.materials[m].maps[MATERIAL_MAP_HEIGHT].texture = LoadTexture(materials[m].displacement_texname);  //char *displacement_texname; // disp
        }
        tinyobj_attrib_free(&attrib);
        tinyobj_shapes_free(meshes, meshCount);
        tinyobj_materials_free(materials, materialCount);
        UnloadFileText(fileText);
        RL_FREE(matFaces);
        RL_FREE(vCount);
        RL_FREE(vtCount);
        RL_FREE(vnCount);
        RL_FREE(faceCount);
        if (CHDIR(currentDir) != 0) TRACELOG(LOG_WARNING, "MODEL: [%s] Failed to change working directory", currentDir);
    }
    return model;
}
