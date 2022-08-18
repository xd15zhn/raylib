# raylib
基于OpenGL的3D图形库。  
原仓库 <https://github.com/raysan5/raylib>，这里根据自己的需求进行了大量修改和裁剪。  

## 依赖库
- <https://github.com/glfw/glfw>
- <https://gitee.com/xd15zhn/tracelog>

## 一个简单的例子
```cpp
#include "raylib.h"
int main(void) {
    Camera camera;
	SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(60);
    InitGraph(1024, 768, "RayLib-3D");
	Init_Camera(&camera);
    Vector3 cubePosition = { 0.0f, 0.0f, 0.0f };

    while (!WindowShouldClose()) {
		Update_Camera(&camera);
        BeginDrawing();
            ClearBackground(RAYWHITE);
            BeginMode3D(camera);
                DrawGrid(120, 5);
                DrawSphere(cubePosition, 2.0f, ORANGE);
            EndMode3D();
            DrawFPS(10, 10);
        EndDrawing();
    }
    CloseGraph();
    return 0;
}
```
```
# CMakeLists.txt
cmake_minimum_required(VERSION 3.12)
project(untitled)
set(CMAKE_BUILD_TYPE release)
add_executable(${CMAKE_PROJECT_NAME} main.cpp)
list(APPEND CMAKE_PREFIX_PATH "E:/cpplibraries/")
find_package(raylib REQUIRED)
message(STATUS "raylib_VERSION: ${raylib_VERSION}")
message(STATUS "raylib_LIBS: ${raylib_LIBS}")
message(STATUS "raylib_INCLUDE_DIRS: ${raylib_INCLUDE_DIRS}")
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${raylib_LIBS})
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${raylib_INCLUDE_DIRS})
```
