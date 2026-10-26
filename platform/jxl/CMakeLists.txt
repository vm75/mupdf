cmake_minimum_required(VERSION 3.16)
project(mupdf_jxl LANGUAGES C CXX)

if(NOT EXISTS "${MUPDF_LIBJXL_SOURCE_DIR}/CMakeLists.txt")
  message(FATAL_ERROR "Initialize the pinned thirdparty/libjxl submodule")
endif()

file(READ "${MUPDF_LIBJXL_SOURCE_DIR}/lib/CMakeLists.txt" _jxl_lib_cmake)
foreach(version_part MAJOR MINOR PATCH)
  if(version_part STREQUAL "MAJOR")
    set(expected 0)
  elseif(version_part STREQUAL "MINOR")
    set(expected 12)
  else()
    set(expected 0)
  endif()
  if(NOT _jxl_lib_cmake MATCHES "set\\(JPEGXL_${version_part}_VERSION[ \t]+${expected}\\)")
    message(FATAL_ERROR "This fork is pinned to libjxl v0.12.0")
  endif()
endforeach()

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

set(LIBJXL_ARCHIVE_DIR "${CMAKE_BINARY_DIR}/lib")
foreach(target jxl_dec jxl_cms hwy brotlidec brotlicommon)
  set_target_properties(${target} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${LIBJXL_ARCHIVE_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${LIBJXL_ARCHIVE_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${LIBJXL_ARCHIVE_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${LIBJXL_ARCHIVE_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "${LIBJXL_ARCHIVE_DIR}")
endforeach()
