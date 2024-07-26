# File from vsg (https://github.com/vsg-dev/VulkanSceneGraph)
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
# Findglslang
# ----------
#
# Try to find glslang in the VulkanSDK
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines :prop_tgt:`IMPORTED` target ``glslang::glslang``, if
# glslang has been found.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables::
#
#   glslang_FOUND          - True if glslang was found
#   glslang_INCLUDE_DIRS   - include directories for glslang
#   glslang_LIBRARIES      - link against this library to use glslang
#
# The module will also define two cache variables::
#
#   glslang_INCLUDE_DIR    - the glslang include directory
#   glslang_LIBRARY        - the path to the glslang library
#

# Added by sgl.
if (DEFINED glslang_DIR)
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

find_path(glslang_INCLUDE_DIR
    NAMES glslang/Public/ShaderLang.h
    HINTS ${ADDITIONAL_PATHS_INCLUDE}
)

find_path(spirv_INCLUDE_DIR
    NAMES glslang/SPIRV/GlslangToSpv.h
    HINTS ${ADDITIONAL_PATHS_INCLUDE}
)

find_library(glslang_LIBRARY
    NAMES glslang
    HINTS ${ADDITIONAL_PATHS_LIBS}
)

find_library(OSDependent_LIBRARY
    NAMES OSDependent
    HINTS ${ADDITIONAL_PATHS_LIBS}
)

find_library(SPIRV_LIBRARY
    NAMES SPIRV
    HINTS ${ADDITIONAL_PATHS_LIBS}
)

find_library(OGLCompiler_LIBRARY
    NAMES OGLCompiler
    HINTS ${ADDITIONAL_PATHS_LIBS}
)

find_library(HLSL_LIBRARY
    NAMES HLSL
    HINTS ${ADDITIONAL_PATHS_LIBS}
)

find_library(MachineIndependent_LIBRARY
    NAMES MachineIndependent
    HINTS ${ADDITIONAL_PATHS_LIBS}
)

find_library(GenericCodeGen_LIBRARY
    NAMES GenericCodeGen
    HINTS ${ADDITIONAL_PATHS_LIBS}
)

find_library(SPIRV-Tools_LIBRARY
    NAMES SPIRV-Tools
    HINTS ${ADDITIONAL_PATHS_LIBS}
)

find_library(SPIRV-Tools-opt_LIBRARY
    NAMES SPIRV-Tools-opt
    HINTS ${ADDITIONAL_PATHS_LIBS}
)

if(WIN32)

    find_library(glslang_LIBRARY_debug
        NAMES glslangd
        HINTS ${ADDITIONAL_PATHS_LIBS}
    )

    find_library(OSDependent_LIBRARY_debug
        NAMES OSDependentd
        HINTS ${ADDITIONAL_PATHS_LIBS}
    )

    find_library(SPIRV_LIBRARY_debug
        NAMES SPIRVd
        HINTS ${ADDITIONAL_PATHS_LIBS}
    )

    find_library(OGLCompiler_LIBRARY_debug
        NAMES OGLCompilerd
        HINTS ${ADDITIONAL_PATHS_LIBS}
    )

    find_library(HLSL_LIBRARY_debug
        NAMES HLSLd
        HINTS ${ADDITIONAL_PATHS_LIBS}
    )

    find_library(MachineIndependent_LIBRARY_debug
        NAMES MachineIndependentd
        HINTS ${ADDITIONAL_PATHS_LIBS}
    )

    find_library(GenericCodeGen_LIBRARY_debug
        NAMES GenericCodeGend
        HINTS ${ADDITIONAL_PATHS_LIBS}
    )

    find_library(SPIRV-Tools_LIBRARY_debug
        NAMES SPIRV-Toolsd
        HINTS ${ADDITIONAL_PATHS_LIBS}
    )

    find_library(SPIRV-Tools-opt_LIBRARY_debug
        NAMES SPIRV-Tools-optd
        HINTS ${ADDITIONAL_PATHS_LIBS}
    )

endif()


mark_as_advanced(glslang_INCLUDE_DIR glslang_LIBRARY)

