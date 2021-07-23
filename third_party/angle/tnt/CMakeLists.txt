cmake_minimum_required(VERSION 3.19)
project(angle)

set(OUR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

set(TARGET              angle)
if (WINDOWS_STORE)
    set(LIB_DIR         ${OUR_DIR}/lib/winrt)
else()
    set(LIB_DIR         ${OUR_DIR}/lib/win32)
endif()
set(PUBLIC_HDR_DIR      ${OUR_DIR}/include)

add_library(libEGL_Debug IMPORTED SHARED)
set_target_properties(libEGL_Debug PROPERTIES 
    IMPORTED_LOCATION ${LIB_DIR}/Debug/libEGL.dll
    IMPORTED_IMPLIB ${LIB_DIR}/Debug/libEGL.dll.lib)

add_library(libEGL_Release IMPORTED SHARED)
set_target_properties(libEGL_Release PROPERTIES 
    IMPORTED_LOCATION ${LIB_DIR}/Release/libEGL.dll
    IMPORTED_IMPLIB ${LIB_DIR}/Release/libEGL.dll.lib)

add_library(libGLESv2_Debug IMPORTED SHARED)
set_target_properties(libGLESv2_Debug PROPERTIES 
    IMPORTED_LOCATION ${LIB_DIR}/Debug/libGLESv2.dll
    IMPORTED_IMPLIB ${LIB_DIR}/Debug/libGLESv2.dll.lib)

add_library(libGLESv2_Release IMPORTED SHARED)
set_target_properties(libGLESv2_Release PROPERTIES 
    IMPORTED_LOCATION ${LIB_DIR}/Release/libGLESv2.dll
    IMPORTED_IMPLIB ${LIB_DIR}/Release/libGLESv2.dll.lib)

add_library(zlib_Debug IMPORTED SHARED)
set_target_properties(zlib_Debug PROPERTIES 
    IMPORTED_LOCATION ${LIB_DIR}/Debug/zlib.dll
    IMPORTED_IMPLIB ${LIB_DIR}/Debug/zlib.dll.lib)

add_library(angle INTERFACE)
target_include_directories(angle INTERFACE ${PUBLIC_HDR_DIR})
target_link_libraries(angle INTERFACE debug libEGL_Debug libGLESv2_Debug zlib_Debug optimized libEGL_Release libGLESv2_Release)
