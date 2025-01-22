cmake_minimum_required (VERSION 3.7)
cmake_policy(VERSION 3.7...3.30)
project(sgl)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
#MESSAGE(STATUS "CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH})

# Eventuall, we will want to switch to C++20 for Emscripten to be able to use co_await and co_yield as a replacement for
# emscripten_sleep with "-sASYNCIFY".
#if (EMSCRIPTEN)
#    set(CMAKE_CXX_STANDARD 20)
#else()
set(CMAKE_CXX_STANDARD 17)
#endif()
set(CMAKE_DEBUG_POSTFIX d)

file(GLOB_RECURSE SOURCES src/*.cpp src/*.c src/*.hpp src/*.h)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus")
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W3 /EHsc")
elseif(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (NOT EMSCRIPTEN OR NOT DISABLE_WARNINGS)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    endif()
endif()

if(MSVC)
    # Disable "[...] needs to have dll-interface to be used by clients of class [...]".
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w44251")
endif()
if (APPLE)
    # Disable '-Wnullability-completeness' warnings caused by VMA using '_Nonnull'.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-nullability-completeness")
endif()

if (MSVC)
    set(DEFAULT_USE_TBB ON)
else()
    set(DEFAULT_USE_TBB OFF)
endif()

option(BUILD_STATIC_LIBRARY "Build static instead of dynamic library." OFF)
option(USE_STATIC_STD_LIBRARIES "Link with standard libraries statically (only supported on Linux for now)." OFF)
option(USE_GLIBCXX_DEBUG "Use the -D_GLIBCXX_DEBUG flag when compiling with GCC." OFF)
option(USE_PRE_CXX11_ABI "Use the -D_GLIBCXX_USE_CXX11_ABI=0 flag when compiling with GCC." OFF)
option(SUPPORT_OPENGL "Build with OpenGL support." ON)
option(SUPPORT_VULKAN "Build with Vulkan support." ON)
option(SUPPORT_WEBGPU "Build with WebGPU support." OFF)
option(SUPPORT_D3D12 "Build with Direct3D 12 support." OFF)
option(SUPPORT_CUDA "Build with Vulkan-CUDA interoperability support." ON)
option(SUPPORT_OPENCL "Build with Vulkan-OpenCL interoperability support." ON)
option(SUPPORT_HIP "Build with Vulkan-HIP interoperability support." ON)
option(SUPPORT_ONEAPI "Build with Vulkan-oneAPI interoperability support." OFF)
option(USE_SGL_VULKAN_HEADERS "Build with Vulkan headers provided by sgl even if Vulkan headers were found on the system." OFF)
option(USE_SHADERC "Build with shaderc shader compiler backend." ON)
option(USE_BOOST "Build with Boost support." ON)
option(USE_BOOST_ALGORITHM "Build with Boost.Algorithm support." ON)
option(USE_BOOST_SPIRIT "Build with Boost.Spirit support." OFF)
option(USE_BOOST_LOCALE "Build with Boost.Locale support." OFF)
option(USE_ICU "Build with ICU unicode support." OFF)
option(USE_LIBARCHIVE "Build with libarchive support." ON)
option(USE_TINYXML2 "Build with TinyXML2 support." ON)

# Window manager backends
option(USE_SDL2 "Build with SDL2 support." ON)
option(USE_SDL3 "Build with SDL3 support." OFF)
option(USE_GLFW "Build with GLFW support." OFF)
# SDL2_image is disabled by default, as it is only used in the legacy OpenGL texture manager, and no program developed
# by me uses anything more than the PNG format sgl already supports anyways.
# SDL2_image pulls in a lot of dependencies, one being JPEG XL, which uses the vectorization library 'highway' that
# causes problems on Fedora 39 when using different GCC versions due to CUDA incompatibilities. I.e.:
# "/usr/bin/ld: /usr/lib64/libhwy.so.1: undefined reference to `__extendhfsf2@GCC_12.0.0'"
option(USE_SDL2_IMAGE "Build with SDL2_image support." OFF)

option(USE_TBB "Build with TBB threading support instead of using OpenMP." ${DEFAULT_USE_TBB})
option(TRACY_ENABLE "Build with Tracy Profiler support." OFF)
option(EMSCRIPTEN_DEBUG "Build with Emscripten debug symbols." OFF)

if (EMSCRIPTEN)
    # When using emscripten and targeting the web, turn off all meaningless/incompatible components.
    # In the future, Boost could be supported.
    set(BUILD_STATIC_LIBRARY ON)
    set(SUPPORT_OPENGL ON)
    set(SUPPORT_VULKAN OFF)
    set(SUPPORT_WEBGPU ON)
    set(SUPPORT_CUDA OFF)
    set(SUPPORT_OPENCL OFF)
    set(SUPPORT_HIP OFF)
    set(SUPPORT_ONEAPI OFF)
    set(USE_SHADERC OFF)
    set(USE_BOOST ON)
    set(USE_BOOST_SPIRIT ON)
    set(USE_BOOST_ALGORITHM OFF)
    set(USE_BOOST_LOCALE OFF)
    set(USE_LIBARCHIVE OFF)
    set(USE_TINYXML2 OFF)
    set(USE_SDL2_IMAGE ON)
    set(shaderc_FOUND FALSE)
    set(glslang_FOUND FALSE)
    # Mainly to specify ports: https://github.com/orgs/emscripten-ports/repositories
    set(EMSCRIPTEN_SGL_FLAGS "")
    set(EMSCRIPTEN_SGL_LINKER_FLAGS "")
endif()

if (NOT EMSCRIPTEN)
    # Prefer GLVND by default when available.
    if (${CMAKE_VERSION} VERSION_GREATER "3.11")
        cmake_policy(SET CMP0072 NEW)
    endif()
    find_package(OpenGL QUIET)
    find_package(GLEW QUIET)
else()
    set(OpenGL_FOUND TRUE)
    set(GLEW_FOUND TRUE)
endif()
if (${SUPPORT_OPENGL})
    if ((NOT OpenGL_FOUND) OR (NOT GLEW_FOUND))
        MESSAGE(WARNING "Warning: OpenGL support was enabled, but OpenGL or GLEW were not found. Disabling OpenGL support.")
    else()
        MESSAGE(STATUS "Found OpenGL and GLEW. Enabling OpenGL support.")
    endif()
endif()
if (NOT UNIX OR EMSCRIPTEN OR APPLE OR (NOT ${SUPPORT_OPENGL}) OR (NOT OpenGL_FOUND) OR (NOT GLEW_FOUND))
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/OpenGL/Context/OffscreenContextGLX.cpp)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/OpenGL/Context/OffscreenContextGLX.hpp)
endif()
if ((NOT ${SUPPORT_OPENGL}) OR (NOT OpenGL_FOUND) OR (NOT GLEW_FOUND))
    file(GLOB_RECURSE OPENGL_SOURCES
            src/Graphics/OpenGL/*.cpp src/Graphics/OpenGL/*.c src/Graphics/OpenGL/*.h src/Graphics/OpenGL/*.hpp
            src/Graphics/Mesh/*.cpp src/Graphics/Mesh/*.c src/Graphics/Mesh/*.h src/Graphics/Mesh/*.hpp)
    foreach(source ${OPENGL_SOURCES})
        list(REMOVE_ITEM SOURCES ${source})
    endforeach()
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui/imgui_impl_opengl3.h)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui/imgui_impl_opengl3.cpp)
else()
    # EGL context creation is not supported on Windows so far due to the (usually) missing EGL implementation.
    if (WIN32 OR EMSCRIPTEN)
        list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/OpenGL/Context/OffscreenContextEGL.hpp)
        list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/OpenGL/Context/OffscreenContextEGL.cpp)
    endif()
    if (EMSCRIPTEN)
        list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/OpenGL/Context/OffscreenContextGlfw.hpp)
        list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/OpenGL/Context/OffscreenContextGlfw.cpp)
    endif()
    if (WIN32 AND NOT EMSCRIPTEN)
        set(USE_GLFW ON)
    endif()
endif()

if (${USE_GLFW} AND NOT EMSCRIPTEN)
    find_package(glfw3 QUIET)
endif()
if (NOT ${USE_GLFW} OR (NOT EMSCRIPTEN AND NOT glfw3_FOUND))
    file(GLOB_RECURSE GLFW_SOURCES src/GLFW/*.cpp src/GLFW/*.c src/GLFW/*.h src/GLFW/*.hpp)
    foreach(source ${GLFW_SOURCES})
        list(REMOVE_ITEM SOURCES ${source})
    endforeach()
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui/imgui_impl_glfw.h)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui/imgui_impl_glfw.cpp)
    if (${SUPPORT_WEBGPU})
        list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/WebGPU/libs/glfw3webgpu.h)
        list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/WebGPU/libs/glfw3webgpu.c)
    endif()
endif()

set(USE_WEBGPU_IMPL_DAWN FALSE)
if (${SUPPORT_WEBGPU})
    if (NOT WEBGPU_DIST_DIR)
        message(FATAL_ERROR "WEBGPU_DIST_DIR needs to be specified.")
    endif()
    if (WEBGPU_BACKEND)
        if ("${WEBGPU_BACKEND}" STREQUAL "DAWN")
            set(USE_WEBGPU_IMPL_DAWN TRUE)
        endif()
    endif()
    # Use naga_cross (https://github.com/chrismile/naga_cross) if available.
    find_package(naga_cross QUIET)
    if (naga_cross_FOUND)
        message(STATUS "Found naga_cross version ${naga_cross_VERSION}. Enabling GLSL-WGSL cross-compilation support.")
    endif()
else()
    file(GLOB_RECURSE WEBGPU_SOURCES src/Graphics/WebGPU/*.cpp src/Graphics/WebGPU/*.c src/Graphics/WebGPU/*.h src/Graphics/WebGPU/*.hpp)
    foreach(source ${WEBGPU_SOURCES})
        list(REMOVE_ITEM SOURCES ${source})
    endforeach()
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui/imgui_impl_wgpu.h)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui/imgui_impl_wgpu.cpp)
endif()

if (NOT (${SUPPORT_WEBGPU} AND ${USE_WEBGPU_IMPL_DAWN}))
    # Dawn ships custom headers and conflicts with Vulkan::Headers.
    find_package(Vulkan QUIET)
endif()
if (${SUPPORT_VULKAN})
    if(VCPKG_TOOLCHAIN)
        # Finding shaderc is complicated on vcpkg without pkg-config. Assume shaderc is available if glslang was found.
        find_package(glslang CONFIG REQUIRED)
        if (${glslang_FOUND})
            set(shaderc_FOUND TRUE)
        else()
            set(shaderc_FOUND FALSE)
        endif()
    else()
        find_package(PkgConfig QUIET)
        if(DEFINED ENV{VULKAN_SDK})
            list(APPEND glslang_HINTS "$ENV{VULKAN_SDK}/share/glslang")
        endif()
        if (UNIX AND NOT APPLE)
            list(APPEND glslang_HINTS "/usr/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu/cmake/glslang")
        endif()
        #find_package(glslang QUIET HINTS "${glslang_HINTS}")
        #if (NOT ${glslang_FOUND})
        find_package(sgl_glslang HINTS "${CMAKE_CURRENT_SOURCE_DIR}/CMake" "${glslang_HINTS}")
        #endif()
        #if (NOT ${glslang_FOUND} AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24.0")
        #    message(STATUS "Vulkan_glslang_FOUND: ${Vulkan_glslang_FOUND}")
        #    target_link_library(Vulkan::glslang)
        #endif()
        if (${USE_SHADERC})
            pkg_check_modules(SHADERC IMPORTED_TARGET shaderc)
        endif()
        if (${SHADERC_FOUND})
            set(shaderc_FOUND TRUE)
        elseif(${USE_SHADERC})
            find_package(sgl_shaderc HINTS "${CMAKE_CURRENT_SOURCE_DIR}/CMake")
            if (${shaderc_FOUND})
                set(SHADERC_INCLUDEDIR ${shaderc_INCLUDE_DIRS})
            endif()
        else()
            set(shaderc_FOUND FALSE)
        endif()
    endif()

    if (NOT ${shaderc_FOUND} AND NOT ${glslang_FOUND})
        MESSAGE(WARNING "Warning: Vulkan support was enabled, but shaderc or glslang were not found. Disabling Vulkan support.")
    elseif (NOT Vulkan_FOUND AND NOT (${SUPPORT_WEBGPU} AND ${USE_WEBGPU_IMPL_DAWN}))
        MESSAGE(STATUS "Vulkan support was enabled, but the Vulkan SDK was not found. Falling back to the Vulkan headers shipped with sgl.")
    elseif (NOT Vulkan_FOUND AND ${SUPPORT_WEBGPU} AND ${USE_WEBGPU_IMPL_DAWN})
        MESSAGE(STATUS "Vulkan support was enabled, but Dawn is used as the WebGPU implementation, which ships custom headers.")
    else()
        MESSAGE(STATUS "Found Vulkan dependencies. Enabling Vulkan support.")
    endif()
endif()
if ((NOT ${SUPPORT_VULKAN}) OR (NOT ${shaderc_FOUND} AND NOT ${glslang_FOUND}))
    file(GLOB_RECURSE VULKAN_SOURCES src/Graphics/Vulkan/*.cpp src/Graphics/Vulkan/*.c src/Graphics/Vulkan/*.h src/Graphics/Vulkan/*.hpp)
    foreach(source ${VULKAN_SOURCES})
        list(REMOVE_ITEM SOURCES ${source})
    endforeach()
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui/imgui_impl_vulkan.h)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui/imgui_impl_vulkan.cpp)
elseif(${SUPPORT_VULKAN} AND NOT ${shaderc_FOUND})
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/Shader/Internal/IncluderInterface.hpp)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/Shader/Internal/IncluderInterface.cpp)
endif()

if (${SUPPORT_OPENGL} AND OpenGL_FOUND AND GLEW_FOUND)
    # GLEW 2.1.0 added GL_EXT_semaphore, GL_EXT_memory_object, GL_EXT_memory_object_fd, GL_EXT_memory_object_win32.
    if (GLEW_VERSION)
        if (${GLEW_VERSION} VERSION_GREATER_EQUAL "2.1.0")
            set(GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT ON)
        else()
            set(GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT OFF)
        endif()
    else()
        set(GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT OFF)
    endif()

    # vcpkg and MSYS2 GLEW does not set GLEW_VERSION, probably as config mode is used instead of module mode.
    if (NOT EMSCRIPTEN AND (VCPKG_TOOLCHAIN OR MSYS OR MINGW))
        set(GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT ON)
    endif()

    if (NOT ${GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT} AND NOT EMSCRIPTEN)
        MESSAGE(WARNING "Warning: An old version of GLEW is used (< 2.1.0). Disabling Vulkan interoperability support.")
    endif()
    if (NOT ${GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT})
        list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/Utils/Interop.hpp)
        list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/Utils/Interop.cpp)
    endif()
endif()

if (${SUPPORT_D3D12})
    # https://github.com/microsoft/DirectX-Headers
    # https://github.com/microsoft/vcpkg/tree/master/ports/directx-headers
    find_package(directx-headers CONFIG QUIET)
    if(NOT directx-headers_FOUND)
        message(WARNING "DirectX-Headers not found. Disabling D3D12 support.")
        set(SUPPORT_D3D12 OFF)
    else()
        message(STATUS "Found DirectX-Headers version ${directx-headers_VERSION} at ${directx-headers_DIR}.")
    endif()
    # https://github.com/Microsoft/DirectXShaderCompiler
    # https://github.com/microsoft/vcpkg/tree/master/ports/directx-dxc
    # https://packages.msys2.org/package/mingw-w64-x86_64-directx-headers?repo=mingw64
    find_package(directx-dxc CONFIG QUIET)
    if(NOT directx-dxc_FOUND)
        message(WARNING "DirectX Shader Compiler not found. Disabling D3D12 support.")
        set(SUPPORT_D3D12 OFF)
    else()
        message(STATUS "Found DirectX Shader Compiler version at ${directx-dxc_DIR}.")
    endif()
    # TODO: Support DirectX 12 Agility SDK?
    # https://devblogs.microsoft.com/directx/directx12agility/
    #find_package(directx12-agility CONFIG QUIET)
    #target_link_libraries(sgl PRIVATE Microsoft::DirectX12-Agility)
    # https://github.com/walbourn/directx-vs-templates/blob/main/d3d12game_win32/CMakeLists.txt#L216
endif()

if (NOT ${SUPPORT_D3D12})
    # Remove source files as soon as they exist.
    file(GLOB_RECURSE WEBGPU_SOURCES src/Graphics/D3D12/*.cpp src/Graphics/D3D12/*.c src/Graphics/D3D12/*.h src/Graphics/D3D12/*.hpp)
    foreach(source ${WEBGPU_SOURCES})
        list(REMOVE_ITEM SOURCES ${source})
    endforeach()
    # ImGui D3D12 backend is not yet included, as so far sgl only uses Vulkan <-> D3D12 interop
    # and Vulkan is used for drawing the UI.
    #list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui/imgui_impl_dx12.h)
    #list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui/imgui_impl_dx12.cpp)
endif()

if (${SUPPORT_CUDA})
    if(${CMAKE_VERSION} VERSION_LESS "3.18.0")
        # https://cmake.org/cmake/help/git-stage/module/FindCUDA.html
        find_package(CUDA QUIET)
        if(${CUDA_FOUND})
            if(${CUDA_VERSION} VERSION_GREATER_EQUAL "11.5")
                set(USE_CUDA ON)
            else()
                MESSAGE(WARNING "Warning: CUDA version >= 11.5 is required, but ${CUDA_FOUND} is provided. Disabling CUDA support.")
            endif()
        endif()
    else()
        # https://cmake.org/cmake/help/git-stage/module/FindCUDAToolkit.html
        find_package(CUDAToolkit QUIET)
        if(${CUDAToolkit_FOUND})
            if(${CUDAToolkit_VERSION} VERSION_GREATER_EQUAL "11.5")
                set(USE_CUDA ON)
            else()
                MESSAGE(WARNING "Warning: CUDA version >= 11.5 is required, but ${CUDAToolkit_VERSION} is provided. Disabling CUDA support.")
            endif()
        endif()
    endif()
endif()
if (NOT USE_CUDA)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/Utils/InteropCuda.hpp)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/Utils/InteropCuda.cpp)
endif()

if (${SUPPORT_OPENCL})
    find_package(OpenCL QUIET)
    if(${OpenCL_FOUND})
        set(USE_OPENCL ON)
    endif()
endif()
if (NOT USE_OPENCL)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/Utils/InteropOpenCL.hpp)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/Utils/InteropOpenCL.cpp)
endif()

if(NOT DEFINED HIP_PATH)
    if(DEFINED ENV{HIP_PATH})
        set(HIP_PATH $ENV{HIP_PATH} CACHE PATH "Path containing the HIP SDK")
    endif()
endif()
if (${SUPPORT_HIP})
    find_path(HIP_INCLUDE_DIR
            NAMES hip_runtime.h
            HINTS "${HIP_PATH}/include/hip"
    )
    if(HIP_INCLUDE_DIR)
        cmake_path(GET HIP_INCLUDE_DIR PARENT_PATH HIP_INCLUDE_DIR)
        MESSAGE(STATUS "Found HIP SDK. Using includes from: ${HIP_INCLUDE_DIR}.")
        set(USE_HIP ON)
    endif()
endif()
if (NOT USE_HIP)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/Utils/InteropHIP.hpp)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/Utils/InteropHIP.cpp)
endif()

if(NOT DEFINED ONEAPI_PATH)
    if(DEFINED ENV{ONEAPI_PATH})
        set(ONEAPI_PATH $ENV{ONEAPI_PATH} CACHE PATH "Path containing the oneAPI SDK")
    elseif(DEFINED WIN32 AND EXISTS "C:/Program Files (x86)/Intel/oneAPI/compiler/latest")
        set(ONEAPI_PATH "C:/Program Files (x86)/Intel/oneAPI/compiler/latest" CACHE PATH "Path containing the oneAPI SDK")
    endif()
endif()
if (${SUPPORT_ONEAPI})
    find_path(ONEAPI_INCLUDE_DIR
            NAMES sycl.hpp
            HINTS "${ONEAPI_PATH}/include/sycl"
    )
    if(ONEAPI_INCLUDE_DIR)
        cmake_path(GET ONEAPI_INCLUDE_DIR PARENT_PATH ONEAPI_INCLUDE_DIR)
        cmake_path(GET ONEAPI_INCLUDE_DIR PARENT_PATH ONEAPI_ROOT_DIR)
        MESSAGE(STATUS "Found oneAPI SDK. Using includes from: ${ONEAPI_INCLUDE_DIR}.")
        set(USE_ONEAPI ON)
    endif()
endif()

if (${USE_LIBARCHIVE})
    find_package(LibArchive QUIET)
    if(${LibArchive_FOUND})
        MESSAGE(STATUS "Found libarchive. Enabling archive file loading support.")
    else()
        MESSAGE(STATUS "Could not locate libarchive. Disabling archive file loading support.")
        list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/File/Archive.hpp)
        list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/File/Archive.cpp)
    endif()
else()
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/File/Archive.hpp)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/File/Archive.cpp)
endif()

if (${USE_TINYXML2})
    set(FPHSA_NAME_MISMATCHED TRUE)
    if(VCPKG_TOOLCHAIN)
        find_package(tinyxml2 CONFIG REQUIRED)
    else()
        find_package(TinyXML2 REQUIRED)
    endif()
else()
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/XML.hpp)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/XML.cpp)
endif()

if (NOT ${USE_SDL2})
    file(GLOB_RECURSE SDL2_SOURCES src/SDL/*.cpp src/SDL/*.c src/SDL/*.h src/SDL/*.hpp)
    foreach(source ${SDL2_SOURCES})
        list(REMOVE_ITEM SOURCES ${source})
    endforeach()
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui/imgui_impl_sdl2.h)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui/imgui_impl_sdl2.cpp)
    if (${SUPPORT_WEBGPU})
        list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/WebGPU/libs/sdl2webgpu.h)
        list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/WebGPU/libs/sdl2webgpu.c)
    endif()
endif()

if (NOT ${USE_SDL3})
    file(GLOB_RECURSE SDL3_SOURCES src/SDL3/*.cpp src/SDL3/*.c src/SDL3/*.h src/SDL3/*.hpp)
    foreach(source ${SDL3_SOURCES})
        list(REMOVE_ITEM SOURCES ${source})
    endforeach()
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui/imgui_impl_sdl3.h)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGui/imgui_impl_sdl3.cpp)
    if (${SUPPORT_WEBGPU})
        list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/WebGPU/libs/sdl3webgpu.h)
        list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/WebGPU/libs/sdl3webgpu.c)
    endif()
endif()


if (${TRACY_ENABLE})
    list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/submodules/tracy/TracyClient.cpp)
endif()

if (${BUILD_STATIC_LIBRARY})
    add_library(sgl STATIC ${SOURCES})
    target_compile_definitions(sgl INTERFACE SGL_STATIC_LIBRARY)
else()
    add_library(sgl SHARED ${SOURCES})
endif()

if (UNIX AND NOT APPLE)
    # As of 2024-05-02, the following issue appeared on some CI pipeline configurations:
    # /usr/bin/ld: libsgl.a(imgui_impl_opengl3.cpp.o): undefined reference to symbol 'dlclose@@GLIBC_2.2.5'
    target_link_libraries(sgl PUBLIC ${CMAKE_DL_LIBS})
endif()

if (WIN32)
    target_compile_definitions(sgl PRIVATE WIN32)
endif()
if (WIN32)
    set(DLLIMPORT "__declspec(dllimport)")
    set(DLLEXPORT "__declspec(dllexport)")
    target_compile_definitions(sgl INTERFACE DLL_OBJECT=${DLLIMPORT})
    target_compile_definitions(sgl PRIVATE DLL_OBJECT=${DLLEXPORT})
    target_compile_definitions(sgl INTERFACE IMGUI_API=${DLLIMPORT})
    target_compile_definitions(sgl PRIVATE IMGUI_API=${DLLEXPORT})
    target_compile_definitions(sgl INTERFACE IGFD_API=${DLLIMPORT})
    target_compile_definitions(sgl PRIVATE IGFD_API=${DLLEXPORT})
else()
    target_compile_definitions(sgl PUBLIC DLL_OBJECT=)
endif()

if (MSVC)
    # Disable warnings concerning fopen. Unfortunately, fopen_s is not system-independent.
    target_compile_definitions(sgl PRIVATE _CRT_SECURE_NO_WARNINGS)

	# Enable parallel build.
	target_compile_options(sgl PRIVATE "/MP")
endif()

if(NOT EMSCRIPTEN AND (MSYS OR MINGW OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")) AND ${USE_STATIC_STD_LIBRARIES})
    target_link_options(sgl PRIVATE -static-libgcc -static-libstdc++)
endif()

if(NOT EMSCRIPTEN AND (MSYS OR MINGW OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")) AND ${USE_GLIBCXX_DEBUG})
    target_compile_definitions(sgl PRIVATE _GLIBCXX_DEBUG)
endif()

if(NOT EMSCRIPTEN AND (MSYS OR MINGW OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")) AND ${USE_PRE_CXX11_ABI})
    target_compile_definitions(sgl PUBLIC _GLIBCXX_USE_CXX11_ABI=0)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-class-memaccess")
endif()

if (${TRACY_ENABLE})
    target_compile_definitions(sgl PUBLIC TRACY_ENABLE)
    target_compile_definitions(sgl INTERFACE TRACY_IMPORTS)
    target_compile_definitions(sgl PRIVATE TRACY_EXPORTS)
    target_include_directories(sgl PRIVATE submodules)
else()
    target_include_directories(sgl PRIVATE submodules/fallback)
endif()

#include_directories(src)
target_include_directories(sgl PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include/sgl> # <prefix>/include/sgl
)

if (${USE_GLFW} AND (EMSCRIPTEN OR glfw3_FOUND))
    target_compile_definitions(sgl PUBLIC SUPPORT_GLFW)
    if (NOT EMSCRIPTEN AND glfw3_FOUND)
        target_link_libraries(sgl PRIVATE glfw)
        target_compile_definitions(sgl PUBLIC SUPPORT_GLFW)
    endif()
    if (EMSCRIPTEN)
        # https://emscripten.org/docs/compiling/Contrib-Ports.html
        #set(EMSCRIPTEN_SGL_FLAGS "${EMSCRIPTEN_SGL_FLAGS}" "--use-port=contrib.glfw3")
        set(EMSCRIPTEN_SGL_LINKER_FLAGS "${EMSCRIPTEN_SGL_LINKER_FLAGS}" "-sUSE_GLFW=3")
    endif()
endif()


if (${SUPPORT_OPENGL} AND OpenGL_FOUND AND GLEW_FOUND)
    target_compile_definitions(sgl PUBLIC SUPPORT_OPENGL)
    if (NOT EMSCRIPTEN)
        target_link_libraries(sgl PRIVATE ${OPENGL_LIBRARIES} GLEW::GLEW)
        target_include_directories(sgl PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS})
        if (glfw3_FOUND)
            target_link_libraries(sgl PRIVATE glfw)
            target_compile_definitions(sgl PUBLIC SUPPORT_GLFW)
        endif()
    endif()

    if (EMSCRIPTEN)
        set(EMSCRIPTEN_SGL_LINKER_FLAGS "${EMSCRIPTEN_SGL_LINKER_FLAGS}" "-sMAX_WEBGL_VERSION=2")
    endif()

    if (ANDROID OR IOS OR EMSCRIPTEN)
        # TODO: Let the user choose whether to use IMGUI_IMPL_OPENGL_ES2 or IMGUI_IMPL_OPENGL_ES3?
        # ImGui by default uses IMGUI_IMPL_OPENGL_ES2 for Emscripten. Check if IMGUI_IMPL_OPENGL_ES3 is valid.
        target_compile_definitions(sgl PUBLIC IMGUI_IMPL_OPENGL_ES3)
    endif()

    if (${GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT})
        target_compile_definitions(sgl PUBLIC GLEW_SUPPORTS_EXTERNAL_OBJECTS_EXT)
    endif()

    # Conda currently does not provide the EGL headers. We use the fallback provided by sgl in this case.
    if (NOT WIN32)
        get_filename_component(COMPILER_EXE_NAME ${CMAKE_CXX_COMPILER} NAME)
        if ((NOT OPENGL_EGL_INCLUDE_DIRS) OR (COMPILER_EXE_NAME MATCHES "conda"))
            MESSAGE(STATUS "OpenGL support was enabled, but the EGL header files were not found. Falling back to the EGL headers shipped with sgl.")
            target_include_directories(sgl PUBLIC
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/libs>
                    $<INSTALL_INTERFACE:include/sgl/Graphics/Vulkan/libs>
            )
        else()
            MESSAGE(STATUS "Found EGL headers. Using includes from: ${OPENGL_EGL_INCLUDE_DIRS}.")
            target_include_directories(sgl PRIVATE ${OPENGL_EGL_INCLUDE_DIRS})
        endif()
    endif()
endif()

if (${SUPPORT_WEBGPU})
    target_compile_definitions(sgl PUBLIC SUPPORT_WEBGPU)

    # https://github.com/eliemichel/WebGPU-distribution/archive/refs/heads/main.zip
    # Or from: https://eliemichel.github.io/LearnWebGPU/getting-started/hello-webgpu.html:
    # https://github.com/eliemichel/WebGPU-distribution/archive/refs/tags/main-v0.2.0-beta1.zip
    #include(FetchContent)
    #FetchContent_Declare(
    #        webgpu
    #        GIT_REPOSITORY https://github.com/eliemichel/WebGPU-distribution
    #        GIT_TAG        main
    #)
    #FetchContent_MakeAvailable(webgpu)
    add_subdirectory(${WEBGPU_DIST_DIR} WebGPU)
    target_copy_webgpu_binaries(sgl)

    #target_include_directories(sgl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/WebGPU/libs)
    target_include_directories(sgl PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/WebGPU/libs>
            $<INSTALL_INTERFACE:include/sgl/Graphics/WebGPU/libs> # <prefix>/include/sgl
    )

    # Using the target leads to problems with installing sgl, i.e.:
    # - "export called with target "sgl" which requires target "webgpu" that is not in any export set."
    # - "Target "webgpu" INTERFACE_INCLUDE_DIRECTORIES property contains path: [...]
    #    which is prefixed in the build directory."
    #target_link_libraries(sgl PRIVATE webgpu)
    get_target_property(WEBGPU_INTERFACE_COMPILE_DEFINITIONS webgpu INTERFACE_COMPILE_DEFINITIONS)
    get_target_property(WEBGPU_INTERFACE_LINK_LIBRARIES webgpu INTERFACE_LINK_LIBRARIES)
    get_target_property(WEBGPU_IMPORTED_LOCATION webgpu IMPORTED_LOCATION)
    get_target_property(WEBGPU_IMPORTED_IMPLIB webgpu IMPORTED_IMPLIB)
    get_target_property(WEBGPU_INTERFACE_LINK_OPTIONS webgpu INTERFACE_LINK_OPTIONS)
    get_target_property(WEBGPU_INTERFACE_INCLUDE_DIRECTORIES webgpu INTERFACE_INCLUDE_DIRECTORIES)
    if (WEBGPU_INTERFACE_COMPILE_DEFINITIONS)
        message(STATUS "WEBGPU_INTERFACE_COMPILE_DEFINITIONS: ${WEBGPU_INTERFACE_COMPILE_DEFINITIONS}")
        target_compile_definitions(sgl PUBLIC ${WEBGPU_INTERFACE_COMPILE_DEFINITIONS})
    endif()
    if (WEBGPU_INTERFACE_LINK_LIBRARIES)
        message(STATUS "WEBGPU_INTERFACE_LINK_LIBRARIES: ${WEBGPU_INTERFACE_LINK_LIBRARIES}")
        target_link_libraries(sgl PRIVATE ${WEBGPU_INTERFACE_LINK_LIBRARIES})
    endif()
    if ((NOT MSVC OR ${USE_WEBGPU_IMPL_DAWN}) AND WEBGPU_IMPORTED_LOCATION)
        message(STATUS "WEBGPU_IMPORTED_LOCATION: ${WEBGPU_IMPORTED_LOCATION}")
        target_link_libraries(sgl PRIVATE ${WEBGPU_IMPORTED_LOCATION})
    endif()
    if (WEBGPU_IMPORTED_IMPLIB)
        message(STATUS "WEBGPU_IMPORTED_IMPLIB: ${WEBGPU_IMPORTED_IMPLIB}")
        target_link_libraries(sgl PRIVATE ${WEBGPU_IMPORTED_IMPLIB})
    endif()
    if (WEBGPU_INTERFACE_LINK_OPTIONS)
        message(STATUS "WEBGPU_INTERFACE_LINK_OPTIONS: ${WEBGPU_INTERFACE_LINK_OPTIONS}")
        target_link_options(sgl PUBLIC ${WEBGPU_INTERFACE_LINK_OPTIONS})
    endif()
    if (WEBGPU_INTERFACE_INCLUDE_DIRECTORIES)
        message(STATUS "WEBGPU_INTERFACE_INCLUDE_DIRECTORIES: ${WEBGPU_INTERFACE_INCLUDE_DIRECTORIES}")
        #target_include_directories(sgl PRIVATE ${WEBGPU_INTERFACE_INCLUDE_DIRECTORIES})
        target_include_directories(sgl PUBLIC
                $<BUILD_INTERFACE:${WEBGPU_INTERFACE_INCLUDE_DIRECTORIES}>
                $<INSTALL_INTERFACE:include/sgl> # <prefix>/include/sgl
        )
    endif()

    # Only wgpu-native is linked dynamically.
    if (NOT EMSCRIPTEN AND NOT ${USE_WEBGPU_IMPL_DAWN})
        # Find the .so or .dll file from WebGPU to install.
        if (WIN32)
            set(LIBRARY_INSTALL_DIR "bin")
        else()
            set(LIBRARY_INSTALL_DIR "lib")
        endif()
        #file(GLOB ALL_LIBRARY_FILES "${CMAKE_CURRENT_BINARY_DIR}/*.so" "${CMAKE_CURRENT_BINARY_DIR}/*.dll")
        #foreach(LIBRARY_FILE ${ALL_LIBRARY_FILES})
        #    get_filename_component(LIBRARY_NAME ${LIBRARY_FILE} NAME)
        #    if (LIBRARY_NAME MATCHES "wgpu" OR LIBRARY_NAME MATCHES "dawn")
        #        set(WEBGPU_LIBRARY_FILE "${LIBRARY_FILE}")
        #        set(WEBGPU_LIBRARY_NAME "${LIBRARY_NAME}")
        #    endif()
        #endforeach()
        set(WEBGPU_LIBRARY_FILE "${WEBGPU_IMPORTED_LOCATION}")
        get_filename_component(WEBGPU_LIBRARY_NAME ${WEBGPU_LIBRARY_FILE} NAME)

        # Set the rpath, as otherwise apps not using any WebGPU symbols will not set the rpath, but sgl needs it.
        if (NOT EMSCRIPTEN)
            set_target_properties(sgl PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LIBRARY_INSTALL_DIR}")
        endif()
    endif()

    if (${USE_WEBGPU_IMPL_DAWN})
        target_compile_definitions(sgl PRIVATE IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
    else()
        target_compile_definitions(sgl PRIVATE IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
    endif()

    # The code below was moved to sglConfig.cmake.
    #if (NOT EMSCRIPTEN)
    #    add_library(webgpusgl UNKNOWN IMPORTED)
    #    set_target_properties(webgpusgl PROPERTIES IMPORTED_LOCATION "${WEBGPU_LIBRARY_FILE}" INTERFACE_INCLUDE_DIRECTORIES "${WEBGPU_INTERFACE_INCLUDE_DIRECTORIES}")
    #    target_link_libraries(sgl INTERFACE webgpusgl)
    #endif()

    # See: https://github.com/eliemichel/sdl2webgpu/blob/main/CMakeLists.txt
    if (APPLE)
        target_compile_options(sgl PRIVATE -x objective-c)
        target_link_libraries(sgl PRIVATE "-framework CoreVideo" "-framework IOKit" "-framework QuartzCore")
        if (IOS)
            target_link_libraries(sgl PRIVATE "-framework UIKit")
        else()
            target_link_libraries(sgl PRIVATE "-framework Cocoa")
        endif()
    endif()

    # Use naga_cross (https://github.com/chrismile/naga_cross) if available.
    if (naga_cross_FOUND)
        target_link_libraries(sgl PRIVATE naga_cross::static)
        target_compile_definitions(sgl PUBLIC SUPPORT_NAGA_CROSS)
    endif()
endif()

if (${SUPPORT_VULKAN} AND (${shaderc_FOUND} OR ${glslang_FOUND}))
    target_compile_definitions(sgl PUBLIC SUPPORT_VULKAN)
    target_compile_definitions(sgl PUBLIC VK_NO_PROTOTYPES)
    if ((Vulkan_FOUND OR TARGET Vulkan::Headers) AND NOT ${USE_SGL_VULKAN_HEADERS} AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.21.0")
        target_link_libraries(sgl PRIVATE Vulkan::Headers)
        #target_include_directories(sgl INTERFACE
        #        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/libs/Vulkan-Headers>
        #        $<INSTALL_INTERFACE:include/sgl/Graphics/Vulkan/libs/Vulkan-Headers> # <prefix>/include/sgl
        #)
    else()
        target_compile_definitions(sgl INTERFACE USE_SGL_VULKAN_HEADERS)
        target_include_directories(sgl PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Vulkan/libs/Vulkan-Headers>
                $<INSTALL_INTERFACE:include/sgl/Graphics/Vulkan/libs/Vulkan-Headers> # <prefix>/include/sgl
        )
    endif()

    if(NOT VCPKG_TOOLCHAIN)
        if (${glslang_FOUND})
            set(GLSLANG_SHADERLANG_H_PATH "${glslang_INCLUDE_DIR}/glslang/Public/ShaderLang.h")
            target_link_libraries(sgl PRIVATE glslang::glslang ${glslang_LIBRARIES})
            target_include_directories(sgl PRIVATE ${glslang_INCLUDE_DIR})
        endif()
        if (${shaderc_FOUND})
            set(SHADERC_ENV_H_PATH "${SHADERC_INCLUDEDIR}/shaderc/env.h")
            set(SHADERC_SHADERC_H_PATH "${SHADERC_INCLUDEDIR}/shaderc/shaderc.h")
            if (${SHADERC_FOUND})
                target_link_libraries(sgl PRIVATE PkgConfig::SHADERC)
            else()
                target_link_libraries(sgl PRIVATE shaderc::shaderc)
            endif()
        endif()
    endif()

    if(VCPKG_TOOLCHAIN AND NOT ${USE_WEBGPU_IMPL_DAWN})
        # Fix for CMake generators with debug/release configuration available at the same time (e.g., Visual Studio).
        # For more details see: https://github.com/Microsoft/vcpkg/issues/2979
        find_path(VCPKG_INSTALL_ROOT_DIR include/spirv-tools/libspirv.h)
        find_path(VCPKG_INCLUDE_DIR spirv-tools/libspirv.h)
        set(VCPKG_DEBUG_LIB_DIR ${VCPKG_INSTALL_ROOT_DIR}/debug/lib)
        set(VCPKG_RELEASE_LIB_DIR ${VCPKG_INSTALL_ROOT_DIR}/lib)

        find_library(SPIRV_TOOLS_LIB_DEBUG SPIRV-Tools
                PATHS ${VCPKG_DEBUG_LIB_DIR}
                NO_DEFAULT_PATH)
        find_library(SPIRV_TOOLS_LIB_RELEASE SPIRV-Tools
                PATHS ${VCPKG_RELEASE_LIB_DIR}
                NO_DEFAULT_PATH)
        add_library(spirvtools STATIC IMPORTED)
        set_target_properties(spirvtools PROPERTIES
                IMPORTED_LOCATION ${SPIRV_TOOLS_LIB_DEBUG}
                IMPORTED_LOCATION_DEBUG ${SPIRV_TOOLS_LIB_DEBUG}
                IMPORTED_LOCATION_RELEASE ${SPIRV_TOOLS_LIB_RELEASE}
                INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INCLUDE_DIR}")

        find_library(SPIRV_TOOLS_OPT_LIB_DEBUG SPIRV-Tools-opt
                PATHS ${VCPKG_DEBUG_LIB_DIR}
                NO_DEFAULT_PATH)
        find_library(SPIRV_TOOLS_OPT_LIB_RELEASE SPIRV-Tools-opt
                PATHS ${VCPKG_RELEASE_LIB_DIR}
                NO_DEFAULT_PATH)
        add_library(spirvtoolsopt STATIC IMPORTED)
        set_target_properties(spirvtoolsopt PROPERTIES
                IMPORTED_LOCATION ${SPIRV_TOOLS_OPT_LIB_DEBUG}
                IMPORTED_LOCATION_DEBUG ${SPIRV_TOOLS_OPT_LIB_DEBUG}
                IMPORTED_LOCATION_RELEASE ${SPIRV_TOOLS_OPT_LIB_RELEASE}
                INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INCLUDE_DIR}")

        find_library(SPIRV_TOOLS_LINK_LIB_DEBUG SPIRV-Tools-link
                PATHS ${VCPKG_DEBUG_LIB_DIR}
                NO_DEFAULT_PATH)
        find_library(SPIRV_TOOLS_LINK_LIB_RELEASE SPIRV-Tools-link
                PATHS ${VCPKG_RELEASE_LIB_DIR}
                NO_DEFAULT_PATH)
        add_library(spirvtoolslink STATIC IMPORTED)
        set_target_properties(spirvtoolslink PROPERTIES
                IMPORTED_LOCATION ${SPIRV_TOOLS_LINK_LIB_DEBUG}
                IMPORTED_LOCATION_DEBUG ${SPIRV_TOOLS_LINK_LIB_DEBUG}
                IMPORTED_LOCATION_RELEASE ${SPIRV_TOOLS_LINK_LIB_RELEASE}
                INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INCLUDE_DIR}")

        if (MSVC)
            find_library(OSDEPENDENT_LIB_DEBUG OSDependentd
                    PATHS ${VCPKG_DEBUG_LIB_DIR}
                    NO_DEFAULT_PATH)
        else()
            find_library(OSDEPENDENT_LIB_DEBUG OSDependent
                    PATHS ${VCPKG_DEBUG_LIB_DIR}
                    NO_DEFAULT_PATH)
        endif()
        find_library(OSDEPENDENT_LIB_RELEASE OSDependent
                PATHS ${VCPKG_RELEASE_LIB_DIR}
                NO_DEFAULT_PATH)
        add_library(OSDependent STATIC IMPORTED)
        set_target_properties(OSDependent PROPERTIES
                IMPORTED_LOCATION ${OSDEPENDENT_LIB_DEBUG}
                IMPORTED_LOCATION_DEBUG ${OSDEPENDENT_LIB_DEBUG}
                IMPORTED_LOCATION_RELEASE ${OSDEPENDENT_LIB_RELEASE}
                INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INCLUDE_DIR}")

        if (MSVC)
            find_library(OGLCOMPILER_LIB_DEBUG OGLCompilerd
                    PATHS ${VCPKG_DEBUG_LIB_DIR}
                    NO_DEFAULT_PATH)
        else()
            find_library(OGLCOMPILER_LIB_DEBUG OGLCompiler
                    PATHS ${VCPKG_DEBUG_LIB_DIR}
                    NO_DEFAULT_PATH)
        endif()
        find_library(OGLCOMPILER_LIB_RELEASE OGLCompiler
                PATHS ${VCPKG_RELEASE_LIB_DIR}
                NO_DEFAULT_PATH)
        # https://github.com/KhronosGroup/glslang/pull/3426
        if (OGLCOMPILER_LIB_DEBUG AND OGLCOMPILER_LIB_RELEASE)
            set(OGL_COMPILER_HLSL_FOUND ON)
            add_library(OGLCompiler STATIC IMPORTED)
            set_target_properties(OGLCompiler PROPERTIES
                    IMPORTED_LOCATION ${OGLCOMPILER_LIB_DEBUG}
                    IMPORTED_LOCATION_DEBUG ${OGLCOMPILER_LIB_DEBUG}
                    IMPORTED_LOCATION_RELEASE ${OGLCOMPILER_LIB_RELEASE}
                    INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INCLUDE_DIR}")

            if (MSVC)
                find_library(HLSL_LIB_DEBUG HLSLd
                        PATHS ${VCPKG_DEBUG_LIB_DIR}
                        NO_DEFAULT_PATH)
            else()
                find_library(HLSL_LIB_DEBUG HLSL
                        PATHS ${VCPKG_DEBUG_LIB_DIR}
                        NO_DEFAULT_PATH)
            endif()
            find_library(HLSL_LIB_RELEASE HLSL
                    PATHS ${VCPKG_RELEASE_LIB_DIR}
                    NO_DEFAULT_PATH)
            add_library(HLSL STATIC IMPORTED)
            set_target_properties(HLSL PROPERTIES
                    IMPORTED_LOCATION ${HLSL_LIB_DEBUG}
                    IMPORTED_LOCATION_DEBUG ${HLSL_LIB_DEBUG}
                    IMPORTED_LOCATION_RELEASE ${HLSL_LIB_RELEASE}
                    INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INCLUDE_DIR}")
        endif()

        if (MSVC)
            find_library(SPIRV_LIB_DEBUG SPIRVd
                    PATHS ${VCPKG_DEBUG_LIB_DIR}
                    NO_DEFAULT_PATH)
        else()
            find_library(SPIRV_LIB_DEBUG SPIRV
                    PATHS ${VCPKG_DEBUG_LIB_DIR}
                    NO_DEFAULT_PATH)
        endif()
        find_library(SPIRV_LIB_RELEASE SPIRV
                PATHS ${VCPKG_RELEASE_LIB_DIR}
                NO_DEFAULT_PATH)
        add_library(SPIRV STATIC IMPORTED)
        set_target_properties(SPIRV PROPERTIES
                IMPORTED_LOCATION ${SPIRV_LIB_DEBUG}
                IMPORTED_LOCATION_DEBUG ${SPIRV_LIB_DEBUG}
                IMPORTED_LOCATION_RELEASE ${SPIRV_LIB_RELEASE}
                INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INCLUDE_DIR}")

        if (MSVC)
            find_library(MACHINE_INDEPENDENT_LIB_DEBUG MachineIndependentd
                    PATHS ${VCPKG_DEBUG_LIB_DIR}
                    NO_DEFAULT_PATH)
        else()
            find_library(MACHINE_INDEPENDENT_LIB_DEBUG MachineIndependent
                    PATHS ${VCPKG_DEBUG_LIB_DIR}
                    NO_DEFAULT_PATH)
        endif()
        find_library(MACHINE_INDEPENDENT_LIB_RELEASE MachineIndependent
                PATHS ${VCPKG_RELEASE_LIB_DIR}
                NO_DEFAULT_PATH)
        add_library(MachineIndependent STATIC IMPORTED)
        set_target_properties(MachineIndependent PROPERTIES
                IMPORTED_LOCATION ${MACHINE_INDEPENDENT_LIB_DEBUG}
                IMPORTED_LOCATION_DEBUG ${MACHINE_INDEPENDENT_LIB_DEBUG}
                IMPORTED_LOCATION_RELEASE ${MACHINE_INDEPENDENT_LIB_RELEASE}
                INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INCLUDE_DIR}")

        if (MSVC)
            find_library(GENERIC_CODE_GEN_LIB_DEBUG GenericCodeGend
                    PATHS ${VCPKG_DEBUG_LIB_DIR}
                    NO_DEFAULT_PATH)
        else()
            find_library(GENERIC_CODE_GEN_LIB_DEBUG GenericCodeGen
                    PATHS ${VCPKG_DEBUG_LIB_DIR}
                    NO_DEFAULT_PATH)
        endif()
        find_library(GENERIC_CODE_GEN_LIB_RELEASE GenericCodeGen
                PATHS ${VCPKG_RELEASE_LIB_DIR}
                NO_DEFAULT_PATH)
        add_library(GenericCodeGen STATIC IMPORTED)
        set_target_properties(GenericCodeGen PROPERTIES
                IMPORTED_LOCATION ${GENERIC_CODE_GEN_LIB_DEBUG}
                IMPORTED_LOCATION_DEBUG ${GENERIC_CODE_GEN_LIB_DEBUG}
                IMPORTED_LOCATION_RELEASE ${GENERIC_CODE_GEN_LIB_RELEASE}
                INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INCLUDE_DIR}")

        find_library(SHADERC_LIB_DEBUG shaderc
                PATHS ${VCPKG_DEBUG_LIB_DIR}
                NO_DEFAULT_PATH)
        find_library(SHADERC_LIB_RELEASE shaderc
                PATHS ${VCPKG_RELEASE_LIB_DIR}
                NO_DEFAULT_PATH)
        add_library(shaderc STATIC IMPORTED)
        set_target_properties(shaderc PROPERTIES
                IMPORTED_LOCATION ${SHADERC_LIB_DEBUG}
                IMPORTED_LOCATION_DEBUG ${SHADERC_LIB_DEBUG}
                IMPORTED_LOCATION_RELEASE ${SHADERC_LIB_RELEASE}
                INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INCLUDE_DIR}")

        find_library(SHADERC_UTIL_LIB_DEBUG shaderc_util
                PATHS ${VCPKG_DEBUG_LIB_DIR}
                NO_DEFAULT_PATH)
        find_library(SHADERC_UTIL_LIB_RELEASE shaderc_util
                PATHS ${VCPKG_RELEASE_LIB_DIR}
                NO_DEFAULT_PATH)
        add_library(shaderc_util STATIC IMPORTED)
        set_target_properties(shaderc_util PROPERTIES
                IMPORTED_LOCATION ${SHADERC_UTIL_LIB_DEBUG}
                IMPORTED_LOCATION_DEBUG ${SHADERC_UTIL_LIB_DEBUG}
                IMPORTED_LOCATION_RELEASE ${SHADERC_UTIL_LIB_RELEASE}
                INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INCLUDE_DIR}")
        target_link_libraries(spirvtoolsopt INTERFACE spirvtools)
        if (${BUILD_STATIC_LIBRARY})
            target_link_libraries(shaderc INTERFACE shaderc_util)
        endif()
        target_link_libraries(shaderc INTERFACE spirvtoolsopt glslang::glslang)
        if (OGL_COMPILER_HLSL_FOUND)
            target_link_libraries(shaderc INTERFACE OSDependent OGLCompiler HLSL SPIRV MachineIndependent GenericCodeGen)
        else()
            target_link_libraries(shaderc INTERFACE OSDependent SPIRV MachineIndependent GenericCodeGen)
        endif()

        set(SHADERC_ENV_H_PATH "${VCPKG_INCLUDE_DIR}/shaderc/env.h")
        set(SHADERC_SHADERC_H_PATH "${VCPKG_INCLUDE_DIR}/shaderc/shaderc.h")
        if (${BUILD_STATIC_LIBRARY})
            target_link_libraries(sgl PUBLIC shaderc)
        else()
            target_link_libraries(sgl PRIVATE shaderc shaderc_util)
        endif()
    endif()

    if(VCPKG_TOOLCHAIN AND ${USE_WEBGPU_IMPL_DAWN})
        if (${BUILD_STATIC_LIBRARY})
            target_link_libraries(sgl PUBLIC glslang::glslang glslang::glslang-default-resource-limits glslang::SPIRV glslang::SPVRemapper)
        else()
            target_link_libraries(sgl PRIVATE glslang::glslang glslang::glslang-default-resource-limits glslang::SPIRV glslang::SPVRemapper)
        endif()
    endif()

    if(VCPKG_TOOLCHAIN)
        set(GLSLANG_SHADERLANG_H_PATH "${VCPKG_INCLUDE_DIR}/glslang/Public/ShaderLang.h")
    endif()

    if (${glslang_FOUND})
        target_compile_definitions(sgl PRIVATE SUPPORT_GLSLANG_BACKEND)

        # Check for Vulkan 1.3 support.
        file(READ "${GLSLANG_SHADERLANG_H_PATH}" GLSLANG_SHADERLANG_H_PATH)
        string(FIND "${GLSLANG_SHADERLANG_H_PATH}" "EShTargetVulkan_1_3" GLSLANG_VULKAN_13_MATCHRES)
        if (${GLSLANG_VULKAN_13_MATCHRES} EQUAL -1)
            target_compile_definitions(sgl PRIVATE GLSLANG_NO_VULKAN_1_3_SUPPORT)
        endif()

        # Check for VK_EXT_mesh_shader support.
        string(FIND "${GLSLANG_SHADERLANG_H_PATH}" "EShLangMesh," GLSLANG_MESH_SHADER_EXT_MATCHRES)
        if (NOT ${GLSLANG_MESH_SHADER_EXT_MATCHRES} EQUAL -1)
            target_compile_definitions(sgl PUBLIC GLSLANG_MESH_SHADER_EXT_SUPPORT)
        endif()

        # Check for VK_EXT_mesh_shader support.
        string(FIND "${GLSLANG_SHADERLANG_H_PATH}" "setDebugInfo" GLSLANG_DEBUG_INFO_MATCHRES)
        if (NOT ${GLSLANG_DEBUG_INFO_MATCHRES} EQUAL -1)
            target_compile_definitions(sgl PUBLIC GLSLANG_DEBUG_INFO_SUPPORT)
        endif()
    endif()
    if (${shaderc_FOUND} AND NOT (VCPKG_TOOLCHAIN AND ${USE_WEBGPU_IMPL_DAWN}))
        target_compile_definitions(sgl PRIVATE SUPPORT_SHADERC_BACKEND)

        # Check for Vulkan 1.3 support.
        file(READ "${SHADERC_ENV_H_PATH}" SHADERC_ENV_H_STRING)
        string(FIND "${SHADERC_ENV_H_STRING}" "shaderc_env_version_vulkan_1_3" SHADERC_VULKAN_13_MATCHRES)
        if (${SHADERC_VULKAN_13_MATCHRES} EQUAL -1)
            target_compile_definitions(sgl PRIVATE SHADERC_NO_VULKAN_1_3_SUPPORT)
        endif()
    endif()
endif()

if (${SUPPORT_D3D12})
    target_link_libraries(sgl PRIVATE
            d3d12.lib dxgi.lib dxguid.lib uuid.lib
            kernel32.lib user32.lib
            comdlg32.lib advapi32.lib shell32.lib
            ole32.lib oleaut32.lib
            runtimeobject.lib
    )
    target_link_libraries(sgl PRIVATE Microsoft::DirectX-Headers Microsoft::DirectX-Guids)
    target_link_libraries(sgl PRIVATE Microsoft::DirectXShaderCompiler)
    target_compile_definitions(sgl PUBLIC SUPPORT_D3D12)
endif()

if (DEFINED USE_CUDA)
    target_compile_definitions(sgl PUBLIC SUPPORT_CUDA_INTEROP)
    get_target_property(SGL_INTERFACE_COMPILE_DEFINITIONS sgl INTERFACE_COMPILE_DEFINITIONS)
    if(${CUDAToolkit_FOUND})
        MESSAGE(STATUS "Found CUDA. Using includes from: " ${CUDAToolkit_INCLUDE_DIRS})
        target_include_directories(sgl PUBLIC ${CUDAToolkit_INCLUDE_DIRS})
    elseif(${CUDA_FOUND})
        MESSAGE(STATUS "Found CUDA. Using includes from: " ${CUDA_INCLUDE_DIRS})
        target_include_directories(sgl PUBLIC ${CUDA_INCLUDE_DIRS})
    endif()
else()
    MESSAGE(STATUS "CUDA Toolkit was not found. Disabling Vulkan-CUDA interoperability support.")
endif()

if (${USE_OPENCL})
    target_compile_definitions(sgl PUBLIC SUPPORT_OPENCL_INTEROP)
    get_target_property(SGL_INTERFACE_COMPILE_DEFINITIONS sgl INTERFACE_COMPILE_DEFINITIONS)
    MESSAGE(STATUS "Found OpenCL. Using includes from: " ${OpenCL_INCLUDE_DIRS})
    if(VCPKG_TOOLCHAIN)
        target_include_directories(sgl PRIVATE ${OpenCL_INCLUDE_DIRS})
    else()
        target_include_directories(sgl PUBLIC ${OpenCL_INCLUDE_DIRS})
    endif()

    # Check for external semaphore support.
    if(EXISTS "${OpenCL_INCLUDE_DIRS}/CL/cl_version.h")
        file(READ "${OpenCL_INCLUDE_DIRS}/CL/cl_version.h" OPENCL_CL_H_STRING)
        string(FIND "${OPENCL_CL_H_STRING}" "CL_VERSION_3_0" CL_VERSION_3_0_MATCHRES)
        if (${CL_VERSION_3_0_MATCHRES} EQUAL -1)
            target_compile_definitions(sgl PUBLIC SGL_NO_CL_3_0_SUPPORT)
        endif()
    else()
        target_compile_definitions(sgl PUBLIC SGL_NO_CL_3_0_SUPPORT)
    endif()
else()
    MESSAGE(STATUS "OpenCL headers were not found. Disabling Vulkan-OpenCL interoperability support.")
endif()

if (USE_HIP)
    target_compile_definitions(sgl PUBLIC SUPPORT_HIP_INTEROP)
    target_include_directories(sgl PUBLIC "${HIP_INCLUDE_DIR}")
endif()

if (USE_ONEAPI)
    if (NOT (MSYS OR MINGW OR (${CMAKE_GENERATOR} STREQUAL "MinGW Makefiles") OR (${CMAKE_GENERATOR} STREQUAL "MSYS Makefiles")))
        target_compile_definitions(sgl PUBLIC SUPPORT_SYCL_INTEROP)
    endif()
    target_compile_definitions(sgl PUBLIC SUPPORT_ONEAPI_INTEROP)
    target_include_directories(sgl PUBLIC "${ONEAPI_INCLUDE_DIR}")
    target_include_directories(sgl PUBLIC "${ONEAPI_INCLUDE_DIR}/sycl")
    target_link_directories(sgl PUBLIC "${ONEAPI_ROOT_DIR}/lib")
    target_link_libraries(sgl PUBLIC sycl)
endif()

if (DEFINED USE_CUDA OR DEFINED USE_HIP OR DEFINED USE_ONEAPI)
    target_compile_definitions(sgl PUBLIC SUPPORT_COMPUTE_INTEROP)
endif()

# Windows needs Boost::Interprocess for the singleton class.
if (WIN32 OR ${USE_BOOST})
    if (EMSCRIPTEN)
        # Necessary for Boost::Spirit WGSL parser.
        set(EMSCRIPTEN_SGL_FLAGS "${EMSCRIPTEN_SGL_FLAGS}" "-sUSE_BOOST_HEADERS=1")
    elseif (${USE_BOOST_LOCALE})
        target_compile_definitions(sgl PRIVATE USE_BOOST_LOCALE)
        find_package(Boost COMPONENTS system filesystem locale REQUIRED)
    else()
        find_package(Boost COMPONENTS system filesystem REQUIRED)
    endif()
    if (NOT EMSCRIPTEN)
        target_link_libraries(sgl PRIVATE ${Boost_LIBRARIES})
        target_include_directories(sgl PRIVATE ${Boost_INCLUDE_DIR})
    endif()
    if (Boost_FOUND AND ${USE_BOOST_ALGORITHM})
        target_compile_definitions(sgl PRIVATE USE_BOOST_ALGORITHM)
    endif()
endif()

if (${USE_ICU})
    find_package(ICU COMPONENTS uc io i18n)
    if (ICU_FOUND)
        target_compile_definitions(sgl PRIVATE USE_ICU)
        target_link_libraries(sgl PRIVATE ${ICU_LIBRARIES})
        target_include_directories(sgl PRIVATE ${ICU_INCLUDE_DIRS})
    endif()
endif()

# Because of case sensitivity bug when glm and TinyXML2 call find_package_handle_standard_args.
set(FPHSA_NAME_MISMATCHED TRUE)
if (NOT EMSCRIPTEN)
    find_package(PNG REQUIRED)
endif()

if (${USE_SDL2} AND NOT EMSCRIPTEN)
    if (VCPKG_TOOLCHAIN)
        find_package(SDL2 CONFIG REQUIRED)
    else()
        find_package(SDL2 REQUIRED)
    endif()
    if(${USE_SDL2_IMAGE})
        if(VCPKG_TOOLCHAIN)
            find_package(sdl2-image CONFIG QUIET)
            if (NOT sdl2-image_FOUND)
                find_package(SDL2_image CONFIG REQUIRED)
            endif()
        else()
            find_package(SDL2_image REQUIRED)
        endif()
    endif()
endif()

if (${USE_SDL3} AND NOT EMSCRIPTEN)
    if (VCPKG_TOOLCHAIN)
        find_package(SDL3 CONFIG REQUIRED)
    else()
        find_package(SDL3 REQUIRED)
    endif()
endif()

if (NOT EMSCRIPTEN)
    find_package(glm REQUIRED)
else()
    find_package(glm CONFIG REQUIRED)
endif()

if((MSYS OR MINGW OR (${CMAKE_GENERATOR} STREQUAL "MinGW Makefiles") OR (${CMAKE_GENERATOR} STREQUAL "MSYS Makefiles"))
        AND NOT EMSCRIPTEN)
    target_link_libraries(sgl PUBLIC mingw32)
endif()


if (${USE_SDL2})
    target_compile_definitions(sgl PUBLIC SUPPORT_SDL2)
endif()

# For apps that want to support older versions of sgl with only SDL keyboard support.
target_compile_definitions(sgl INTERFACE SGL_INPUT_API_V2)

if (${USE_SDL2} AND NOT EMSCRIPTEN)
    if(VCPKG_TOOLCHAIN)
        if(TARGET SDL2::SDL2-static)
            target_link_libraries(sgl PUBLIC SDL2::SDL2-static)
        else()
            target_link_libraries(sgl PUBLIC SDL2::SDL2)
        endif()
    else()
        target_link_libraries(sgl PUBLIC SDL2::Core)
    endif()
    if(${USE_SDL2_IMAGE})
        target_compile_definitions(sgl PRIVATE USE_SDL2_IMAGE)
        if(VCPKG_TOOLCHAIN)
            if (sdl2-image_FOUND)
                target_link_libraries(sgl PRIVATE SDL2::SDL2_image)
            else()
                target_link_libraries(sgl PRIVATE $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>)
            endif()
        else()
            target_link_libraries(sgl PRIVATE SDL2::Image)
        endif()
    endif()
endif()

if (${USE_SDL2} AND EMSCRIPTEN)
    set(EMSCRIPTEN_SGL_FLAGS "${EMSCRIPTEN_SGL_FLAGS}" "--use-port=sdl2" "--use-port=sdl2_image:formats=png")
endif()


if (${USE_SDL3})
    target_compile_definitions(sgl PUBLIC SUPPORT_SDL3)
endif()

if (${USE_SDL3} AND NOT EMSCRIPTEN)
    if(TARGET SDL3::SDL3-static)
        target_link_libraries(sgl PUBLIC SDL3::SDL3-static)
    else()
        target_link_libraries(sgl PUBLIC SDL3::SDL3)
    endif()
endif()

if (${USE_SDL3} AND EMSCRIPTEN)
    set(EMSCRIPTEN_SGL_FLAGS "${EMSCRIPTEN_SGL_FLAGS}" "--use-port=sdl3" "--use-port=sdl3_image:formats=png")
endif()


if ((${USE_SDL2} OR ${USE_SDL3}) AND EMSCRIPTEN)
    # Already set by WEBGPU_INTERFACE_LINK_OPTIONS.
    #if (${SUPPORT_WEBGPU})
    #    set(EMSCRIPTEN_SGL_LINKER_FLAGS "${EMSCRIPTEN_SGL_LINKER_FLAGS}" "-sUSE_WEBGPU")
    #endif()
    set(EMSCRIPTEN_SGL_LINKER_FLAGS "${EMSCRIPTEN_SGL_LINKER_FLAGS}" "-sASYNCIFY")
    if (${EMSCRIPTEN_DEBUG})
        # "-fwasm-exceptions" clashes with "-sASYNCIFY".
        #set(EMSCRIPTEN_SGL_FLAGS "${EMSCRIPTEN_SGL_FLAGS}" "-fwasm-exceptions")
        # According to https://emscripten.org/docs/porting/exceptions.html, this has a high overhead. Disable when not needed.
        set(EMSCRIPTEN_SGL_FLAGS "${EMSCRIPTEN_SGL_FLAGS}" "-fexceptions")
        set(EMSCRIPTEN_SGL_LINKER_FLAGS "${EMSCRIPTEN_SGL_LINKER_FLAGS}" "-sASSERTIONS" "--profiling-funcs")
    endif()
endif()

if (NOT EMSCRIPTEN)
    target_link_libraries(sgl PRIVATE PNG::PNG)
else()
    set(EMSCRIPTEN_SGL_FLAGS "${EMSCRIPTEN_SGL_FLAGS}" "--use-port=libpng")
endif()

if (${USE_TINYXML2})
    if(VCPKG_TOOLCHAIN)
        target_link_libraries(sgl PRIVATE tinyxml2::tinyxml2)
    else()
        target_link_libraries(sgl PRIVATE ${TINYXML2_LIBRARIES})
    endif()
    target_include_directories(sgl PRIVATE ${TINYXML2_INCLUDE_DIR})
    target_compile_definitions(sgl PUBLIC SUPPORT_TINYXML2)
endif()

IF (NOT WIN32 AND NOT (UNIX AND NOT APPLE))
    find_package(efsw QUIET)
    if(${efsw_FOUND})
        if(UNIX AND NOT APPLE AND ("${efsw_DIR}" MATCHES "/home.*"))
            set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
        endif()
        #file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(efsw REQUIRED)\n")
    endif()
endif()

if(GLM_FOUND OR glm_FOUND)
    target_compile_definitions(sgl PUBLIC USE_GLM)
endif()

if (${USE_LIBARCHIVE})
    if(${LibArchive_FOUND})
        add_definitions(-DUSE_LIBARCHIVE)
        target_link_libraries(sgl PRIVATE ${LibArchive_LIBRARIES})
        target_include_directories(sgl PRIVATE ${LibArchive_INCLUDE_DIRS})
    endif()
endif()

if (NOT EMSCRIPTEN)
    target_include_directories(sgl PRIVATE ${GLM_INCLUDE_DIR} ${GLM_INCLUDE_DIRS})
else()
    target_link_libraries(sgl PRIVATE glm::glm-header-only)
endif()

if(NOT WIN32 AND NOT (UNIX AND NOT APPLE))
    if(${efsw_FOUND})
        add_definitions(-DUSE_EFSW)
        target_link_libraries(sgl PRIVATE efsw::efsw)
    else()
        MESSAGE(STATUS "Could not locate esfw. Disabling filesystem watches.")
    endif()
endif()

if (${USE_TBB})
    find_package(TBB REQUIRED)
    target_link_libraries(sgl PRIVATE TBB::tbb)
    target_compile_definitions(sgl PRIVATE USE_TBB)
else()
    # According to https://devblogs.microsoft.com/cppblog/improved-openmp-support-for-cpp-in-visual-studio/,
    # support for LLVM OpenMP was added with Visual Studio 2019 version 16.9. According to
    # https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=msvc-170,
    # both Visual Studio 2019 version 16.8 and 16.9 have _MSC_VER / MSVC_VERSION 1928, so we need to check
    # for a minimum version 1929.
    if (EMSCRIPTEN)
        # TODO: https://github.com/emscripten-core/emscripten/issues/13892
    elseif (APPLE)
        # There are some problems with Homebrew OpenMP as of 2022-10-20.
        find_package(OpenMP QUIET)
        if (OpenMP_FOUND)
            MESSAGE(STATUS "Found OpenMP version ${OpenMP_CXX_VERSION}. Enabling OpenMP threading support.")
        else()
            MESSAGE(STATUS "OpenMP not found. Disabling OpenMP threading support.")
        endif()
    else()
        find_package(OpenMP REQUIRED)
    endif()
    if (OpenMP_FOUND)
        if(MSVC)
            option(USE_MSVC_OPENMP_LLVM "Use LLVM OpenMP with MSVC (available since Visual Studio 2019 version 16.9)." ON)
            option(USE_MSVC_OPENMP_EXPERIMENTAL "Use experimental OpenMP with MSVC (available since Visual Studio 2019 version 16.9)." ON)

            if(${MSVC_VERSION} GREATER_EQUAL 1929 AND (USE_MSVC_OPENMP_LLVM OR USE_MSVC_OPENMP_EXPERIMENTAL))
                if(USE_MSVC_OPENMP_LLVM)
                    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /openmp:llvm")
                    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /openmp:llvm")
                endif()
                if(USE_MSVC_OPENMP_EXPERIMENTAL)
                    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /openmp:experimental")
                    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /openmp:experimental")
                endif()
                if(USE_MSVC_OPENMP_LLVM AND USE_MSVC_OPENMP_EXPERIMENTAL)
                    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /w44005")
                    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w44005")
                endif()
            endif()
        elseif(NOT TARGET OpenMP::OpenMP_CXX)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
        else()
            target_link_libraries(sgl PRIVATE OpenMP::OpenMP_CXX)
        endif()
    endif()
endif()


if (EMSCRIPTEN)
    target_compile_options(sgl PRIVATE ${EMSCRIPTEN_SGL_FLAGS})
    target_link_options(sgl PRIVATE ${EMSCRIPTEN_SGL_FLAGS})
    target_link_options(sgl PUBLIC ${EMSCRIPTEN_SGL_LINKER_FLAGS})
endif()



file(READ "${CMAKE_CURRENT_SOURCE_DIR}/sglConfig.cmake.in" CONTENTS)
file(WRITE "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "${CONTENTS}")
if(${efsw_FOUND})
    file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(efsw REQUIRED)\n")
endif()
if (${USE_SDL2} AND NOT EMSCRIPTEN)
    file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\n")
    if(DEFINED SDL2_DIR)
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set(SDL2_DIR \"${SDL2_DIR}\")\n")
    endif()
    if(VCPKG_TOOLCHAIN)
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "find_package(SDL2 CONFIG REQUIRED)\n")
        if(${USE_SDL2_IMAGE})
            if (sdl2-image_FOUND)
                file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "find_package(sdl2-image REQUIRED)\n")
            else()
                file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "find_package(SDL2_image CONFIG REQUIRED)\n")
            endif()
        endif()
    else()
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "find_package(SDL2 REQUIRED)\n")
        if(${USE_SDL2_IMAGE})
            file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "find_package(SDL2_image REQUIRED)\n")
        endif()
    endif()
endif()
if(${USE_SDL3} AND NOT EMSCRIPTEN)
    file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\n")
    if(DEFINED SDL3_DIR)
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set(SDL3_DIR \"${SDL3_DIR}\")\n")
    endif()
    if(VCPKG_TOOLCHAIN)
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(SDL3 CONFIG REQUIRED)")
    else()
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(SDL3 REQUIRED)")
    endif()
endif()
if(${SUPPORT_WEBGPU} AND NOT EMSCRIPTEN AND NOT ${USE_WEBGPU_IMPL_DAWN})
    # Add a target for the WebGPU libraries. Unfortunately, imported libraries cannot be directly exported otherwise...
    file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nget_filename_component(_IMPORT_PREFIX \"\${CMAKE_CURRENT_LIST_FILE}\" PATH)\n")
    file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "get_filename_component(_IMPORT_PREFIX \"\${_IMPORT_PREFIX}\" PATH)\n")
    file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "get_filename_component(_IMPORT_PREFIX \"\${_IMPORT_PREFIX}\" PATH)\n")
    file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "get_filename_component(_IMPORT_PREFIX \"\${_IMPORT_PREFIX}\" PATH)\n")
    if (WEBGPU_LIBRARY_NAME MATCHES ".so$")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(webgpusgl SHARED IMPORTED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(webgpusgl PROPERTIES IMPORTED_LOCATION \"\${_IMPORT_PREFIX}/${LIBRARY_INSTALL_DIR}/${WEBGPU_LIBRARY_NAME}\" IMPORTED_NO_SONAME 1)\n")
    else()
        if (MSVC AND WEBGPU_IMPORTED_IMPLIB)
            file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(webgpusgl SHARED IMPORTED)\n")
        else()
            file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(webgpusgl UNKNOWN IMPORTED)\n")
        endif()
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(webgpusgl PROPERTIES IMPORTED_LOCATION \"\${_IMPORT_PREFIX}/${LIBRARY_INSTALL_DIR}/${WEBGPU_LIBRARY_NAME}\")\n")
        if (MSVC AND WEBGPU_IMPORTED_IMPLIB)
            get_filename_component(WEBGPU_IMPLIB_NAME ${WEBGPU_IMPORTED_IMPLIB} NAME)
            file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(webgpusgl PROPERTIES IMPORTED_IMPLIB \"\${_IMPORT_PREFIX}/lib/${WEBGPU_IMPLIB_NAME}\")\n")
        endif()
    endif()
    file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "target_link_libraries(sgl INTERFACE webgpusgl)\n")
    file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set(_IMPORT_PREFIX)\n")
endif()
if (EMSCRIPTEN)
    file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set(EMSCRIPTEN_APP_FLAGS \"--use-port=libpng\")\n")
    file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set(EMSCRIPTEN_LINKER_FLAGS \"-sALLOW_MEMORY_GROWTH\")\n")
    if (${USE_SDL2})
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nset(EMSCRIPTEN_APP_FLAGS \"\${EMSCRIPTEN_APP_FLAGS}\" \"--use-port=sdl2\" \"--use-port=sdl2_image:formats=png\")\n")
    endif()
    if (${USE_SDL3})
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nset(EMSCRIPTEN_APP_FLAGS \"\${EMSCRIPTEN_APP_FLAGS}\" \"--use-port=sdl3\" \"--use-port=sdl3_image:formats=png\")\n")
    endif()
    if (${USE_GLFW})
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nset(EMSCRIPTEN_LINKER_FLAGS \"\${EMSCRIPTEN_LINKER_FLAGS}\" \"-sUSE_GLFW=3\")\n")
    endif()
    file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set(EMSCRIPTEN_LINKER_FLAGS \"\${EMSCRIPTEN_LINKER_FLAGS}\" \"-sASYNCIFY\")\n")
    if (${SUPPORT_WEBGPU})
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set(EMSCRIPTEN_LINKER_FLAGS \"\${EMSCRIPTEN_LINKER_FLAGS}\" \"-sUSE_WEBGPU\")\n")
    endif()
    if (${SUPPORT_OPENGL})
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set(EMSCRIPTEN_LINKER_FLAGS \"\${EMSCRIPTEN_LINKER_FLAGS}\" \"-sMAX_WEBGL_VERSION=2\")\n")
    endif()
    if (${EMSCRIPTEN_DEBUG})
        # "-fwasm-exceptions" clashes with "-sASYNCIFY".
        #file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set(EMSCRIPTEN_APP_FLAGS \"\${EMSCRIPTEN_APP_FLAGS}\" \"-fwasm-exceptions\")\n")
        # According to https://emscripten.org/docs/porting/exceptions.html, this has a high overhead. Disable when not needed.
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set(EMSCRIPTEN_APP_FLAGS \"\${EMSCRIPTEN_APP_FLAGS}\" \"-fexceptions\")\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set(EMSCRIPTEN_LINKER_FLAGS \"\${EMSCRIPTEN_LINKER_FLAGS}\" \"-sASSERTIONS\" \"--profiling-funcs\")\n")
    endif()
endif()
if (${BUILD_STATIC_LIBRARY} AND NOT EMSCRIPTEN)
    file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(PNG REQUIRED)")
    if(${LibArchive_FOUND})
        if(VCPKG_TOOLCHAIN)
            file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(LibXml2 REQUIRED)")
            file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(BZip2 REQUIRED)")
            file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(LibLZMA REQUIRED)")
            file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(zstd REQUIRED)")
            file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(lz4 CONFIG REQUIRED)")
        endif()
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(LibArchive REQUIRED)")
    endif()
    if(GLEW_FOUND AND NOT EMSCRIPTEN)
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\nfind_package(GLEW REQUIRED)")
    endif()

    if (VCPKG_TOOLCHAIN AND ${SUPPORT_VULKAN} AND (${shaderc_FOUND} OR ${glslang_FOUND}))
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "find_package(glslang CONFIG REQUIRED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(spirvtools STATIC IMPORTED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(spirvtools PROPERTIES IMPORTED_LOCATION ${SPIRV_TOOLS_LIB_DEBUG} IMPORTED_LOCATION_DEBUG ${SPIRV_TOOLS_LIB_DEBUG} IMPORTED_LOCATION_RELEASE ${SPIRV_TOOLS_LIB_RELEASE} INTERFACE_INCLUDE_DIRECTORIES \"${VCPKG_INCLUDE_DIR}\")\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(spirvtoolsopt STATIC IMPORTED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(spirvtoolsopt PROPERTIES IMPORTED_LOCATION ${SPIRV_TOOLS_OPT_LIB_DEBUG} IMPORTED_LOCATION_DEBUG ${SPIRV_TOOLS_OPT_LIB_DEBUG} IMPORTED_LOCATION_RELEASE ${SPIRV_TOOLS_OPT_LIB_RELEASE} INTERFACE_INCLUDE_DIRECTORIES \"${VCPKG_INCLUDE_DIR}\")\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(spirvtoolslink STATIC IMPORTED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(spirvtoolslink PROPERTIES IMPORTED_LOCATION ${SPIRV_TOOLS_LINK_LIB_DEBUG} IMPORTED_LOCATION_DEBUG ${SPIRV_TOOLS_LINK_LIB_DEBUG} IMPORTED_LOCATION_RELEASE ${SPIRV_TOOLS_LINK_LIB_RELEASE} INTERFACE_INCLUDE_DIRECTORIES \"${VCPKG_INCLUDE_DIR}\")\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(OSDependent STATIC IMPORTED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(OSDependent PROPERTIES IMPORTED_LOCATION ${OSDEPENDENT_LIB_DEBUG} IMPORTED_LOCATION_DEBUG ${OSDEPENDENT_LIB_DEBUG} IMPORTED_LOCATION_RELEASE ${OSDEPENDENT_LIB_RELEASE} INTERFACE_INCLUDE_DIRECTORIES \"${VCPKG_INCLUDE_DIR}\")\n")
        if (OGL_COMPILER_HLSL_FOUND)
            file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(OGLCompiler STATIC IMPORTED)\n")
            file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(OGLCompiler PROPERTIES IMPORTED_LOCATION ${OGLCOMPILER_LIB_DEBUG} IMPORTED_LOCATION_DEBUG ${OGLCOMPILER_LIB_DEBUG} IMPORTED_LOCATION_RELEASE ${OGLCOMPILER_LIB_RELEASE} INTERFACE_INCLUDE_DIRECTORIES \"${VCPKG_INCLUDE_DIR}\")\n")
            file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(HLSL STATIC IMPORTED)\n")
            file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(HLSL PROPERTIES IMPORTED_LOCATION ${HLSL_LIB_DEBUG} IMPORTED_LOCATION_DEBUG ${HLSL_LIB_DEBUG} IMPORTED_LOCATION_RELEASE ${HLSL_LIB_RELEASE} INTERFACE_INCLUDE_DIRECTORIES \"${VCPKG_INCLUDE_DIR}\")\n")
        endif()
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(SPIRV STATIC IMPORTED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(SPIRV PROPERTIES IMPORTED_LOCATION ${SPIRV_LIB_DEBUG} IMPORTED_LOCATION_DEBUG ${SPIRV_LIB_DEBUG} IMPORTED_LOCATION_RELEASE ${SPIRV_LIB_RELEASE} INTERFACE_INCLUDE_DIRECTORIES \"${VCPKG_INCLUDE_DIR}\")\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(MachineIndependent STATIC IMPORTED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(MachineIndependent PROPERTIES IMPORTED_LOCATION ${MACHINE_INDEPENDENT_LIB_DEBUG} IMPORTED_LOCATION_DEBUG ${MACHINE_INDEPENDENT_LIB_DEBUG} IMPORTED_LOCATION_RELEASE ${MACHINE_INDEPENDENT_LIB_RELEASE} INTERFACE_INCLUDE_DIRECTORIES \"${VCPKG_INCLUDE_DIR}\")\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(GenericCodeGen STATIC IMPORTED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(GenericCodeGen PROPERTIES IMPORTED_LOCATION ${GENERIC_CODE_GEN_LIB_DEBUG} IMPORTED_LOCATION_DEBUG ${GENERIC_CODE_GEN_LIB_DEBUG} IMPORTED_LOCATION_RELEASE ${GENERIC_CODE_GEN_LIB_RELEASE} INTERFACE_INCLUDE_DIRECTORIES \"${VCPKG_INCLUDE_DIR}\")\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(shaderc STATIC IMPORTED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(shaderc PROPERTIES IMPORTED_LOCATION ${SHADERC_LIB_DEBUG} IMPORTED_LOCATION_DEBUG ${SHADERC_LIB_DEBUG} IMPORTED_LOCATION_RELEASE ${SHADERC_LIB_RELEASE} INTERFACE_INCLUDE_DIRECTORIES \"${VCPKG_INCLUDE_DIR}\")\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "target_link_libraries(spirvtoolsopt INTERFACE spirvtools)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "target_link_libraries(shaderc INTERFACE shaderc_util)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "target_link_libraries(shaderc INTERFACE spirvtoolsopt glslang::glslang)\n")
        if (OGL_COMPILER_HLSL_FOUND)
            file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "target_link_libraries(shaderc INTERFACE OSDependent OGLCompiler HLSL SPIRV MachineIndependent GenericCodeGen)\n")
        else()
            file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "target_link_libraries(shaderc INTERFACE OSDependent SPIRV MachineIndependent GenericCodeGen)\n")
        endif()
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "add_library(shaderc_util STATIC IMPORTED)\n")
        file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "set_target_properties(shaderc_util PROPERTIES IMPORTED_LOCATION ${SHADERC_UTIL_LIB_DEBUG} IMPORTED_LOCATION_DEBUG ${SHADERC_UTIL_LIB_DEBUG} IMPORTED_LOCATION_RELEASE ${SHADERC_UTIL_LIB_RELEASE} INTERFACE_INCLUDE_DIRECTORIES \"${VCPKG_INCLUDE_DIR}\")\n")
    endif()

    file(APPEND "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp" "\n")
endif()


# For creating sglConfig.cmake.in
include(GenerateExportHeader)
generate_export_header(sgl)
install(
        TARGETS sgl EXPORT sglTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include/sgl
)

if (${TRACY_ENABLE} AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/submodules/tracy/Tracy.hpp")
    install(
            DIRECTORY ${CMAKE_SOURCE_DIR}/submodules/tracy/
            DESTINATION include/sgl/tracy
            FILES_MATCHING PATTERN "*.h*"
            PATTERN ".github" EXCLUDE
            PATTERN ".vscode" EXCLUDE
            PATTERN "capture" EXCLUDE
            PATTERN "csvexport" EXCLUDE
            PATTERN "doc" EXCLUDE
            PATTERN "examples" EXCLUDE
            PATTERN "extra" EXCLUDE
            PATTERN "getopt" EXCLUDE
            PATTERN "icon" EXCLUDE
            PATTERN "imgui" EXCLUDE
            PATTERN "import-chrome" EXCLUDE
            PATTERN "libbacktrace" EXCLUDE
            PATTERN "library" EXCLUDE
            PATTERN "manual" EXCLUDE
            PATTERN "nfd" EXCLUDE
            PATTERN "profiler" EXCLUDE
            PATTERN "server" EXCLUDE
            PATTERN "test" EXCLUDE
            PATTERN "update" EXCLUDE
            PATTERN "vcpkg" EXCLUDE
            PATTERN "zstd" EXCLUDE
    )
else()
    # If the submodule does not exist or Tracy was not enabled, add empty compile definitions as a fallback.
    if (NOT ${TRACY_ENABLE} OR NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/submodules/tracy/Tracy.hpp")
        install(
                DIRECTORY ${CMAKE_SOURCE_DIR}/submodules/fallback/tracy/
                DESTINATION include/sgl/tracy
                FILES_MATCHING PATTERN "*.h*"
        )
    endif()
endif()

install(
        DIRECTORY ${CMAKE_SOURCE_DIR}/src/
        DESTINATION include/sgl
        FILES_MATCHING PATTERN "*.h*" PATTERN "*.inl"
)

include(CMakePackageConfigHelpers)
#write_basic_package_version_file()

export(
        EXPORT sglTargets
        FILE "${CMAKE_CURRENT_BINARY_DIR}/sgl/sglTargets.cmake"
)
configure_file(
        "${CMAKE_BINARY_DIR}/sglConfig.cmake.tmp"
        "${CMAKE_CURRENT_BINARY_DIR}/sgl/sglConfig.cmake"
        COPYONLY
)

set(ConfigPackageLocation lib/cmake/sgl)
install(
        EXPORT sglTargets
        FILE sglTargets.cmake
        DESTINATION ${ConfigPackageLocation}
)
install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/sgl/sglConfig.cmake"
        DESTINATION ${ConfigPackageLocation}
)
if (WIN32 AND ((CMAKE_CXX_COMPILER_ID STREQUAL "MSVC") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")))
    install(
            FILES $<TARGET_PDB_FILE:sgl>
            DESTINATION bin OPTIONAL
    )
endif()

# Install WebGPU binaries if Emscripten is not used (in this case, the browser provides the implementation).
if (${SUPPORT_WEBGPU} AND NOT EMSCRIPTEN AND NOT ${USE_WEBGPU_IMPL_DAWN})
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.21.0" AND NOT ${USE_WEBGPU_IMPL_DAWN})
        install(IMPORTED_RUNTIME_ARTIFACTS webgpu)
    else()
        install(
                FILES ${WEBGPU_LIBRARY_FILE}
                DESTINATION ${LIBRARY_INSTALL_DIR}
        )
    endif()
    if (MSVC AND WEBGPU_IMPORTED_IMPLIB AND NOT ${USE_WEBGPU_IMPL_DAWN})
        install(
                FILES ${WEBGPU_IMPORTED_IMPLIB}
                DESTINATION lib
        )
    endif()
endif()
# Install the WebGPU includes if available.
if (WEBGPU_INTERFACE_INCLUDE_DIRECTORIES)
    if (${USE_WEBGPU_IMPL_DAWN})
        #get_target_property(WEBGPU_INTERFACE_INCLUDE_DIRECTORIES_2 webgpu_dawn INTERFACE_INCLUDE_DIRECTORIES)
        #message(FATAL_ERROR "WEBGPU_INTERFACE_INCLUDE_DIRECTORIES_2: ${WEBGPU_INTERFACE_INCLUDE_DIRECTORIES_2}")
        foreach(WEBGPU_INTERFACE_INCLUDE_DIRECTORY IN LISTS WEBGPU_INTERFACE_INCLUDE_DIRECTORIES)
            set(DAWN_INCLUDE_DIR "${WEBGPU_INTERFACE_INCLUDE_DIRECTORY}")
        endforeach()
        # [...]/_deps/dawn-src/include
        # -> [...]/_deps/dawn-build/gen/include/dawn/webgpu.h
        get_filename_component(DAWN_INCLUDE_DIR "${DAWN_INCLUDE_DIR}" PATH) # _deps/dawn-src
        get_filename_component(DAWN_INCLUDE_DIR "${DAWN_INCLUDE_DIR}" PATH) # _deps
        set(DAWN_INCLUDE_DIR "${DAWN_INCLUDE_DIR}/dawn-build/gen/include/dawn") # _deps/dawn-build/gen/include/dawn
        #list(APPEND WEBGPU_INTERFACE_INCLUDE_DIRECTORIES "${DAWN_INCLUDE_DIR}")
        set(WEBGPU_INTERFACE_INCLUDE_DIRECTORY_DAWN "${DAWN_INCLUDE_DIR}")
        foreach(WEBGPU_INTERFACE_INCLUDE_DIRECTORY IN LISTS WEBGPU_INTERFACE_INCLUDE_DIRECTORY_DAWN)
            install(
                    DIRECTORY "${WEBGPU_INTERFACE_INCLUDE_DIRECTORY}"
                    DESTINATION include/sgl/webgpu
                    FILES_MATCHING PATTERN "*.h*" PATTERN "*.inl"
            )
        endforeach()
    endif()

    foreach(WEBGPU_INTERFACE_INCLUDE_DIRECTORY IN LISTS WEBGPU_INTERFACE_INCLUDE_DIRECTORIES)
        install(
                DIRECTORY "${WEBGPU_INTERFACE_INCLUDE_DIRECTORY}/"
                DESTINATION include/sgl
                FILES_MATCHING PATTERN "*.h*" PATTERN "*.inl"
        )
    endforeach()
endif()