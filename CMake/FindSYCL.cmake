# BSD 2-Clause License
#
# Copyright (c) 2025, Christoph Neuhauser
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

if(NOT DEFINED ONEAPI_PATH)
    if(DEFINED ENV{ONEAPI_PATH})
        set(ONEAPI_PATH $ENV{ONEAPI_PATH} CACHE PATH "Path containing the oneAPI SDK")
    elseif(DEFINED WIN32 AND EXISTS "C:/Program Files (x86)/Intel/oneAPI/compiler/latest")
        set(ONEAPI_PATH "C:/Program Files (x86)/Intel/oneAPI/compiler/latest" CACHE PATH "Path containing the oneAPI SDK")
    elseif(NOT DEFINED WIN32 AND EXISTS "/opt/intel/oneapi/compiler/latest")
        set(ONEAPI_PATH "/opt/intel/oneapi/compiler/latest" CACHE PATH "Path containing the oneAPI SDK")
    endif()
endif()

# Check if the compiler is icpx or clang++.
find_program(ONEAPI_COMPILER NAMES "icpx" "clang++" HINTS "${ONEAPI_PATH}/bin")
#find_path(ONEAPI_INCLUDE_DIR NAMES sycl.hpp HINTS "${ONEAPI_PATH}/include/sycl")
#cmake_path(GET ONEAPI_INCLUDE_DIR PARENT_PATH ONEAPI_INCLUDE_DIR)
mark_as_advanced(ONEAPI_COMPILER ONEAPI_PATH)
set(ONEAPI_INCLUDE_DIR "${ONEAPI_PATH}/include")

if(ONEAPI_COMPILER)
    set(SYCL_FOUND "YES")
    message(STATUS "Found SYCL compiler at ${ONEAPI_COMPILER}")
else()
    set(SYCL_FOUND "NO")
    message(STATUS "Failed to find SYCL compiler")
endif()

# For libraries/applications linking to SYCL without compiling device kernels.
function(target_link_sycl_host target_name)
    target_include_directories(${target_name} PUBLIC "${ONEAPI_INCLUDE_DIR}")
    target_include_directories(${target_name} PUBLIC "${ONEAPI_INCLUDE_DIR}/sycl")
    target_link_directories(${target_name} PUBLIC "${ONEAPI_PATH}/lib")
    if (WIN32)
        target_link_libraries(${target_name} PUBLIC "sycl8$<$<CONFIG:Debug>:d>.lib")
    else()
        target_link_libraries(${target_name} PUBLIC sycl)
    endif()
    target_compile_definitions(${target_name} PUBLIC SYCL_DISABLE_FSYCL_SYCLHPP_WARNING)
endfunction()

function(append_conditional list_dst list_src condition)
    foreach(list_item IN LISTS ${list_src})
        list(APPEND ${list_dst} $<$<CONFIG:${condition}>:${list_item}>)
    endforeach()
    set(${list_dst} ${${list_dst}} PARENT_SCOPE)
endfunction()

# Set up the SYCL compiler arguments.
if (SYCL_FOUND)
    set(ONEAPI_COMPILER_FLAGS -fsycl -fsycl-unnamed-lambda)
    if (WIN32)
        list(APPEND ONEAPI_COMPILER_FLAGS -fms-extensions -fms-compatibility -D_DLL "-DDLL_OBJECT_SYCL=__declspec(dllexport)")
    else()
        list(APPEND ONEAPI_COMPILER_FLAGS "-DDLL_OBJECT_SYCL=")
    endif()
    #list(APPEND ONEAPI_COMPILER_FLAGS -isystem "${ONEAPI_PATH}/include" -isystem "${ONEAPI_PATH}/include/sycl")
    set(ONEAPI_COMPILER_FLAGS_RELEASE -DNDEBUG)
    set(ONEAPI_COMPILER_FLAGS_MINSIZEREL -DNDEBUG)
    set(ONEAPI_COMPILER_FLAGS_RELWITHDEBINFO -g -DNDEBUG)
    set(ONEAPI_COMPILER_FLAGS_DEBUG -g -O0 -D_DEBUG)
    if (WIN32)
        list(APPEND ONEAPI_COMPILER_FLAGS_DEBUG -Xclang --dependent-lib=ucrtd -Xclang --dependent-lib=msvcrtd)
    else()
        list(APPEND ONEAPI_COMPILER_FLAGS -fPIC)
    endif()
    append_conditional(ONEAPI_COMPILER_FLAGS "${ONEAPI_COMPILER_FLAGS_RELEASE}" Release)
    append_conditional(ONEAPI_COMPILER_FLAGS ONEAPI_COMPILER_FLAGS_MINSIZEREL MinSizeRel)
    append_conditional(ONEAPI_COMPILER_FLAGS ONEAPI_COMPILER_FLAGS_RELWITHDEBINFO RelWithDebInfo)
    append_conditional(ONEAPI_COMPILER_FLAGS ONEAPI_COMPILER_FLAGS_DEBUG Debug)

    set(ONEAPI_LINKER_FLAGS -fsycl -shared "-L${ONEAPI_PATH}/lib")
    if (WIN32)
        set(ONEAPI_LINKER_FLAGS_DEBUG -fuse-ld=lld-link -nostdlib)
        list(APPEND ONEAPI_LINKER_FLAGS_DEBUG -Xlinker /defaultlib:ucrtd)
        list(APPEND ONEAPI_LINKER_FLAGS_DEBUG -Xlinker /defaultlib:msvcrtd)
        append_conditional(ONEAPI_LINKER_FLAGS ONEAPI_LINKER_FLAGS_DEBUG Debug)
    else()
        list(APPEND ONEAPI_LINKER_FLAGS -fPIC -Wl,-rpath,'$$ORIGIN')
    endif()
