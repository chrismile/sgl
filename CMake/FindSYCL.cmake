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
    elseif(DEFINED ONEAPI_COMPILER)
        cmake_path(GET ONEAPI_COMPILER PARENT_PATH BIN_DIR)
        cmake_path(GET BIN_DIR PARENT_PATH ONEAPI_PATH)
        set(ONEAPI_PATH "${ONEAPI_PATH}" CACHE PATH "Path containing the oneAPI SDK")
    endif()
else()
    set(ONEAPI_PATH "${ONEAPI_PATH}" CACHE PATH "Path containing the oneAPI SDK")
endif()

# Check if the compiler is icpx or clang++.
find_program(ONEAPI_COMPILER NAMES "icpx" "clang++" HINTS "${ONEAPI_PATH}/bin")
#find_path(ONEAPI_INCLUDE_DIR NAMES sycl.hpp HINTS "${ONEAPI_PATH}/include/sycl")
#cmake_path(GET ONEAPI_INCLUDE_DIR PARENT_PATH ONEAPI_INCLUDE_DIR)
# spir64_gen not added to targets, as currently only using JIT compilation is supported.
if ((CUDA_FOUND OR CUDAToolkit_FOUND) AND (NOT DEFINED SUPPORT_ONEAPI_CUDA OR SUPPORT_ONEAPI_CUDA))
    set(ONEAPI_SYCL_TARGETS spir64 nvptx64-nvidia-cuda CACHE STRING "oneAPI SYCL targets")
else()
    set(ONEAPI_SYCL_TARGETS spir64 CACHE STRING "oneAPI SYCL targets")
endif()
mark_as_advanced(ONEAPI_COMPILER ONEAPI_PATH ONEAPI_SYCL_TARGETS)
set(ONEAPI_INCLUDE_DIR "${ONEAPI_PATH}/include")

if(DEFINED ONEAPI_PATH AND ONEAPI_COMPILER)
    set(SYCL_FOUND "YES")
    message(STATUS "Found SYCL compiler at ${ONEAPI_COMPILER}")
else()
    set(SYCL_FOUND "NO")
    if (${SYCL_FIND_REQUIRED})
        message(FATAL_ERROR "Failed to find SYCL compiler")
    else()
        message(STATUS "Failed to find SYCL compiler")
    endif()
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
        list(APPEND ONEAPI_COMPILER_FLAGS_DEBUG -Xclang --dependent-lib=ucrtd -Xclang --dependent-lib=msvcrtd -fms-runtime-lib=dll_dbg)
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

# https://support.codeplay.com/t/ptxas-fatal-optimized-debugging-not-supported-when-building-cmake-on-windows/747
list(JOIN ONEAPI_SYCL_TARGETS "," ONEAPI_SYCL_TARGETS_STRING)
list(APPEND ONEAPI_COMPILER_FLAGS "-fsycl-targets=${ONEAPI_SYCL_TARGETS_STRING}")
list(APPEND ONEAPI_LINKER_FLAGS "-fsycl-targets=${ONEAPI_SYCL_TARGETS_STRING}")
foreach(target ${ONEAPI_SYCL_TARGETS})
    if (DEFINED ONEAPI_SYCL_OPTIONS_${target})
        list(APPEND ONEAPI_COMPILER_FLAGS -Xsycl-target-backend=${target} "${ONEAPI_SYCL_OPTIONS_${target}}")
        list(APPEND ONEAPI_LINKER_FLAGS -Xsycl-target-backend=${target} "${ONEAPI_SYCL_OPTIONS_${target}}")
    endif()
    if ("${target}" STREQUAL "nvptx64-nvidia-cuda")
        # https://support.codeplay.com/t/ptxas-fatal-optimized-debugging-not-supported-when-building-cmake-on-windows/747/3
        list(APPEND ONEAPI_LINKER_FLAGS
                $<$<CONFIG:Debug>:-Xcuda-ptxas>
                $<$<CONFIG:Debug>:-suppress-debug-info>
                $<$<CONFIG:Debug>:-g>)
    endif()
endforeach()