if(glslang_LIBRARY AND glslang_INCLUDE_DIR AND SPIRV-Tools_LIBRARY AND SPIRV-Tools-opt_LIBRARY)
    set(glslang_FOUND "YES")
    message(STATUS "Found glslang: ${glslang_LIBRARY}")
else()
    set(glslang_FOUND "NO")
    message(STATUS "Failed to find glslang")
endif()

if(glslang_FOUND AND NOT TARGET glslang::glslang)
    set(glslang_INCLUDE_DIRS ${glslang_INCLUDE_DIR})

    add_library(glslang::glslang UNKNOWN IMPORTED)
    set_target_properties(glslang::glslang PROPERTIES IMPORTED_LOCATION "${glslang_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${glslang_INCLUDE_DIRS}")

    if (OSDependent_LIBRARY)
        add_library(glslang::OSDependent UNKNOWN IMPORTED)
        set_target_properties(glslang::OSDependent PROPERTIES IMPORTED_LOCATION "${OSDependent_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${OSDependent_INCLUDE_DIRS}")
    endif()

    add_library(glslang::SPIRV UNKNOWN IMPORTED)
    set_target_properties(glslang::SPIRV PROPERTIES IMPORTED_LOCATION "${SPIRV_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${spirv_INCLUDE_DIR}")

    # https://github.com/KhronosGroup/glslang/pull/3426
    if (OGLCompiler_LIBRARY)
        add_library(glslang::OGLCompiler UNKNOWN IMPORTED)
        set_target_properties(glslang::OGLCompiler PROPERTIES IMPORTED_LOCATION "${OGLCompiler_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${glslang_INCLUDE_DIRS}")
    endif()

    # https://github.com/KhronosGroup/glslang/pull/3426
    if (HLSL_LIBRARY)
        add_library(glslang::HLSL UNKNOWN IMPORTED)
        set_target_properties(glslang::HLSL PROPERTIES IMPORTED_LOCATION "${HLSL_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${glslang_INCLUDE_DIRS}")
    endif()

    if (SPIRV-Tools_LIBRARY)
        add_library(glslang::SPIRV-Tools UNKNOWN IMPORTED)
        set_target_properties(glslang::SPIRV-Tools PROPERTIES IMPORTED_LOCATION "${SPIRV-Tools_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${SPIRV-Tools__INCLUDE_DIR}")
    endif()

    if (SPIRV-Tools-opt_LIBRARY)
        add_library(glslang::SPIRV-Tools-opt UNKNOWN IMPORTED)
        set_target_properties(glslang::SPIRV-Tools-opt PROPERTIES IMPORTED_LOCATION "${SPIRV-Tools-opt_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${SPIRV-Tools-opt_INCLUDE_DIR}")
    endif()

    if (MachineIndependent_LIBRARY)
        add_library(glslang::MachineIndependent UNKNOWN IMPORTED)
        set_target_properties(glslang::MachineIndependent PROPERTIES IMPORTED_LOCATION "${MachineIndependent_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${MachineIndependent_INCLUDE_DIRS}")
    endif()

    if (GenericCodeGen_LIBRARY)
        add_library(glslang::GenericCodeGen UNKNOWN IMPORTED)
        set_target_properties(glslang::GenericCodeGen PROPERTIES IMPORTED_LOCATION "${GenericCodeGen_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${GenericCodeGen_INCLUDE_DIRS}")
    endif()

    if(WIN32)
        if (glslang_LIBRARY_debug)
            set_target_properties(glslang::glslang PROPERTIES IMPORTED_LOCATION_DEBUG "${glslang_LIBRARY_debug}")
        endif()
        if (OSDependent_LIBRARY_debug)
            set_target_properties(glslang::OSDependent PROPERTIES IMPORTED_LOCATION_DEBUG "${OSDependent_LIBRARY_debug}")
        endif()
        if (SPIRV_LIBRARY_debug)
            set_target_properties(glslang::SPIRV PROPERTIES IMPORTED_LOCATION_DEBUG "${SPIRV_LIBRARY_debug}")
        endif()
        if (OGLCompiler_LIBRARY_debug)
            set_target_properties(glslang::OGLCompiler PROPERTIES IMPORTED_LOCATION_DEBUG "${OGLCompiler_LIBRARY_debug}")
        endif()
        if (HLSL_LIBRARY_debug)
            set_target_properties(glslang::HLSL PROPERTIES IMPORTED_LOCATION_DEBUG "${HLSL_LIBRARY_debug}")
        endif()
        if (MachineIndependent_LIBRARY_debug)
            set_target_properties(glslang::MachineIndependent PROPERTIES IMPORTED_LOCATION_DEBUG "${MachineIndependent_LIBRARY_debug}")
        endif()
        if (GenericCodeGen_LIBRARY_debug)
            set_target_properties(glslang::GenericCodeGen PROPERTIES IMPORTED_LOCATION_DEBUG "${GenericCodeGen_LIBRARY_debug}")
        endif()
        if (SPIRV-Tools_LIBRARY_debug)
            set_target_properties(glslang::SPIRV-Tools PROPERTIES IMPORTED_LOCATION_DEBUG "${SPIRV-Tools_LIBRARY_debug}")
        endif()
        if (SPIRV-Tools-opt_LIBRARY_debug)
            set_target_properties(glslang::SPIRV-Tools-opt PROPERTIES IMPORTED_LOCATION_DEBUG "${SPIRV-Tools-opt_LIBRARY_debug}")
        endif()
    endif()

    set(glslang_LIBRARIES "")
    if (MachineIndependent_LIBRARY AND GenericCodeGen_LIBRARY)
        # VULKAN_SDK and associated glslang 1.2.147.1 onwards
        list(APPEND glslang_LIBRARIES "glslang::MachineIndependent")
        if (OSDependent_LIBRARY)
            list(APPEND glslang_LIBRARIES "glslang::OSDependent")
        endif()
        if (OGLCompiler_LIBRARY)
            list(APPEND glslang_LIBRARIES "glslang::OGLCompiler")
        endif()
        list(APPEND glslang_LIBRARIES "glslang::SPIRV")
        list(APPEND glslang_LIBRARIES "glslang::GenericCodeGen")
    else()
        # VULKAN_SDK and associated glslang before 1.2.147.1
        list(APPEND glslang_LIBRARIES "glslang::glslang")
        if (OSDependent_LIBRARY)
            list(APPEND glslang_LIBRARIES "glslang::OSDependent")
        endif()
        if (OGLCompiler_LIBRARY)
            list(APPEND glslang_LIBRARIES "glslang::OGLCompiler")
        endif()
        list(APPEND glslang_LIBRARIES "glslang::SPIRV")
        list(APPEND glslang_LIBRARIES "glslang::HLSL")
    endif()

    #if (MachineIndependent_LIBRARY AND GenericCodeGen_LIBRARY)
    #    # VULKAN_SDK and associated glslang 1.2.147.1 onwards
    #    if (OGLCompiler_LIBRARY)
    #        set(glslang_LIBRARIES
    #                glslang::MachineIndependent
    #                glslang::OSDependent
    #                glslang::OGLCompiler
    #                glslang::SPIRV
    #                glslang::GenericCodeGen
    #        )
    #    else()
    #        set(glslang_LIBRARIES
    #                glslang::MachineIndependent
    #                glslang::OSDependent
    #                glslang::SPIRV
    #                glslang::GenericCodeGen
    #        )
    #    endif()
    #else()
    #    # VULKAN_SDK and associated glslang before 1.2.147.1
    #    if (OGLCompiler_LIBRARY)
    #        set(glslang_LIBRARIES
    #                glslang::glslang
    #                glslang::OSDependent
    #                glslang::OGLCompiler
    #                glslang::SPIRV
    #                glslang::HLSL
    #        )
    #    else()
    #        set(glslang_LIBRARIES
    #                glslang::glslang
    #                glslang::OSDependent
    #                glslang::SPIRV
    #                glslang::HLSL
    #        )
    #    endif()
    #endif()

    if (SPIRV-Tools-opt_LIBRARY)
        list(APPEND glslang_LIBRARIES glslang::SPIRV-Tools-opt)
    endif()
    if (SPIRV-Tools_LIBRARY)
        list(APPEND glslang_LIBRARIES glslang::SPIRV-Tools)
    endif()

endif()
