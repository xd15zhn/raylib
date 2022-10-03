# V1.1.5
- [CMakeLists.txt] RENAMED: `CMAKE_PREFIX_PATH` to `CMAKE_INSTALL_PREFIX`
- [rcamera.c] RENAMED: `Init_Camera` to `InitDefaultCamera`
- [rcamera.c] MODIFIED: 若干变量改为宏。

# V1.1.6
- [rmodels.c] DELETED: `GenMeshPlane`。
- [rmodels.c] DELETED: `GenMeshPoly`。
- [rcamera.c] MOVED: `InitDefaultCamera` to `rcore.c`。

# V1.1.7
- [rcore.c] CHANGED: `SetWindowMonitor`(全屏时窗口能选择建立在哪个显示器上)。
