# Adapted file from vsg (https://github.com/vsg-dev/VulkanSceneGraph)
#
# MIT License
#
# Copyright(c) 2018-2022 Robert Osfield, Christoph Neuhauser
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
# documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
#.rst:
# Findshaderc
# ----------
#
# Try to find shaderc in the VulkanSDK
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines :prop_tgt:`IMPORTED` target ``shaderc::shaderc``, if
# shaderc has been found.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables::
#
#   shaderc_FOUND          - True if shaderc was found
#   shaderc_INCLUDE_DIRS   - include directories for shaderc
#   shaderc_LIBRARIES      - link against this library to use shaderc
#
# The module will also define two cache variables::
#
#   shaderc_INCLUDE_DIR    - the shaderc include directory
#   shaderc_LIBRARY        - the path to the shaderc library
#

# Added by sgl.
if (DEFINED shaderc_DIR)
    set(ADDITIONAL_PATHS_INCLUDE "${shaderc_DIR}/include")
    set(ADDITIONAL_PATHS_LIBS "${shaderc_DIR}/lib")
elseif (DEFINED glslang_DIR)
    set(ADDITIONAL_PATHS_INCLUDE "${glslang_DIR}/include")
    set(ADDITIONAL_PATHS_LIBS "${glslang_DIR}/lib")
elseif (DEFINED ENV{VULKAN_SDK} AND NOT (${CMAKE_GENERATOR} STREQUAL "MinGW Makefiles" OR ${CMAKE_GENERATOR} STREQUAL "MSYS Makefiles") AND NOT WIN32)
    if(WIN32)
        set(ADDITIONAL_PATHS_INCLUDE "$ENV{VULKAN_SDK}/Include")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(ADDITIONAL_PATHS_LIBS
                "$ENV{VULKAN_SDK}/Lib"
                "$ENV{VULKAN_SDK}/Bin"
            )
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
            set(ADDITIONAL_PATHS_LIBS
                "$ENV{VULKAN_SDK}/Lib32"
                "$ENV{VULKAN_SDK}/Bin32"
            )
        endif()
    else()
        set(ADDITIONAL_PATHS_INCLUDE "$ENV{VULKAN_SDK}/include")
        set(ADDITIONAL_PATHS_LIBS "$ENV{VULKAN_SDK}/lib")
    endif()
endif()

find_path(shaderc_INCLUDE_DIR
    NAMES shaderc/shaderc.h
    HINTS ${ADDITIONAL_PATHS_INCLUDE}
)

find_library(shaderc_LIBRARY
    NAMES shaderc_shared
    HINTS ${ADDITIONAL_PATHS_LIBS}
)


mark_as_advanced(shaderc_INCLUDE_DIR shaderc_LIBRARY)

if(shaderc_LIBRARY AND shaderc_INCLUDE_DIR)
    set(shaderc_FOUND "YES")
    message(STATUS "Found shaderc: ${shaderc_LIBRARY}")
else()
    set(shaderc_FOUND "NO")
    message(STATUS "Failed to find shaderc")
endif()

if(shaderc_FOUND AND NOT TARGET shaderc::shaderc)
    set(shaderc_INCLUDE_DIRS ${shaderc_INCLUDE_DIR})

    add_library(shaderc::shaderc UNKNOWN IMPORTED)
    set_target_properties(shaderc::shaderc PROPERTIES IMPORTED_LOCATION "${shaderc_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${shaderc_INCLUDE_DIRS}")

    set(shaderc_LIBRARIES shaderc::shaderc)
endif()
