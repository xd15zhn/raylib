cmake_minimum_required(VERSION 3.12)
project(raylib VERSION 1.1.6)
set(CMAKE_BUILD_TYPE release)

message(STATUS "project name: ${CMAKE_PROJECT_NAME}")
message(STATUS "project version: ${PROJECT_VERSION}")
message(STATUS "project build type: ${CMAKE_BUILD_TYPE}")

set(RAYLIB_SOURCES
    ${PROJECT_SOURCE_DIR}/src/rcore.c
    ${PROJECT_SOURCE_DIR}/src/rmodels.c
    ${PROJECT_SOURCE_DIR}/src/rshapes.c
    ${PROJECT_SOURCE_DIR}/src/rtext.c
    ${PROJECT_SOURCE_DIR}/src/rtextures.c
    ${PROJECT_SOURCE_DIR}/src/rfiles.c
    ${PROJECT_SOURCE_DIR}/src/rlgl.c
    ${PROJECT_SOURCE_DIR}/src/raymath.c
    ${PROJECT_SOURCE_DIR}/src/basetext.c
)

add_library(${CMAKE_PROJECT_NAME} STATIC ${RAYLIB_SOURCES})
option(USE_TRACELOG "Dependent lirary tracelog, used to print logs." ON)

if (WIN32)
    MESSAGE(STATUS "System is WINDOWS.")
    set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
    set(CMAKE_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_PREFIX}/include)
elseif (UNIX)
    MESSAGE(STATUS "System is UNIX.")
    add_compile_options(-Wno-psabi)
    include(GNUInstallDirs)
endif ()

set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "INSTALL_CONFIGDIR: ${INSTALL_CONFIGDIR}")
message(STATUS "CMAKE_INSTALL_INCLUDEDIR: ${CMAKE_INSTALL_INCLUDEDIR}")

if(USE_TRACELOG)
    message(STATUS "Use dependent lirary tracelog.")
    find_package(tracelog REQUIRED)
    message(STATUS "tracelog_VERSION: ${tracelog_VERSION}")
    message(STATUS "tracelog_DIR: ${tracelog_DIR}")
    message(STATUS "tracelog_LIBS: ${tracelog_LIBS}")
    message(STATUS "tracelog_INCLUDE_DIRS: ${tracelog_INCLUDE_DIRS}")
    add_definitions(-DUSE_TRACELOG)
    target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${tracelog_LIBS})
    target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${tracelog_INCLUDE_DIRS}>)
endif()
target_include_directories(${CMAKE_PROJECT_NAME}
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/external>
    PUBLIC $<BUILD_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(${CMAKE_PROJECT_NAME}
    PUBLIC @CMAKE_INSTALL_PREFIX@/lib/libglfw3.a
    PUBLIC ${tracelog_LIBS}
    winmm
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)
install(TARGETS ${PROJECT_NAME}
    EXPORT install_targets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/
)
install(EXPORT install_targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${INSTALL_CONFIGDIR}
)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/raylib.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/raymath.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    DESTINATION ${INSTALL_CONFIGDIR}
)
