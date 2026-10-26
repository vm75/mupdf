cmake_minimum_required(VERSION 3.16)
project(mupdf_android_jxl LANGUAGES C CXX)

if(NOT ANDROID)
  message(FATAL_ERROR "Use the Android NDK CMake toolchain")
endif()
if(NOT EXISTS "${MUPDF_LIBJXL_SOURCE_DIR}/CMakeLists.txt")
  message(FATAL_ERROR "Set MUPDF_LIBJXL_SOURCE_DIR to the pinned libjxl v0.12.0 checkout")
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
foreach(option TOOLS DEVTOOLS BENCHMARK EXAMPLES JNI DOXYGEN MANPAGES SJPEG
    OPENEXR VIEWERS PLUGINS TCMALLOC)
  set(JPEGXL_ENABLE_${option} OFF CACHE BOOL "" FORCE)
endforeach()
set(JPEGXL_ENABLE_SKCMS ON CACHE BOOL "" FORCE)
set(JPEGXL_FORCE_SYSTEM_HWY OFF CACHE BOOL "" FORCE)
set(JPEGXL_FORCE_SYSTEM_BROTLI OFF CACHE BOOL "" FORCE)
set(HWY_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(HWY_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(HWY_ENABLE_CONTRIB OFF CACHE BOOL "" FORCE)

add_subdirectory("${MUPDF_LIBJXL_SOURCE_DIR}" libjxl EXCLUDE_FROM_ALL)

file(READ "${MUPDF_LIBJXL_SOURCE_DIR}/lib/CMakeLists.txt" _jxl_lib_cmake)
if(NOT _jxl_lib_cmake MATCHES "JPEGXL_MAJOR_VERSION[ \t]+0" OR
   NOT _jxl_lib_cmake MATCHES "JPEGXL_MINOR_VERSION[ \t]+12" OR
   NOT _jxl_lib_cmake MATCHES "JPEGXL_PATCH_VERSION[ \t]+0")
  message(FATAL_ERROR "This fork is pinned to libjxl v0.12.0")
endif()

# Export exact target filenames, without depending on libjxl's build layout.
# skcms is compiled into jxl_cms. libc++ and libatomic are supplied by the NDK.
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/mupdf-jxl-archives.txt" CONTENT
  "$<TARGET_FILE:jxl_dec>\n$<TARGET_FILE:jxl_cms>\n$<TARGET_FILE:hwy>\n$<TARGET_FILE:brotlidec>\n$<TARGET_FILE:brotlicommon>\n")