# Internal function (for single vs multi config generators).
function(compile_sycl_kernels_single ONEAPI_KERNEL_OUTPUT_DIR sources compiler_flags linker_flags)
    # Commands for compiling source files into object files.
    foreach(ONEAPI_DEVICE_SOURCE IN LISTS sources)
        set(SOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${ONEAPI_DEVICE_SOURCE}")
        set(OBJECT_FILE "${ONEAPI_KERNEL_OUTPUT_DIR}/${ONEAPI_DEVICE_SOURCE}${CMAKE_CXX_OUTPUT_EXTENSION}")
        get_filename_component(OUTPUT_DIR "${OBJECT_FILE}" DIRECTORY)
        file(MAKE_DIRECTORY "${OUTPUT_DIR}")
        add_custom_command(
                COMMAND ${ONEAPI_COMPILER} ${compiler_flags} -c "${SOURCE_FILE}" -o "${OBJECT_FILE}"
                MAIN_DEPENDENCY "${SOURCE_FILE}"
                OUTPUT "${OBJECT_FILE}"
                VERBATIM
        )
        list(APPEND ONEAPI_OBJECT_FILES "${OBJECT_FILE}")
    endforeach()

    # Command for linking object files into a shared library.
    set(ONEAPI_KERNEL_DLL_PATH "${ONEAPI_KERNEL_OUTPUT_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}SyclKernels${CMAKE_SHARED_LIBRARY_SUFFIX}")
    if (CMAKE_LINK_LIBRARY_SUFFIX)
        # On Windows, link libraries have the suffix .lib (same as import libraries and static libraries).
        set(ONEAPI_KERNEL_LIB_PATH "${ONEAPI_KERNEL_OUTPUT_DIR}/SyclKernels${CMAKE_IMPORT_LIBRARY_SUFFIX}")
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
    set(ONEAPI_KERNEL_DLL_PATH "${ONEAPI_KERNEL_DLL_PATH}" PARENT_SCOPE)
    set(ONEAPI_KERNEL_LIB_PATH "${ONEAPI_KERNEL_LIB_PATH}" PARENT_SCOPE)
endfunction()

# Compile kernels to .dll/.so file (SyclKernels) and link target_name with it.
function(compile_sycl_kernels target_name sources compiler_flags linker_flags)
    get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if (${IS_MULTI_CONFIG})
        foreach (CONFIG ${CMAKE_CONFIGURATION_TYPES})
            set(ONEAPI_KERNEL_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${target_name}.dir/${CONFIG}")
            compile_sycl_kernels_single("${ONEAPI_KERNEL_OUTPUT_DIR}" "${sources}" "${compiler_flags}" "${linker_flags}")
            list(APPEND SYCL_CONFIG_DLL_LIST "$<$<CONFIG:${CONFIG}>:${ONEAPI_KERNEL_DLL_PATH}>")
            list(APPEND SYCL_CONFIG_LIB_LIST "$<$<CONFIG:${CONFIG}>:${ONEAPI_KERNEL_LIB_PATH}>")
        endforeach()

        # Copy shared library to binary directory.
        add_custom_command(TARGET ${target_name}
                POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SYCL_CONFIG_DLL_LIST} "$<TARGET_FILE_DIR:${target_name}>")

        # Add the custom target for the kernel library.
        add_custom_target(SyclKernels ALL DEPENDS ${SYCL_CONFIG_LIB_LIST})
        target_link_libraries(${target_name} PRIVATE ${SYCL_CONFIG_LIB_LIST})
    else()
        set(ONEAPI_KERNEL_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${target_name}.dir")
        compile_sycl_kernels_single("${ONEAPI_KERNEL_OUTPUT_DIR}" "${sources}" "${compiler_flags}" "${linker_flags}")

        # Copy shared library to binary directory.
        add_custom_command(TARGET ${target_name}
                POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ONEAPI_KERNEL_DLL_PATH}" "$<TARGET_FILE_DIR:${target_name}>")

        # Add the custom target for the kernel library.
        add_custom_target(SyclKernels ALL DEPENDS ${ONEAPI_KERNEL_LIB_PATH})
        target_link_libraries(${target_name} PRIVATE "${ONEAPI_KERNEL_LIB_PATH}")
    endif()

    add_dependencies(${target_name} SyclKernels)
    if (WIN32)
        set(DLLIMPORT "__declspec(dllimport)")
        target_compile_definitions(${target_name} PRIVATE DLL_OBJECT_SYCL=${DLLIMPORT})
        #set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    else()
        target_compile_definitions(${target_name} PRIVATE DLL_OBJECT_SYCL=)
        target_link_options(${target_name} PRIVATE -Wl,-rpath,'$$ORIGIN')
    endif()
endfunction()

function(helper_sycl_copy_lib target_name pattern)
    if (WIN32)
        if (NOT EXISTS "${pattern}.dll")
            return()
        endif()
        # ".dll" (release) or "d.dll" (debug) need to be prepended on Windows.
        get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
        if (${IS_MULTI_CONFIG})
            foreach (CONFIG ${CMAKE_CONFIGURATION_TYPES})
                if (CONFIG STREQUAL "Debug")
                    list(APPEND SYCL_DEPS_CONFIG_DLL_LIST "$<$<CONFIG:${CONFIG}>:${pattern}d.dll>")
                else()
                    list(APPEND SYCL_DEPS_CONFIG_DLL_LIST "$<$<CONFIG:${CONFIG}>:${pattern}.dll>")
                endif()
            endforeach()
            add_custom_command(TARGET ${target_name}
                    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SYCL_DEPS_CONFIG_DLL_LIST} "$<TARGET_FILE_DIR:${target_name}>")
        else()
            if (CMAKE_BUILD_TYPE STREQUAL "Debug")
                add_custom_command(TARGET ${target_name}
                        POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${pattern}d.dll" "$<TARGET_FILE_DIR:${target_name}>")
            else()
                add_custom_command(TARGET ${target_name}
                        POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${pattern}.dll" "$<TARGET_FILE_DIR:${target_name}>")
            endif()
        endif()
    else()
        # Pattern is used on Linux.
        file(GLOB_RECURSE LIB_FILES "${pattern}")
        foreach(LIB_FILE IN LISTS LIB_FILES)
            if (NOT LIB_FILE MATCHES ".py$")
                add_custom_command(TARGET ${target_name}
                        POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIB_FILE}" "$<TARGET_FILE_DIR:${target_name}>")
            endif()
        endforeach()
    endif()
endfunction()

function(copy_sycl_libs_to_target target_name)
    if (WIN32)
        helper_sycl_copy_lib(${target_name} "${ONEAPI_PATH}/bin/ur_loader")
        helper_sycl_copy_lib(${target_name} "${ONEAPI_PATH}/bin/ur_win_proxy_loader")
        helper_sycl_copy_lib(${target_name} "${ONEAPI_PATH}/bin/ur_adapter_level_zero")
        helper_sycl_copy_lib(${target_name} "${ONEAPI_PATH}/bin/ur_adapter_level_zero_v2")
        helper_sycl_copy_lib(${target_name} "${ONEAPI_PATH}/bin/ur_adapter_cuda")
        helper_sycl_copy_lib(${target_name} "${ONEAPI_PATH}/bin/ur_adapter_hip")
    else()
        helper_sycl_copy_lib(${target_name} "${ONEAPI_PATH}/lib/libsycl.so*")
        helper_sycl_copy_lib(${target_name} "${ONEAPI_PATH}/lib/libur_loader.so*")
        helper_sycl_copy_lib(${target_name} "${ONEAPI_PATH}/lib/libur_adapter_level_zero.so*")
        helper_sycl_copy_lib(${target_name} "${ONEAPI_PATH}/lib/libur_adapter_level_zero_v2.so*")
        helper_sycl_copy_lib(${target_name} "${ONEAPI_PATH}/lib/libur_adapter_cuda.so*")
        helper_sycl_copy_lib(${target_name} "${ONEAPI_PATH}/lib/libur_adapter_hip.so*")
        # helper_sycl_copy_lib(${target_name} "${ONEAPI_PATH}/lib/libur_adapter_opencl.so*")
        helper_sycl_copy_lib(${target_name} "${ONEAPI_PATH}/lib/libumf.so*")
        target_link_options(${target_name} PRIVATE -Wl,-rpath,'$$ORIGIN')
    endif()
endfunction()

# Sets up variables in parent scope for use with CheckCXXSourceCompiles.
function(setup_check_cxx_sycl)
    set(CMAKE_TRY_COMPILE_CONFIGURATION "Release" PARENT_SCOPE)
    set(CMAKE_REQUIRED_INCLUDES "${ONEAPI_INCLUDE_DIR}" PARENT_SCOPE)
    if (CMAKE_VERSION GREATER_EQUAL 3.31)
        set(CMAKE_REQUIRED_LINK_DIRECTORIES "${ONEAPI_PATH}/lib" PARENT_SCOPE)
        if (WIN32)
            set(CMAKE_REQUIRED_LIBRARIES "sycl8.lib" PARENT_SCOPE)
        else()
            set(CMAKE_REQUIRED_LIBRARIES "sycl" PARENT_SCOPE)
        endif()
    else()
        if (WIN32)
            set(CMAKE_REQUIRED_LIBRARIES "${ONEAPI_PATH}/lib/sycl8.lib" PARENT_SCOPE)
        else()
            set(CMAKE_REQUIRED_LIBRARIES "${ONEAPI_PATH}/lib/libsycl.so" PARENT_SCOPE)
        endif()
    endif()
endfunction()