endif()

# Compile kernels to .dll/.so file (SyclKernels) and link target_name with it.
function(compile_sycl_kernels target_name sources compiler_flags linker_flags)
    foreach(ONEAPI_DEVICE_SOURCE IN LISTS sources)
        set(SOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${ONEAPI_DEVICE_SOURCE}")
        set(OBJECT_FILE "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${target_name}.dir/${ONEAPI_DEVICE_SOURCE}${CMAKE_CXX_OUTPUT_EXTENSION}")
        add_custom_command(
                COMMAND ${ONEAPI_COMPILER} ${compiler_flags} -c "${SOURCE_FILE}" -o "${OBJECT_FILE}"
                MAIN_DEPENDENCY "${SOURCE_FILE}"
                OUTPUT "${OBJECT_FILE}"
                VERBATIM
        )
        list(APPEND ONEAPI_SOURCE_FILES "${SOURCE_FILE}")
        list(APPEND ONEAPI_OBJECT_FILES "${OBJECT_FILE}")
    endforeach()
    set(ONEAPI_KERNEL_DLL_PATH "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${target_name}.dir/${CMAKE_SHARED_LIBRARY_PREFIX}SyclKernels${CMAKE_SHARED_LIBRARY_SUFFIX}")
    if (CMAKE_LINK_LIBRARY_SUFFIX)
        # On Windows, link libraries have the suffix .lib (same as import libraries and static libraries).
        set(ONEAPI_KERNEL_LIB_PATH "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${target_name}.dir/SyclKernels${CMAKE_IMPORT_LIBRARY_SUFFIX}")
    else()
        # This platform has no import library. Just link with the .so object.
        set(ONEAPI_KERNEL_LIB_PATH "${ONEAPI_KERNEL_DLL_PATH}")
    endif()
    add_custom_command(
            COMMAND ${ONEAPI_COMPILER} ${linker_flags} -o "${ONEAPI_KERNEL_DLL_PATH}" "${ONEAPI_OBJECT_FILES}"
            DEPENDS ${ONEAPI_OBJECT_FILES}
            OUTPUT "${ONEAPI_KERNEL_LIB_PATH}"
            VERBATIM
    )
    add_custom_command(TARGET ${target_name}
            POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ONEAPI_KERNEL_DLL_PATH}" "$<TARGET_FILE_DIR:${target_name}>")
    add_custom_target(SyclKernels ALL DEPENDS ${ONEAPI_KERNEL_LIB_PATH})
    add_dependencies(${target_name} SyclKernels)
    target_link_libraries(${target_name} PRIVATE "${ONEAPI_KERNEL_LIB_PATH}")
    if (WIN32)
        set(DLLIMPORT "__declspec(dllimport)")
        target_compile_definitions(${target_name} PRIVATE DLL_OBJECT_SYCL=${DLLIMPORT})
        #set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    else()
        target_compile_definitions(${target_name} PRIVATE DLL_OBJECT_SYCL=)
        target_link_options(${target_name} PRIVATE -Wl,-rpath,'$$ORIGIN')
    endif()
endfunction()

# Sets up variables in parent scope for use with CheckCXXSourceCompiles.
function(setup_check_cxx_sycl)
    set(CMAKE_TRY_COMPILE_CONFIGURATION "Release" PARENT_SCOPE)
    set(CMAKE_REQUIRED_INCLUDES "${ONEAPI_INCLUDE_DIR}" PARENT_SCOPE)
    set(CMAKE_REQUIRED_LINK_DIRECTORIES "${ONEAPI_PATH}/lib" PARENT_SCOPE)
    if (WIN32)
        set(CMAKE_REQUIRED_LIBRARIES "sycl8.lib" PARENT_SCOPE)
    else()
        set(CMAKE_REQUIRED_LIBRARIES "sycl" PARENT_SCOPE)
    endif()
endfunction()
