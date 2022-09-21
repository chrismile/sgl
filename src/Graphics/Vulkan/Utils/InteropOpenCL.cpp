/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2022, Christoph Neuhauser
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "InteropOpenCL.hpp"

#include <map>
#include <boost/algorithm/string/case_conv.hpp>
#if defined(__linux__)
#include <dlfcn.h>
#include <unistd.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#elif defined(__APPLE__)
#include <dlfcn.h>
#endif

#include <Math/Math.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/StringUtils.hpp>

#ifndef CL_DEVICE_BOARD_NAME_AMD
#define CL_DEVICE_BOARD_NAME_AMD 0x4038
#endif

namespace sgl { namespace vk {

OpenCLFunctionTable g_openclFunctionTable{};

#ifdef _WIN32
HMODULE g_openclLibraryHandle = nullptr;
#define dlsym GetProcAddress
#else
void* g_openclLibraryHandle = nullptr;
#endif

bool initializeOpenCLFunctionTable() {
    typedef cl_int ( *PFN_clGetPlatformIDs )( cl_uint num_entries, cl_platform_id* platforms, cl_uint* num_platforms );
    typedef cl_int ( *PFN_clGetPlatformInfo )( cl_platform_id platform, cl_platform_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret );
    typedef cl_int ( *PFN_clGetDeviceIDs )( cl_platform_id platform, cl_device_type device_type, cl_uint num_entries, cl_device_id* devices, cl_uint* num_devices );
    typedef cl_int ( *PFN_clGetDeviceInfo )( cl_device_id device, cl_device_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret );
    typedef cl_context ( *PFN_clCreateContext )( const cl_context_properties * properties, cl_uint num_devices, const cl_device_id* devices, void (CL_CALLBACK* pfn_notify)(const char* errinfo, const void* private_info, size_t cb, void* user_data), void* user_data, cl_int* errcode_ret );
    typedef cl_int ( *PFN_clRetainContext )( cl_context context );
    typedef cl_int ( *PFN_clReleaseContext )( cl_context context );
    typedef cl_command_queue ( *PFN_clCreateCommandQueue )( cl_context context, cl_device_id device, cl_command_queue_properties properties, cl_int* errcode_ret );
    typedef cl_int ( *PFN_clRetainCommandQueue )( cl_command_queue command_queue );
    typedef cl_int ( *PFN_clReleaseCommandQueue )( cl_command_queue command_queue );
    typedef cl_int ( *PFN_clGetCommandQueueInfo )( cl_command_queue command_queue, cl_command_queue_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret );
    typedef cl_mem ( *PFN_clCreateBuffer )( cl_context context, cl_mem_flags flags, size_t size, void* host_ptr, cl_int* errcode_ret );
    typedef cl_int ( *PFN_clRetainMemObject )(cl_mem memobj);
    typedef cl_int ( *PFN_clReleaseMemObject )(cl_mem memobj);
    typedef cl_program ( *PFN_clCreateProgramWithSource )( cl_context context, cl_uint count, const char** strings, const size_t* lengths, cl_int *errcode_ret );
    typedef cl_program ( *PFN_clCreateProgramWithBinary )( cl_context context, cl_uint num_devices, const cl_device_id* device_list, const size_t* lengths, const unsigned char** binaries, cl_int* binary_status, cl_int* errcode_ret );
    typedef cl_program ( *PFN_clCreateProgramWithBuiltInKernels )( cl_context context, cl_uint num_devices, const cl_device_id* device_list, const char* kernel_names, cl_int* errcode_ret );
    typedef cl_program ( *PFN_clCreateProgramWithIL )( cl_context context, const void* il, size_t length, cl_int* errcode_ret ); // optional
    typedef cl_int ( *PFN_clRetainProgram )( cl_program program );
    typedef cl_int ( *PFN_clReleaseProgram )( cl_program program );
    typedef cl_int ( *PFN_clBuildProgram )( cl_program program, cl_uint num_devices, const cl_device_id* device_list, const char* options, void (CL_CALLBACK* pfn_notify)(cl_program program, void* user_data), void* user_data );
    typedef cl_int ( *PFN_clCompileProgram )( cl_program program, cl_uint num_devices, const cl_device_id* device_list, const char* options, cl_uint num_input_headers, const cl_program* input_headers, const char** header_include_names, void (CL_CALLBACK* pfn_notify)(cl_program program, void* user_data), void* user_data );
    typedef cl_program ( *PFN_clLinkProgram )( cl_context context, cl_uint num_devices, const cl_device_id* device_list, const char* options, cl_uint num_input_programs, const cl_program* input_programs, void (CL_CALLBACK* pfn_notify)(cl_program program, void* user_data), void* user_data, cl_int* errcode_ret );
    typedef cl_kernel ( *PFN_clCreateKernel )( cl_program program, const char* kernel_name, cl_int* errcode_ret );
    typedef cl_int ( *PFN_clCreateKernelsInProgram )( cl_program program, cl_uint num_kernels, cl_kernel* kernels, cl_uint* num_kernels_ret );
    typedef cl_int ( *PFN_clRetainKernel )( cl_kernel kernel );
    typedef cl_int ( *PFN_clReleaseKernel )( cl_kernel kernel );
    typedef cl_int ( *PFN_clSetKernelArg )( cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void* arg_value );
    typedef cl_int ( *PFN_clFlush )( cl_command_queue command_queue );
    typedef cl_int ( *PFN_clFinish )( cl_command_queue command_queue );
    typedef cl_int ( *PFN_clEnqueueReadBuffer )( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read, size_t offset, size_t size, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event );
    typedef cl_int ( *PFN_clEnqueueReadBufferRect )( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read, const size_t* buffer_offset, const size_t* host_offset, const size_t* region, size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);
    typedef cl_int ( *PFN_clEnqueueWriteBuffer )( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write, size_t offset, size_t size, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event );
    typedef cl_int ( *PFN_clEnqueueWriteBufferRect )( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write, const size_t* buffer_offset, const size_t* host_offset, const size_t* region, size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);
    typedef cl_int ( *PFN_clEnqueueFillBuffer )( cl_command_queue command_queue, cl_mem buffer, const void* pattern, size_t pattern_size, size_t offset, size_t size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event );
    typedef cl_int ( *PFN_clEnqueueCopyBuffer )( cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer, size_t src_offset, size_t dst_offset, size_t size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event );
    typedef cl_int ( *PFN_clEnqueueCopyBufferRect )( cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer, const size_t* src_origin, const size_t* dst_origin, const size_t* region, size_t src_row_pitch, size_t src_slice_pitch, size_t dst_row_pitch, size_t dst_slice_pitch, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);
    typedef cl_int ( *PFN_clEnqueueReadImage )( cl_command_queue command_queue, cl_mem image, cl_bool blocking_read, const size_t* origin, const size_t* region, size_t row_pitch, size_t slice_pitch, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event );
    typedef cl_int ( *PFN_clEnqueueWriteImage )( cl_command_queue command_queue, cl_mem image, cl_bool blocking_write, const size_t* origin, const size_t* region, size_t input_row_pitch, size_t input_slice_pitch, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event );
    typedef cl_int ( *PFN_clEnqueueFillImage )( cl_command_queue command_queue, cl_mem image, const void* fill_color, const size_t* origin, const size_t* region, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event );
    typedef cl_int ( *PFN_clEnqueueCopyImage )( cl_command_queue command_queue, cl_mem src_image, cl_mem dst_image, const size_t* src_origin, const size_t* dst_origin, const size_t* region, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event );
    typedef cl_int ( *PFN_clEnqueueCopyImageToBuffer )( cl_command_queue command_queue, cl_mem src_image, cl_mem dst_buffer, const size_t* src_origin, const size_t* region, size_t dst_offset, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event );
    typedef cl_int ( *PFN_clEnqueueCopyBufferToImage )( cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_image, size_t src_offset, const size_t* dst_origin, const size_t* region, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event );
    typedef void* ( *PFN_clEnqueueMapBuffer )( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_map, cl_map_flags map_flags, size_t offset, size_t size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event, cl_int* errcode_ret );
    typedef void* ( *PFN_clEnqueueMapImage )( cl_command_queue command_queue, cl_mem image, cl_bool blocking_map, cl_map_flags map_flags, const size_t* origin, const size_t* region, size_t* image_row_pitch, size_t* image_slice_pitch, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event, cl_int* errcode_ret );
    typedef cl_int ( *PFN_clEnqueueUnmapMemObject )( cl_command_queue command_queue, cl_mem memobj, void* mapped_ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event );
    typedef cl_int ( *PFN_clEnqueueMigrateMemObjects )( cl_command_queue command_queue, cl_uint num_mem_objects, const cl_mem* mem_objects, cl_mem_migration_flags flags, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event );
    typedef cl_int ( *PFN_clEnqueueNDRangeKernel )( cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim, const size_t* global_work_offset, const size_t* global_work_size, const size_t* local_work_size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event );
#ifdef cl_khr_semaphore
    typedef cl_semaphore_khr ( *PFN_clCreateSemaphoreWithPropertiesKHR )(cl_context context, const cl_semaphore_properties_khr* sema_props, cl_int* errcode_ret);
    typedef cl_int ( *PFN_clEnqueueWaitSemaphoresKHR )(cl_command_queue command_queue, cl_uint num_sema_objects, const cl_semaphore_khr* sema_objects, const cl_semaphore_payload_khr* sema_payload_list, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);
    typedef cl_int ( *PFN_clEnqueueSignalSemaphoresKHR )(cl_command_queue command_queue, cl_uint num_sema_objects, const cl_semaphore_khr* sema_objects, const cl_semaphore_payload_khr* sema_payload_list, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);
    typedef cl_int ( *PFN_clGetSemaphoreInfoKHR )(cl_semaphore_khr sema_object, cl_semaphore_info_khr param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);
    typedef cl_int ( *PFN_clReleaseSemaphoreKHR )(cl_semaphore_khr sema_object);
    typedef cl_int ( *PFN_clRetainSemaphoreKHR )(cl_semaphore_khr sema_object);
#endif
#if defined(CL_VERSION_3_0) && defined(cl_khr_external_memory_opaque_fd)
    typedef cl_mem ( *PFN_clCreateBufferWithProperties )(cl_context context, const cl_mem_properties* properties, cl_mem_flags flags, size_t size, void* host_ptr, cl_int* errcode_ret);
    typedef cl_mem ( *PFN_clCreateImageWithProperties )(cl_context context, const cl_mem_properties* properties, cl_mem_flags flags, const cl_image_format* image_format, const cl_image_desc* image_desc, void* host_ptr, cl_int* errcode_ret);
#endif

#if defined(__linux__)
    g_openclLibraryHandle = dlopen("libOpenCL.so", RTLD_NOW | RTLD_LOCAL);
    if (!g_openclLibraryHandle) {
        sgl::Logfile::get()->writeInfo("initializeOpenCLFunctionTable: Could not load libOpenCL.so.");
        return false;
    }
#elif defined(_WIN32)
    g_openclLibraryHandle = LoadLibraryA("OpenCL.dll");
    if (!g_openclLibraryHandle) {
        sgl::Logfile::get()->writeInfo("initializeOpenCLFunctionTable: Could not load OpenCL.dll.");
        return false;
    }
#elif defined(__APPLE__)
    g_openclLibraryHandle = dlopen("libOpenCL.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!g_openclLibraryHandle) {
        sgl::Logfile::get()->writeInfo("initializeOpenCLFunctionTable: Could not load libOpenCL.dylib.");
        return false;
    }
#endif
    g_openclFunctionTable.clGetPlatformIDs = PFN_clGetPlatformIDs(dlsym(g_openclLibraryHandle, TOSTRING(clGetPlatformIDs)));
    g_openclFunctionTable.clGetPlatformInfo = PFN_clGetPlatformInfo(dlsym(g_openclLibraryHandle, TOSTRING(clGetPlatformInfo)));
    g_openclFunctionTable.clGetDeviceIDs = PFN_clGetDeviceIDs(dlsym(g_openclLibraryHandle, TOSTRING(clGetDeviceIDs)));
    g_openclFunctionTable.clGetDeviceInfo = PFN_clGetDeviceInfo(dlsym(g_openclLibraryHandle, TOSTRING(clGetDeviceInfo)));
    g_openclFunctionTable.clCreateContext = PFN_clCreateContext(dlsym(g_openclLibraryHandle, TOSTRING(clCreateContext)));
    g_openclFunctionTable.clRetainContext = PFN_clRetainContext(dlsym(g_openclLibraryHandle, TOSTRING(clRetainContext)));
    g_openclFunctionTable.clReleaseContext = PFN_clReleaseContext(dlsym(g_openclLibraryHandle, TOSTRING(clReleaseContext)));
    g_openclFunctionTable.clCreateCommandQueue = PFN_clCreateCommandQueue(dlsym(g_openclLibraryHandle, TOSTRING(clCreateCommandQueue)));
    g_openclFunctionTable.clRetainCommandQueue = PFN_clRetainCommandQueue(dlsym(g_openclLibraryHandle, TOSTRING(clRetainCommandQueue)));
    g_openclFunctionTable.clReleaseCommandQueue = PFN_clReleaseCommandQueue(dlsym(g_openclLibraryHandle, TOSTRING(clReleaseCommandQueue)));
    g_openclFunctionTable.clGetCommandQueueInfo = PFN_clGetCommandQueueInfo(dlsym(g_openclLibraryHandle, TOSTRING(clGetCommandQueueInfo)));
    g_openclFunctionTable.clCreateBuffer = PFN_clCreateBuffer(dlsym(g_openclLibraryHandle, TOSTRING(clCreateBuffer)));
    g_openclFunctionTable.clRetainMemObject = PFN_clRetainMemObject(dlsym(g_openclLibraryHandle, TOSTRING(clRetainMemObject)));
    g_openclFunctionTable.clReleaseMemObject = PFN_clReleaseMemObject(dlsym(g_openclLibraryHandle, TOSTRING(clReleaseMemObject)));
    g_openclFunctionTable.clCreateProgramWithSource = PFN_clCreateProgramWithSource(dlsym(g_openclLibraryHandle, TOSTRING(clCreateProgramWithSource)));
    g_openclFunctionTable.clCreateProgramWithBinary = PFN_clCreateProgramWithBinary(dlsym(g_openclLibraryHandle, TOSTRING(clCreateProgramWithBinary)));
    g_openclFunctionTable.clCreateProgramWithBuiltInKernels = PFN_clCreateProgramWithBuiltInKernels(dlsym(g_openclLibraryHandle, TOSTRING(clCreateProgramWithBuiltInKernels)));
    g_openclFunctionTable.clCreateProgramWithIL = PFN_clCreateProgramWithIL(dlsym(g_openclLibraryHandle, TOSTRING(clCreateProgramWithIL)));
    g_openclFunctionTable.clRetainProgram = PFN_clRetainProgram(dlsym(g_openclLibraryHandle, TOSTRING(clRetainProgram)));
    g_openclFunctionTable.clReleaseProgram = PFN_clReleaseProgram(dlsym(g_openclLibraryHandle, TOSTRING(clReleaseProgram)));
    g_openclFunctionTable.clBuildProgram = PFN_clBuildProgram(dlsym(g_openclLibraryHandle, TOSTRING(clBuildProgram)));
    g_openclFunctionTable.clCompileProgram = PFN_clCompileProgram(dlsym(g_openclLibraryHandle, TOSTRING(clCompileProgram)));
    g_openclFunctionTable.clLinkProgram = PFN_clLinkProgram(dlsym(g_openclLibraryHandle, TOSTRING(clLinkProgram)));
    g_openclFunctionTable.clCreateKernel = PFN_clCreateKernel(dlsym(g_openclLibraryHandle, TOSTRING(clCreateKernel)));
    g_openclFunctionTable.clCreateKernelsInProgram = PFN_clCreateKernelsInProgram(dlsym(g_openclLibraryHandle, TOSTRING(clCreateKernelsInProgram)));
    g_openclFunctionTable.clRetainKernel = PFN_clRetainKernel(dlsym(g_openclLibraryHandle, TOSTRING(clRetainKernel)));
    g_openclFunctionTable.clReleaseKernel = PFN_clReleaseKernel(dlsym(g_openclLibraryHandle, TOSTRING(clReleaseKernel)));
    g_openclFunctionTable.clSetKernelArg = PFN_clSetKernelArg(dlsym(g_openclLibraryHandle, TOSTRING(clSetKernelArg)));
    g_openclFunctionTable.clFlush = PFN_clFlush(dlsym(g_openclLibraryHandle, TOSTRING(clFlush)));
    g_openclFunctionTable.clFinish = PFN_clFinish(dlsym(g_openclLibraryHandle, TOSTRING(clFinish)));
    g_openclFunctionTable.clEnqueueReadBuffer = PFN_clEnqueueReadBuffer(dlsym(g_openclLibraryHandle, TOSTRING(clEnqueueReadBuffer)));
    g_openclFunctionTable.clEnqueueReadBufferRect = PFN_clEnqueueReadBufferRect(dlsym(g_openclLibraryHandle, TOSTRING(clEnqueueReadBufferRect)));
    g_openclFunctionTable.clEnqueueWriteBuffer = PFN_clEnqueueWriteBuffer(dlsym(g_openclLibraryHandle, TOSTRING(clEnqueueWriteBuffer)));
    g_openclFunctionTable.clEnqueueWriteBufferRect = PFN_clEnqueueWriteBufferRect(dlsym(g_openclLibraryHandle, TOSTRING(clEnqueueWriteBufferRect)));
    g_openclFunctionTable.clEnqueueFillBuffer = PFN_clEnqueueFillBuffer(dlsym(g_openclLibraryHandle, TOSTRING(clEnqueueFillBuffer)));
    g_openclFunctionTable.clEnqueueCopyBuffer = PFN_clEnqueueCopyBuffer(dlsym(g_openclLibraryHandle, TOSTRING(clEnqueueCopyBuffer)));
    g_openclFunctionTable.clEnqueueCopyBufferRect = PFN_clEnqueueCopyBufferRect(dlsym(g_openclLibraryHandle, TOSTRING(clEnqueueCopyBufferRect)));
    g_openclFunctionTable.clEnqueueReadImage = PFN_clEnqueueReadImage(dlsym(g_openclLibraryHandle, TOSTRING(clEnqueueReadImage)));
    g_openclFunctionTable.clEnqueueWriteImage = PFN_clEnqueueWriteImage(dlsym(g_openclLibraryHandle, TOSTRING(clEnqueueWriteImage)));
    g_openclFunctionTable.clEnqueueFillImage = PFN_clEnqueueFillImage(dlsym(g_openclLibraryHandle, TOSTRING(clEnqueueFillImage)));
    g_openclFunctionTable.clEnqueueCopyImage = PFN_clEnqueueCopyImage(dlsym(g_openclLibraryHandle, TOSTRING(clEnqueueCopyImage)));
    g_openclFunctionTable.clEnqueueCopyImageToBuffer = PFN_clEnqueueCopyImageToBuffer(dlsym(g_openclLibraryHandle, TOSTRING(clEnqueueCopyImageToBuffer)));
    g_openclFunctionTable.clEnqueueCopyBufferToImage = PFN_clEnqueueCopyBufferToImage(dlsym(g_openclLibraryHandle, TOSTRING(clEnqueueCopyBufferToImage)));
    g_openclFunctionTable.clEnqueueMapBuffer = PFN_clEnqueueMapBuffer(dlsym(g_openclLibraryHandle, TOSTRING(clEnqueueMapBuffer)));
    g_openclFunctionTable.clEnqueueMapImage = PFN_clEnqueueMapImage(dlsym(g_openclLibraryHandle, TOSTRING(clEnqueueMapImage)));
    g_openclFunctionTable.clEnqueueUnmapMemObject = PFN_clEnqueueUnmapMemObject(dlsym(g_openclLibraryHandle, TOSTRING(clEnqueueUnmapMemObject)));
    g_openclFunctionTable.clEnqueueMigrateMemObjects = PFN_clEnqueueMigrateMemObjects(dlsym(g_openclLibraryHandle, TOSTRING(clEnqueueMigrateMemObjects)));
    g_openclFunctionTable.clEnqueueNDRangeKernel = PFN_clEnqueueNDRangeKernel(dlsym(g_openclLibraryHandle, TOSTRING(clEnqueueNDRangeKernel)));
#ifdef cl_khr_semaphore
    g_openclFunctionTable.clCreateSemaphoreWithPropertiesKHR = PFN_clCreateSemaphoreWithPropertiesKHR(dlsym(g_openclLibraryHandle, TOSTRING(clCreateSemaphoreWithPropertiesKHR)));
    g_openclFunctionTable.clEnqueueWaitSemaphoresKHR = PFN_clEnqueueWaitSemaphoresKHR(dlsym(g_openclLibraryHandle, TOSTRING(clEnqueueWaitSemaphoresKHR)));
    g_openclFunctionTable.clEnqueueSignalSemaphoresKHR = PFN_clEnqueueSignalSemaphoresKHR(dlsym(g_openclLibraryHandle, TOSTRING(clEnqueueSignalSemaphoresKHR)));
    g_openclFunctionTable.clGetSemaphoreInfoKHR = PFN_clGetSemaphoreInfoKHR(dlsym(g_openclLibraryHandle, TOSTRING(clGetSemaphoreInfoKHR)));
    g_openclFunctionTable.clReleaseSemaphoreKHR = PFN_clReleaseSemaphoreKHR(dlsym(g_openclLibraryHandle, TOSTRING(clReleaseSemaphoreKHR)));
    g_openclFunctionTable.clRetainSemaphoreKHR = PFN_clRetainSemaphoreKHR(dlsym(g_openclLibraryHandle, TOSTRING(clRetainSemaphoreKHR)));
#endif
#if defined(CL_VERSION_3_0) && defined(cl_khr_external_memory_opaque_fd)
    g_openclFunctionTable.clCreateBufferWithProperties = PFN_clCreateBufferWithProperties(dlsym(g_openclLibraryHandle, TOSTRING(clCreateBufferWithProperties)));
    g_openclFunctionTable.clCreateImageWithProperties = PFN_clCreateImageWithProperties(dlsym(g_openclLibraryHandle, TOSTRING(clCreateImageWithProperties)));
#endif

    if (!g_openclFunctionTable.clGetPlatformIDs
            || !g_openclFunctionTable.clGetPlatformInfo
            || !g_openclFunctionTable.clGetDeviceIDs
            || !g_openclFunctionTable.clGetDeviceInfo
            || !g_openclFunctionTable.clCreateContext
            || !g_openclFunctionTable.clRetainContext
            || !g_openclFunctionTable.clReleaseContext
            || !g_openclFunctionTable.clCreateCommandQueue
            || !g_openclFunctionTable.clRetainCommandQueue
            || !g_openclFunctionTable.clReleaseCommandQueue
            || !g_openclFunctionTable.clGetCommandQueueInfo
            || !g_openclFunctionTable.clCreateBuffer
            || !g_openclFunctionTable.clRetainMemObject
            || !g_openclFunctionTable.clReleaseMemObject
            || !g_openclFunctionTable.clCreateProgramWithSource
            || !g_openclFunctionTable.clCreateProgramWithBinary
            || !g_openclFunctionTable.clCreateProgramWithBuiltInKernels
            //|| !g_openclFunctionTable.clCreateProgramWithIL // optional, OpenCL 2.1
            || !g_openclFunctionTable.clRetainProgram
            || !g_openclFunctionTable.clReleaseProgram
            || !g_openclFunctionTable.clBuildProgram
            || !g_openclFunctionTable.clCompileProgram
            || !g_openclFunctionTable.clLinkProgram
            || !g_openclFunctionTable.clCreateKernel
            || !g_openclFunctionTable.clCreateKernelsInProgram
            || !g_openclFunctionTable.clRetainKernel
            || !g_openclFunctionTable.clReleaseKernel
            || !g_openclFunctionTable.clSetKernelArg
            || !g_openclFunctionTable.clFlush
            || !g_openclFunctionTable.clFinish
            || !g_openclFunctionTable.clEnqueueReadBuffer
            || !g_openclFunctionTable.clEnqueueReadBufferRect
            || !g_openclFunctionTable.clEnqueueWriteBuffer
            || !g_openclFunctionTable.clEnqueueWriteBufferRect
            || !g_openclFunctionTable.clEnqueueFillBuffer
            || !g_openclFunctionTable.clEnqueueCopyBuffer
            || !g_openclFunctionTable.clEnqueueCopyBufferRect
            || !g_openclFunctionTable.clEnqueueReadImage
            || !g_openclFunctionTable.clEnqueueWriteImage
            || !g_openclFunctionTable.clEnqueueFillImage
            || !g_openclFunctionTable.clEnqueueCopyImage
            || !g_openclFunctionTable.clEnqueueCopyImageToBuffer
            || !g_openclFunctionTable.clEnqueueCopyBufferToImage
            || !g_openclFunctionTable.clEnqueueMapBuffer
            || !g_openclFunctionTable.clEnqueueMapImage
            || !g_openclFunctionTable.clEnqueueUnmapMemObject
            || !g_openclFunctionTable.clEnqueueMigrateMemObjects
            || !g_openclFunctionTable.clEnqueueNDRangeKernel) {
        sgl::Logfile::get()->throwError(
                "Error in initializeOpenCLFunctionTable: "
                "At least one function pointer could not be loaded.");
    }

    return true;
}

#ifdef _WIN32
#undef dlsym
#endif

bool getIsOpenCLFunctionTableInitialized() {
    return g_openclLibraryHandle != nullptr;
}

void freeOpenCLFunctionTable() {
    if (g_openclLibraryHandle) {
#if defined(__linux__)
        dlclose(g_openclLibraryHandle);
#elif defined(_WIN32)
        FreeLibrary(g_openclLibraryHandle);
#endif
        g_openclLibraryHandle = {};
    }
}

template<>
std::string getOpenCLDeviceInfo<std::string>(cl_device_id device, cl_device_info info) {
    size_t deviceExtensionStringSize = 0;
    cl_int res;
    res = sgl::vk::g_openclFunctionTable.clGetDeviceInfo(device, info, 0, nullptr, &deviceExtensionStringSize);
    sgl::vk::checkResultCL(res, "Error in clGetDeviceInfo: ");
    char* strObj = new char[deviceExtensionStringSize + 1];
    res = sgl::vk::g_openclFunctionTable.clGetDeviceInfo(device, info, deviceExtensionStringSize, strObj, nullptr);
    sgl::vk::checkResultCL(res, "Error in clGetDeviceInfo: ");
    strObj[deviceExtensionStringSize] = '\0';
    return strObj;
}

std::string getOpenCLDeviceInfoString(cl_device_id device, cl_device_info info) {
    size_t deviceExtensionStringSize = 0;
    cl_int res;
    res = sgl::vk::g_openclFunctionTable.clGetDeviceInfo(device, info, 0, nullptr, &deviceExtensionStringSize);
    sgl::vk::checkResultCL(res, "Error in clGetDeviceInfo: ");
    char* strObj = new char[deviceExtensionStringSize + 1];
    res = sgl::vk::g_openclFunctionTable.clGetDeviceInfo(device, info, deviceExtensionStringSize, strObj, nullptr);
    sgl::vk::checkResultCL(res, "Error in clGetDeviceInfo: ");
    strObj[deviceExtensionStringSize] = '\0';
    return strObj;
}

std::unordered_set<std::string> getOpenCLDeviceExtensionsSet(cl_device_id device) {
    std::vector<std::string> extensionsList;
    std::string extensionsString = getOpenCLDeviceInfoString(device, CL_DEVICE_EXTENSIONS);
    splitStringWhitespace(extensionsString, extensionsList);
    return std::unordered_set<std::string>(extensionsList.begin(), extensionsList.end());
}

cl_device_id getMatchingOpenCLDevice(sgl::vk::Device* device) {
    cl_int res;
    cl_uint numPlatforms = 0;
    res = sgl::vk::g_openclFunctionTable.clGetPlatformIDs(0, nullptr, &numPlatforms);
    sgl::vk::checkResultCL(res, "Error in clGetPlatformIDs: ");
    auto* platforms = new cl_platform_id[numPlatforms];
    res = sgl::vk::g_openclFunctionTable.clGetPlatformIDs(numPlatforms, platforms, nullptr);
    sgl::vk::checkResultCL(res, "Error in clGetPlatformIDs: ");

    const VkPhysicalDeviceIDProperties& deviceIdProperties = device->getDeviceIDProperties();
    cl_device_id clDevice = nullptr;
    for (cl_uint platIdx = 0; platIdx < numPlatforms; platIdx++) {
        // Enumerate the devices.
        cl_uint numDevices = 0;
        res = sgl::vk::g_openclFunctionTable.clGetDeviceIDs(
                platforms[platIdx], CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
        if (res == CL_DEVICE_NOT_FOUND || numDevices == 0) {
            continue;
        }
        sgl::vk::checkResultCL(res, "Error in clGetDeviceIDs: ");

        auto* clDevices = new cl_device_id[numDevices];
        res = sgl::vk::g_openclFunctionTable.clGetDeviceIDs(
                platforms[platIdx], CL_DEVICE_TYPE_ALL, numDevices, clDevices, nullptr);
        sgl::vk::checkResultCL(res, "Error in clGetDeviceIDs: ");

        for (cl_uint deviceIdx = 0; deviceIdx < numDevices; deviceIdx++) {
            cl_device_id clCurrDevice = clDevices[deviceIdx];

            std::unordered_set<std::string> deviceExtensions = sgl::vk::getOpenCLDeviceExtensionsSet(
                    clCurrDevice);

#ifdef cl_khr_device_uuid
            if (deviceExtensions.find("cl_khr_device_uuid") != deviceExtensions.end()) {
                uint8_t clUuid[16];
                res = sgl::vk::g_openclFunctionTable.clGetDeviceInfo(
                        clCurrDevice, CL_DEVICE_UUID_KHR, CL_UUID_SIZE_KHR, &clUuid, nullptr);
                sgl::vk::checkResultCL(res, "Error in clGetDeviceInfo[CL_DEVICE_UUID_KHR]: ");

                bool isSameUuid = true;
                for (int i = 0; i < 16; i++) {
                    if (deviceIdProperties.deviceUUID[i] != clUuid[i]) {
                        isSameUuid = false;
                        break;
                    }
                }
                if (isSameUuid) {
                    clDevice = clCurrDevice;
                    break;
                }
            } else {
#endif
                /*
                 * Use heuristics for finding correct device if cl_khr_device_uuid is not supported.
                 * Comparing the device name turned out to be sufficient for an NVIDIA RTX 3090 and the AMD APP SDK.
                 * However, on the Steam Deck, the name of the OpenCL Clover driver uses the code name
                 * "AMD Custom GPU 0405 (vangogh, ...)" compared to the Vulkan device name "AMD RADV VANGOGH".
                 */
                auto deviceNameString = sgl::vk::getOpenCLDeviceInfoString(
                        clCurrDevice, CL_DEVICE_NAME);
                if (deviceNameString == device->getDeviceName()) {
                    clDevice = clCurrDevice;
                    break;
                }

                /*
                 * Make sure that the vendor ID matches. Otherwise, when in the next step checking sub-strings of the
                 * device name, we might get incorrect matches when an APU is used.
                 * E.g., POCL puts the CPU name into the device name, and the APU name might be identical.
                 */
                auto clDeviceVendorId = sgl::vk::getOpenCLDeviceInfo<uint32_t>(
                        clCurrDevice, CL_DEVICE_VENDOR_ID);
                if (clDeviceVendorId != device->getVendorId()) {
                    continue;
                }

                /**
                 * We assume matching devices if at least half of the Vulkan device name can be found in the OpenCL
                 * device name. Example for the Steam Deck: "AMD RADV VANGOGH" has two matches ("amd", "vangogh") in
                 * the OpenCL device name "AMD Custom GPU 0405 (vangogh, ...)".
                 */
                std::string deviceNameStringCl = boost::to_lower_copy(deviceNameString);
                std::string deviceNameStringVk = boost::to_lower_copy(std::string(device->getDeviceName()));
                std::vector<std::string> deviceNameStringVkParts;
                sgl::splitStringWhitespace(deviceNameStringVk, deviceNameStringVkParts);
                int numStringPartsFound = 0;
                for (const std::string& deviceNameStringVkPart : deviceNameStringVkParts) {
                    if (deviceNameStringCl.find(deviceNameStringVkPart) != std::string::npos) {
                        numStringPartsFound++;
                    }
                }
                if (numStringPartsFound >= sgl::iceil(int(deviceNameStringVkParts.size()), 2)) {
                    clDevice = clCurrDevice;
                    break;
                }

                /*
                 * On ROCm, the device name is a codename like "gfx1030", which is different from the "real" device name
                 * (aka. board name). AMD offers an extension to get the board name, matching the name of the Vulkan
                 * device (e.g., "AMD Radeon RX 6900 XT").
                 */
                if (deviceExtensions.find("cl_amd_device_attribute_query") != deviceExtensions.end()) {
                    auto boardNameAmd = sgl::vk::getOpenCLDeviceInfoString(
                            clCurrDevice, CL_DEVICE_BOARD_NAME_AMD);
                    if (boardNameAmd == device->getDeviceName()) {
                        clDevice = clCurrDevice;
                        break;
                    }

                    std::string boardNameStringCl = boost::to_lower_copy(boardNameAmd);
                    numStringPartsFound = 0;
                    for (const std::string& deviceNameStringVkPart : deviceNameStringVkParts) {
                        if (boardNameStringCl.find(deviceNameStringVkPart) != std::string::npos) {
                            numStringPartsFound++;
                        }
                    }
                    if (numStringPartsFound >= sgl::iceil(int(deviceNameStringVkParts.size()), 2)) {
                        clDevice = clCurrDevice;
                        break;
                    }
                }
#ifdef cl_khr_device_uuid
            }
#endif
        }

        delete[] clDevices;
    }

    delete[] platforms;

    return clDevice;
}


static std::map<cl_int, const char*> openclErrorStringMap = {
        { CL_SUCCESS, "CL_SUCCESS" },
        { CL_DEVICE_NOT_FOUND, "CL_DEVICE_NOT_FOUND" },
        { CL_DEVICE_NOT_AVAILABLE, "CL_DEVICE_NOT_AVAILABLE" },
        { CL_COMPILER_NOT_AVAILABLE, "CL_COMPILER_NOT_AVAILABLE" },
        { CL_MEM_OBJECT_ALLOCATION_FAILURE, "CL_MEM_OBJECT_ALLOCATION_FAILURE" },
        { CL_OUT_OF_RESOURCES, "CL_OUT_OF_RESOURCES" },
        { CL_OUT_OF_HOST_MEMORY, "CL_OUT_OF_HOST_MEMORY" },
        { CL_PROFILING_INFO_NOT_AVAILABLE, "CL_PROFILING_INFO_NOT_AVAILABLE" },
        { CL_MEM_COPY_OVERLAP, "CL_MEM_COPY_OVERLAP" },
        { CL_IMAGE_FORMAT_MISMATCH, "CL_IMAGE_FORMAT_MISMATCH" },
        { CL_IMAGE_FORMAT_NOT_SUPPORTED, "CL_IMAGE_FORMAT_NOT_SUPPORTED" },
        { CL_BUILD_PROGRAM_FAILURE, "CL_BUILD_PROGRAM_FAILURE" },
        { CL_MAP_FAILURE, "CL_MAP_FAILURE" },
        { CL_MISALIGNED_SUB_BUFFER_OFFSET, "CL_MISALIGNED_SUB_BUFFER_OFFSET" },
        { CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST, "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST" },
        { CL_COMPILE_PROGRAM_FAILURE, "CL_COMPILE_PROGRAM_FAILURE" },
        { CL_LINKER_NOT_AVAILABLE, "CL_LINKER_NOT_AVAILABLE" },
        { CL_LINK_PROGRAM_FAILURE, "CL_LINK_PROGRAM_FAILURE" },
        { CL_DEVICE_PARTITION_FAILED, "CL_DEVICE_PARTITION_FAILED" },
        { CL_KERNEL_ARG_INFO_NOT_AVAILABLE, "CL_KERNEL_ARG_INFO_NOT_AVAILABLE" },
        { CL_INVALID_VALUE, "CL_INVALID_VALUE" },
        { CL_INVALID_DEVICE_TYPE, "CL_INVALID_DEVICE_TYPE" },
        { CL_INVALID_PLATFORM, "CL_INVALID_PLATFORM" },
        { CL_INVALID_DEVICE, "CL_INVALID_DEVICE" },
        { CL_INVALID_CONTEXT, "CL_INVALID_CONTEXT" },
        { CL_INVALID_QUEUE_PROPERTIES, "CL_INVALID_QUEUE_PROPERTIES" },
        { CL_INVALID_COMMAND_QUEUE, "CL_INVALID_COMMAND_QUEUE" },
        { CL_INVALID_HOST_PTR, "CL_INVALID_HOST_PTR" },
        { CL_INVALID_MEM_OBJECT, "CL_INVALID_MEM_OBJECT" },
        { CL_INVALID_IMAGE_FORMAT_DESCRIPTOR, "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR" },
        { CL_INVALID_IMAGE_SIZE, "CL_INVALID_IMAGE_SIZE" },
        { CL_INVALID_SAMPLER, "CL_INVALID_SAMPLER" },
        { CL_INVALID_BINARY, "CL_INVALID_BINARY" },
        { CL_INVALID_BUILD_OPTIONS, "CL_INVALID_BUILD_OPTIONS" },
        { CL_INVALID_PROGRAM, "CL_INVALID_PROGRAM" },
        { CL_INVALID_PROGRAM_EXECUTABLE, "CL_INVALID_PROGRAM_EXECUTABLE" },
        { CL_INVALID_KERNEL_NAME, "CL_INVALID_KERNEL_NAME" },
        { CL_INVALID_KERNEL_DEFINITION, "CL_INVALID_KERNEL_DEFINITION" },
        { CL_INVALID_KERNEL, "CL_INVALID_KERNEL" },
        { CL_INVALID_ARG_INDEX, "CL_INVALID_ARG_INDEX" },
        { CL_INVALID_ARG_VALUE, "CL_INVALID_ARG_VALUE" },
        { CL_INVALID_ARG_SIZE, "CL_INVALID_ARG_SIZE" },
        { CL_INVALID_KERNEL_ARGS, "CL_INVALID_KERNEL_ARGS" },
        { CL_INVALID_WORK_DIMENSION, "CL_INVALID_WORK_DIMENSION" },
        { CL_INVALID_WORK_GROUP_SIZE, "CL_INVALID_WORK_GROUP_SIZE" },
        { CL_INVALID_WORK_ITEM_SIZE, "CL_INVALID_WORK_ITEM_SIZE" },
        { CL_INVALID_GLOBAL_OFFSET, "CL_INVALID_GLOBAL_OFFSET" },
        { CL_INVALID_EVENT_WAIT_LIST, "CL_INVALID_EVENT_WAIT_LIST" },
        { CL_INVALID_EVENT, "CL_INVALID_EVENT" },
        { CL_INVALID_OPERATION, "CL_INVALID_OPERATION" },
        { CL_INVALID_GL_OBJECT, "CL_INVALID_GL_OBJECT" },
        { CL_INVALID_BUFFER_SIZE, "CL_INVALID_BUFFER_SIZE" },
        { CL_INVALID_MIP_LEVEL, "CL_INVALID_MIP_LEVEL" },
        { CL_INVALID_GLOBAL_WORK_SIZE, "CL_INVALID_GLOBAL_WORK_SIZE" },
        { CL_INVALID_PROPERTY, "CL_INVALID_PROPERTY" },
        { CL_INVALID_IMAGE_DESCRIPTOR, "CL_INVALID_IMAGE_DESCRIPTOR" },
        { CL_INVALID_COMPILER_OPTIONS, "CL_INVALID_COMPILER_OPTIONS" },
        { CL_INVALID_LINKER_OPTIONS, "CL_INVALID_LINKER_OPTIONS" },
        { CL_INVALID_DEVICE_PARTITION_COUNT, "CL_INVALID_DEVICE_PARTITION_COUNT" },
#ifdef CL_VERSION_2_0
        { CL_INVALID_PIPE_SIZE, "CL_INVALID_PIPE_SIZE" },
        { CL_INVALID_DEVICE_QUEUE, "CL_INVALID_DEVICE_QUEUE" },
#endif
#ifdef CL_VERSION_2_2
        { CL_INVALID_SPEC_ID, "CL_INVALID_SPEC_ID" },
        { CL_MAX_SIZE_RESTRICTION_EXCEEDED, "CL_MAX_SIZE_RESTRICTION_EXCEEDED" },
#endif
};

void _checkResultCL(cl_int res, const char* text, const char* locationText) {
    if (res != CL_SUCCESS) {
        auto it = openclErrorStringMap.find(res);
        if (it != openclErrorStringMap.end()) {
            sgl::Logfile::get()->throwError(std::string() + locationText + ": " + text + it->second);
        } else {
            sgl::Logfile::get()->throwError(std::string() + locationText + ": " + text + "Unknown error type.");
        }
    }
}



#ifdef cl_khr_semaphore
SemaphoreVkOpenCLInterop::SemaphoreVkOpenCLInterop(
        sgl::vk::Device* device, cl_context context, VkSemaphoreCreateFlags semaphoreCreateFlags,
        VkSemaphoreType semaphoreType, uint64_t timelineSemaphoreInitialValue) {
    VkExportSemaphoreCreateInfo exportSemaphoreCreateInfo = {};
    exportSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
#if defined(_WIN32)
    exportSemaphoreCreateInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#elif defined(__linux__)
    exportSemaphoreCreateInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
#else
    Logfile::get()->throwError(
                "Error in SemaphoreVkOpenCLInterop::SemaphoreVkOpenCLInterop: "
                "External semaphores are only supported on Linux, Android and Windows systems!");
#endif
    _initialize(
            device, semaphoreCreateFlags, semaphoreType, timelineSemaphoreInitialValue,
            &exportSemaphoreCreateInfo);


    cl_semaphore_properties_khr semaphoreHandleType;
    cl_semaphore_properties_khr semaphoreHandle;
#if defined(_WIN32)
    auto _vkGetSemaphoreWin32HandleKHR = (PFN_vkGetSemaphoreWin32HandleKHR)vkGetDeviceProcAddr(
                device->getVkDevice(), "vkGetSemaphoreWin32HandleKHR");
    if (!_vkGetSemaphoreWin32HandleKHR) {
        Logfile::get()->throwError(
                "Error in SemaphoreVkOpenCLInterop::SemaphoreVkOpenCLInterop: "
                "vkGetSemaphoreWin32HandleKHR was not found!");
    }

    VkSemaphoreGetWin32HandleInfoKHR semaphoreGetWin32HandleInfo = {};
    semaphoreGetWin32HandleInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
    semaphoreGetWin32HandleInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
    semaphoreGetWin32HandleInfo.semaphore = semaphoreVk;
    handle = nullptr;
    if (_vkGetSemaphoreWin32HandleKHR(
            device->getVkDevice(), &semaphoreGetWin32HandleInfo, &handle) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in SemaphoreVkOpenCLInterop::SemaphoreVkOpenCLInterop: "
                "vkGetSemaphoreFdKHR failed!");
    }

    if (isTimelineSemaphore()) {
        sgl::Logfile::get()->throwError(
                "Error in SemaphoreVkOpenCLInterop::SemaphoreVkOpenCLInterop: Timeline semaphores "
                "are not yet supported by OpenCL.");
    }
    semaphoreHandleType = CL_SEMAPHORE_HANDLE_OPAQUE_WIN32_KHR;
    semaphoreHandle = cl_semaphore_properties_khr(handle);
#elif defined(__linux__)
    auto _vkGetSemaphoreFdKHR = (PFN_vkGetSemaphoreFdKHR)vkGetDeviceProcAddr(
            device->getVkDevice(), "vkGetSemaphoreFdKHR");
    if (!_vkGetSemaphoreFdKHR) {
        Logfile::get()->throwError(
                "Error in SemaphoreVkOpenCLInterop::SemaphoreVkOpenCLInterop: "
                "vkGetSemaphoreFdKHR was not found!");
    }

    VkSemaphoreGetFdInfoKHR semaphoreGetFdInfo = {};
    semaphoreGetFdInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
    semaphoreGetFdInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
    semaphoreGetFdInfo.semaphore = semaphoreVk;
    fileDescriptor = 0;
    if (_vkGetSemaphoreFdKHR(device->getVkDevice(), &semaphoreGetFdInfo, &fileDescriptor) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in SemaphoreVkOpenCLInterop::SemaphoreVkOpenCLInterop: "
                "vkGetSemaphoreFdKHR failed!");
    }

    if (isTimelineSemaphore()) {
        sgl::Logfile::get()->throwError(
                "Error in SemaphoreVkOpenCLInterop::SemaphoreVkOpenCLInterop: Timeline semaphores "
                "are not yet supported by OpenCL.");
    }
    semaphoreHandleType = CL_SEMAPHORE_HANDLE_OPAQUE_FD_KHR;
    semaphoreHandle = fileDescriptor;
#else
    sgl::Logfile::get()->throwError(
                "Error in SemaphoreVkOpenCLInterop::SemaphoreVkOpenCLInterop: Vulkan-OpenCL interop "
                "is currently only supported on Linux.");
#endif

    cl_semaphore_properties_khr semaphoreProperties[] =
            {cl_semaphore_properties_khr(CL_SEMAPHORE_TYPE_KHR),
             cl_semaphore_properties_khr(CL_SEMAPHORE_TYPE_BINARY_KHR),
             semaphoreHandleType, semaphoreHandle, 0};
    cl_int errorCode = VK_SUCCESS;
    clSemaphore = g_openclFunctionTable.clCreateSemaphoreWithPropertiesKHR(
            context, semaphoreProperties, &errorCode);
    sgl::vk::checkResultCL(errorCode, "Error in clCreateSemaphoreWithPropertiesKHR: ");

    /*
     * Assume ownership is transferred just like for CUDA. TODO: validate.
     */
#if defined(__linux__)
    fileDescriptor = -1;
#endif
}

SemaphoreVkOpenCLInterop::~SemaphoreVkOpenCLInterop() {
    cl_int res = g_openclFunctionTable.clReleaseSemaphoreKHR(clSemaphore);
    sgl::vk::checkResultCL(res, "Error in clReleaseSemaphoreKHR: ");
}

void SemaphoreVkOpenCLInterop::enqueueSignalSemaphoreCL(cl_command_queue commandQueueCL) {
    cl_int res = g_openclFunctionTable.clEnqueueSignalSemaphoresKHR(
            commandQueueCL, 1, &clSemaphore, nullptr,
            0, nullptr, nullptr);
    sgl::vk::checkResultCL(res, "Error in clEnqueueSignalSemaphoresKHR: ");
}

void SemaphoreVkOpenCLInterop::enqueueWaitSemaphoreCL(cl_command_queue commandQueueCL) {
    cl_int res = g_openclFunctionTable.clEnqueueWaitSemaphoresKHR(
            commandQueueCL, 1, &clSemaphore, nullptr,
            0, nullptr, nullptr);
    sgl::vk::checkResultCL(res, "Error in clEnqueueWaitSemaphoresKHR: ");
}
#endif


#if defined(CL_VERSION_3_0) && defined(cl_khr_external_memory_opaque_fd)
BufferOpenCLExternalMemoryVk::BufferOpenCLExternalMemoryVk(cl_context context, vk::BufferPtr& vulkanBuffer)
        : vulkanBuffer(vulkanBuffer) {
    VkDevice device = vulkanBuffer->getDevice()->getVkDevice();
    VkDeviceMemory deviceMemory = vulkanBuffer->getVkDeviceMemory();

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(device, vulkanBuffer->getVkBuffer(), &memoryRequirements);

    cl_mem_properties memoryHandleType;
    cl_mem_properties memoryHandle;
#if defined(_WIN32)
    auto _vkGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)vkGetDeviceProcAddr(
            device, "vkGetMemoryWin32HandleKHR");
    if (!_vkGetMemoryWin32HandleKHR) {
        Logfile::get()->throwError(
                "Error in Buffer::createGlMemoryObject: vkGetMemoryWin32HandleKHR was not found!");
        return;
    }
    VkMemoryGetWin32HandleInfoKHR memoryGetWin32HandleInfo = {};
    memoryGetWin32HandleInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    memoryGetWin32HandleInfo.memory = deviceMemory;
    memoryGetWin32HandleInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    HANDLE handle = nullptr;
    if (_vkGetMemoryWin32HandleKHR(device, &memoryGetWin32HandleInfo, &handle) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in BufferOpenCLExternalMemoryVk::BufferOpenCLExternalMemoryVk: "
                "Could not retrieve the file descriptor from the Vulkan device memory!");
        return;
    }

    memoryHandleType = CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KHR;
    memoryHandle = cl_mem_properties(handle);
    this->handle = handle;
#elif defined(__linux__)
    auto _vkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(device, "vkGetMemoryFdKHR");
    if (!_vkGetMemoryFdKHR) {
        Logfile::get()->throwError(
                "Error in BufferOpenCLExternalMemoryVk::BufferOpenCLExternalMemoryVk: "
                "vkGetMemoryFdKHR was not found!");
        return;
    }

    VkMemoryGetFdInfoKHR memoryGetFdInfoKhr = {};
    memoryGetFdInfoKhr.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
    memoryGetFdInfoKhr.memory = deviceMemory;
    memoryGetFdInfoKhr.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

    int fileDescriptor = 0;
    if (_vkGetMemoryFdKHR(device, &memoryGetFdInfoKhr, &fileDescriptor) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in BufferOpenCLExternalMemoryVk::BufferOpenCLExternalMemoryVk: "
                "Could not retrieve the file descriptor from the Vulkan device memory!");
        return;
    }

    memoryHandleType = CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR;
    memoryHandle = cl_mem_properties(fileDescriptor);
    this->fileDescriptor = fileDescriptor;
#else
    Logfile::get()->throwError(
            "Error in BufferOpenCLExternalMemoryVk::BufferOpenCLExternalMemoryVk: "
            "External memory is only supported on Linux, Android and Windows systems!");
    return false;
#endif

    cl_mem_properties memoryProperties[] = { memoryHandleType, memoryHandle, 0 };
    cl_int errorCode = VK_SUCCESS;
    extMemoryBuffer = g_openclFunctionTable.clCreateBufferWithProperties(
            context, memoryProperties, 0, vulkanBuffer->getSizeInBytes(), nullptr, &errorCode);
    sgl::vk::checkResultCL(errorCode, "Error in clCreateBufferWithProperties: ");

    /*
     * Assume ownership is transferred just like for CUDA. TODO: validate.
     */
#if defined(__linux__)
    this->fileDescriptor = -1;
#endif
}

BufferOpenCLExternalMemoryVk::~BufferOpenCLExternalMemoryVk() {
#ifdef _WIN32
    CloseHandle(handle);
#else
    if (fileDescriptor != -1) {
        close(fileDescriptor);
        fileDescriptor = -1;
    }
#endif
    if (extMemoryBuffer) {
        cl_int res = g_openclFunctionTable.clReleaseMemObject(extMemoryBuffer);
        sgl::vk::checkResultCL(res, "Error in clReleaseMemObject: ");
    }
}
#endif


#if defined(CL_VERSION_3_0) && defined(cl_khr_external_memory_opaque_fd)
std::map<VkFormat, cl_channel_order> vulkanToCLChannelOrderMap = {
        { VK_FORMAT_R8_UNORM, CL_R },
        { VK_FORMAT_R8_SNORM, CL_R },
        { VK_FORMAT_R8_UINT, CL_R },
        { VK_FORMAT_R8_SINT, CL_R },
        { VK_FORMAT_R8_SRGB, CL_R },

        { VK_FORMAT_R8G8_UNORM, CL_RG },
        { VK_FORMAT_R8G8_SNORM, CL_RG },
        { VK_FORMAT_R8G8_UINT, CL_RG },
        { VK_FORMAT_R8G8_SINT, CL_RG },
        { VK_FORMAT_R8G8_SRGB, CL_RG },

        { VK_FORMAT_R8G8B8_UNORM, CL_RGB },
        { VK_FORMAT_R8G8B8_SNORM, CL_RGB },
        { VK_FORMAT_R8G8B8_UINT, CL_RGB },
        { VK_FORMAT_R8G8B8_SINT, CL_RGB },
        { VK_FORMAT_R8G8B8_SRGB, CL_sRGB },

        { VK_FORMAT_R8G8B8A8_UNORM, CL_RGBA },
        { VK_FORMAT_R8G8B8A8_SNORM, CL_RGBA },
        { VK_FORMAT_R8G8B8A8_UINT, CL_RGBA },
        { VK_FORMAT_R8G8B8A8_SINT, CL_RGBA },
        { VK_FORMAT_R8G8B8A8_SRGB, CL_sRGBA },

        { VK_FORMAT_R16_UNORM, CL_R },
        { VK_FORMAT_R16_SNORM, CL_R },
        { VK_FORMAT_R16_UINT, CL_R },
        { VK_FORMAT_R16_SINT, CL_R },
        { VK_FORMAT_R16_SFLOAT, CL_R },

        { VK_FORMAT_R16G16_UNORM, CL_RG },
        { VK_FORMAT_R16G16_SNORM, CL_RG },
        { VK_FORMAT_R16G16_UINT, CL_RG },
        { VK_FORMAT_R16G16_SINT, CL_RG },
        { VK_FORMAT_R16G16_SFLOAT, CL_RG },

        { VK_FORMAT_R16G16B16_UNORM, CL_RGB },
        { VK_FORMAT_R16G16B16_SNORM, CL_RGB },
        { VK_FORMAT_R16G16B16_UINT, CL_RGB },
        { VK_FORMAT_R16G16B16_SINT, CL_RGB },
        { VK_FORMAT_R16G16B16_SFLOAT, CL_RGB },

        { VK_FORMAT_R16G16B16A16_UNORM, CL_RGBA },
        { VK_FORMAT_R16G16B16A16_SNORM, CL_RGBA },
        { VK_FORMAT_R16G16B16A16_UINT, CL_RGBA },
        { VK_FORMAT_R16G16B16A16_SINT, CL_RGBA },
        { VK_FORMAT_R16G16B16A16_SFLOAT, CL_RGBA },

        { VK_FORMAT_R32_UINT, CL_R },
        { VK_FORMAT_R32_SINT, CL_R },
        { VK_FORMAT_R32_SFLOAT, CL_R },

        { VK_FORMAT_R32G32_UINT, CL_RG },
        { VK_FORMAT_R32G32_SINT, CL_RG },
        { VK_FORMAT_R32G32_SFLOAT, CL_RG },

        { VK_FORMAT_R32G32B32_UINT, CL_RGB },
        { VK_FORMAT_R32G32B32_SINT, CL_RGB },
        { VK_FORMAT_R32G32B32_SFLOAT, CL_RGB },

        { VK_FORMAT_R32G32B32A32_UINT, CL_RGBA },
        { VK_FORMAT_R32G32B32A32_SINT, CL_RGBA },
        { VK_FORMAT_R32G32B32A32_SFLOAT, CL_RGBA },

        { VK_FORMAT_D16_UNORM, CL_DEPTH },
        { VK_FORMAT_X8_D24_UNORM_PACK32, CL_DEPTH_STENCIL },
        { VK_FORMAT_D32_SFLOAT, CL_DEPTH },
        { VK_FORMAT_D16_UNORM_S8_UINT, CL_DEPTH_STENCIL },
        { VK_FORMAT_D24_UNORM_S8_UINT, CL_DEPTH_STENCIL },
        { VK_FORMAT_D32_SFLOAT_S8_UINT, CL_DEPTH_STENCIL },
};
std::map<VkFormat, cl_channel_order> vulkanToCLChannelDataTypeMap = {
        { VK_FORMAT_R8_UNORM, CL_UNORM_INT8 },
        { VK_FORMAT_R8_SNORM, CL_SNORM_INT8 },
        { VK_FORMAT_R8_UINT, CL_UNSIGNED_INT8 },
        { VK_FORMAT_R8_SINT, CL_SIGNED_INT8 },
        { VK_FORMAT_R8_SRGB, CL_UNORM_INT8 },

        { VK_FORMAT_R8G8_UNORM, CL_UNORM_INT8 },
        { VK_FORMAT_R8G8_SNORM, CL_SNORM_INT8 },
        { VK_FORMAT_R8G8_UINT, CL_UNSIGNED_INT8 },
        { VK_FORMAT_R8G8_SINT, CL_SIGNED_INT8 },
        { VK_FORMAT_R8G8_SRGB, CL_UNORM_INT8 },

        { VK_FORMAT_R8G8B8_UNORM, CL_UNORM_INT8 },
        { VK_FORMAT_R8G8B8_SNORM, CL_SNORM_INT8 },
        { VK_FORMAT_R8G8B8_UINT, CL_UNSIGNED_INT8 },
        { VK_FORMAT_R8G8B8_SINT, CL_SIGNED_INT8 },
        { VK_FORMAT_R8G8B8_SRGB, CL_UNORM_INT8 },

        { VK_FORMAT_R8G8B8A8_UNORM, CL_UNORM_INT8 },
        { VK_FORMAT_R8G8B8A8_SNORM, CL_SNORM_INT8 },
        { VK_FORMAT_R8G8B8A8_UINT, CL_UNSIGNED_INT8 },
        { VK_FORMAT_R8G8B8A8_SINT, CL_SIGNED_INT8 },
        { VK_FORMAT_R8G8B8A8_SRGB, CL_UNORM_INT8 },

        { VK_FORMAT_R16_UNORM, CL_UNORM_INT16 },
        { VK_FORMAT_R16_SNORM, CL_SNORM_INT16 },
        { VK_FORMAT_R16_UINT, CL_UNSIGNED_INT16 },
        { VK_FORMAT_R16_SINT, CL_SIGNED_INT16 },
        { VK_FORMAT_R16_SFLOAT, CL_HALF_FLOAT },

        { VK_FORMAT_R16G16_UNORM, CL_UNORM_INT16 },
        { VK_FORMAT_R16G16_SNORM, CL_SNORM_INT16 },
        { VK_FORMAT_R16G16_UINT, CL_UNSIGNED_INT16 },
        { VK_FORMAT_R16G16_SINT, CL_SIGNED_INT16 },
        { VK_FORMAT_R16G16_SFLOAT, CL_HALF_FLOAT },

        { VK_FORMAT_R16G16B16_UNORM, CL_UNORM_INT16 },
        { VK_FORMAT_R16G16B16_SNORM, CL_SNORM_INT16 },
        { VK_FORMAT_R16G16B16_UINT, CL_UNSIGNED_INT16 },
        { VK_FORMAT_R16G16B16_SINT, CL_SIGNED_INT16 },
        { VK_FORMAT_R16G16B16_SFLOAT, CL_HALF_FLOAT },

        { VK_FORMAT_R16G16B16A16_UNORM, CL_UNORM_INT16 },
        { VK_FORMAT_R16G16B16A16_SNORM, CL_SNORM_INT16 },
        { VK_FORMAT_R16G16B16A16_UINT, CL_UNSIGNED_INT16 },
        { VK_FORMAT_R16G16B16A16_SINT, CL_SIGNED_INT16 },
        { VK_FORMAT_R16G16B16A16_SFLOAT, CL_HALF_FLOAT },

        { VK_FORMAT_R32_UINT, CL_UNSIGNED_INT32 },
        { VK_FORMAT_R32_SINT, CL_SIGNED_INT32 },
        { VK_FORMAT_R32_SFLOAT, CL_FLOAT },

        { VK_FORMAT_R32G32_UINT, CL_UNSIGNED_INT32 },
        { VK_FORMAT_R32G32_SINT, CL_SIGNED_INT32 },
        { VK_FORMAT_R32G32_SFLOAT, CL_FLOAT },

        { VK_FORMAT_R32G32B32_UINT, CL_UNSIGNED_INT32 },
        { VK_FORMAT_R32G32B32_SINT, CL_SIGNED_INT32 },
        { VK_FORMAT_R32G32B32_SFLOAT, CL_FLOAT },

        { VK_FORMAT_R32G32B32A32_UINT, CL_UNSIGNED_INT32 },
        { VK_FORMAT_R32G32B32A32_SINT, CL_SIGNED_INT32 },
        { VK_FORMAT_R32G32B32A32_SFLOAT, CL_FLOAT },

        { VK_FORMAT_D16_UNORM, CL_UNORM_INT16 },
        //{ VK_FORMAT_X8_D24_UNORM_PACK32, GL_DEPTH24_STENCIL8 },
        { VK_FORMAT_D32_SFLOAT, CL_FLOAT },
        //{ VK_FORMAT_D16_UNORM_S8_UINT, UNSUPPORTED },
        //{ VK_FORMAT_D24_UNORM_S8_UINT, GL_DEPTH24_STENCIL8 },
        //{ VK_FORMAT_D32_SFLOAT_S8_UINT, GL_DEPTH32F_STENCIL8 },
};

ImageOpenCLExternalMemoryVk::ImageOpenCLExternalMemoryVk(cl_context context, vk::ImagePtr& vulkanImage)
        : vulkanImage(vulkanImage) {
    if (!vulkanImage->getImageSettings().exportMemory) {
        Logfile::get()->throwError(
                "Error in ImageOpenCLExternalMemoryVk::ImageOpenCLExternalMemoryVk: An external memory object can "
                "only be created if the export memory flag was set on creation!");
        return;
    }

    VkDevice device = vulkanImage->getDevice()->getVkDevice();
    VkDeviceMemory deviceMemory = vulkanImage->getVkDeviceMemory();

    cl_mem_properties memoryHandleType;
    cl_mem_properties memoryHandle;
#if defined(_WIN32)
    auto _vkGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)vkGetDeviceProcAddr(
            device, "vkGetMemoryWin32HandleKHR");
    if (!_vkGetMemoryWin32HandleKHR) {
        Logfile::get()->throwError(
                "Error in Buffer::createGlMemoryObject: vkGetMemoryWin32HandleKHR was not found!");
        return;
    }
    VkMemoryGetWin32HandleInfoKHR memoryGetWin32HandleInfo = {};
    memoryGetWin32HandleInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    memoryGetWin32HandleInfo.memory = deviceMemory;
    memoryGetWin32HandleInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    HANDLE handle = nullptr;
    if (_vkGetMemoryWin32HandleKHR(device, &memoryGetWin32HandleInfo, &handle) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in ImageOpenCLExternalMemoryVk::ImageOpenCLExternalMemoryVk: "
                "Could not retrieve the file descriptor from the Vulkan device memory!");
        return;
    }

    memoryHandleType = CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KHR;
    memoryHandle = cl_mem_properties(handle);
    this->handle = handle;
#elif defined(__linux__)
    auto _vkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(device, "vkGetMemoryFdKHR");
    if (!_vkGetMemoryFdKHR) {
        Logfile::get()->throwError(
                "Error in ImageOpenCLExternalMemoryVk::ImageOpenCLExternalMemoryVk: "
                "vkGetMemoryFdKHR was not found!");
        return;
    }

    VkMemoryGetFdInfoKHR memoryGetFdInfoKhr = {};
    memoryGetFdInfoKhr.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
    memoryGetFdInfoKhr.memory = deviceMemory;
    memoryGetFdInfoKhr.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

    int fileDescriptor = 0;
    if (_vkGetMemoryFdKHR(device, &memoryGetFdInfoKhr, &fileDescriptor) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in ImageOpenCLExternalMemoryVk::ImageOpenCLExternalMemoryVk: "
                "Could not retrieve the file descriptor from the Vulkan device memory!");
        return;
    }

    memoryHandleType = CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR;
    memoryHandle = cl_mem_properties(fileDescriptor);
    this->fileDescriptor = fileDescriptor;
#else
    Logfile::get()->throwError(
            "Error in ImageOpenCLExternalMemoryVk::ImageOpenCLExternalMemoryVk: "
            "External memory is only supported on Linux, Android and Windows systems!");
    return false;
#endif

    const sgl::vk::ImageSettings& imageSettings = vulkanImage->getImageSettings();
    auto itChannelOrder = vulkanToCLChannelOrderMap.find(imageSettings.format);
    if (itChannelOrder == vulkanToCLChannelOrderMap.end()) {
        sgl::Logfile::get()->throwError(
                "Error in ImageOpenCLExternalMemoryVk::ImageOpenCLExternalMemoryVk: Unsupported format for "
                "channel order.");
    }
    auto itChannelDataType = vulkanToCLChannelDataTypeMap.find(imageSettings.format);
    if (itChannelDataType == vulkanToCLChannelDataTypeMap.end()) {
        sgl::Logfile::get()->throwError(
                "Error in ImageOpenCLExternalMemoryVk::ImageOpenCLExternalMemoryVk: Unsupported format for "
                "channel data type.");
    }

    cl_image_format clImageFormat{};
    clImageFormat.image_channel_order = itChannelOrder->second;
    clImageFormat.image_channel_data_type = itChannelDataType->second;

    cl_image_desc clImageDesc{};
    clImageDesc.image_width = size_t(imageSettings.width);
    clImageDesc.image_height = size_t(imageSettings.height);
    clImageDesc.image_depth = size_t(imageSettings.depth);

    if (imageSettings.imageType == VK_IMAGE_TYPE_1D) {
        if (imageSettings.arrayLayers > 1) {
            clImageDesc.image_type = CL_MEM_OBJECT_IMAGE1D;
        } else {
            clImageDesc.image_type = CL_MEM_OBJECT_IMAGE1D_ARRAY;
        }
    } else if (imageSettings.imageType == VK_IMAGE_TYPE_2D) {
        if (imageSettings.arrayLayers > 1) {
            clImageDesc.image_type = CL_MEM_OBJECT_IMAGE2D;
        } else {
            clImageDesc.image_type = CL_MEM_OBJECT_IMAGE2D_ARRAY;
        }
    } else {
        clImageDesc.image_type = CL_MEM_OBJECT_IMAGE3D;
    }
    clImageDesc.image_array_size = imageSettings.arrayLayers;
    if (imageSettings.mipLevels > 1) {
        sgl::Logfile::get()->throwError(
                "Error in ImageOpenCLExternalMemoryVk::ImageOpenCLExternalMemoryVk: OpenCL does not support "
                "mipmapping.");
    }
    if (imageSettings.numSamples != VK_SAMPLE_COUNT_1_BIT) {
        sgl::Logfile::get()->throwError(
                "Error in ImageOpenCLExternalMemoryVk::ImageOpenCLExternalMemoryVk: OpenCL does not support "
                "multisampling.");
    }

    cl_mem_properties memoryProperties[] = { memoryHandleType, memoryHandle, 0 };
    cl_int errorCode = VK_SUCCESS;
    extMemoryBuffer = g_openclFunctionTable.clCreateImageWithProperties(
            context, memoryProperties, 0, &clImageFormat, &clImageDesc, nullptr, &errorCode);
    sgl::vk::checkResultCL(errorCode, "Error in clCreateImageWithProperties: ");

    /*
     * Assume ownership is transferred just like for CUDA. TODO: validate.
     */
#if defined(__linux__)
    this->fileDescriptor = -1;
#endif
}

ImageOpenCLExternalMemoryVk::~ImageOpenCLExternalMemoryVk() {
#ifdef _WIN32
    if (handle != nullptr) {
        CloseHandle(handle);
        handle = nullptr;
    }
#else
    if (fileDescriptor != -1) {
        close(fileDescriptor);
        fileDescriptor = -1;
    }
#endif
    if (extMemoryBuffer) {
        cl_int res = g_openclFunctionTable.clReleaseMemObject(extMemoryBuffer);
        sgl::vk::checkResultCL(res, "Error in clReleaseMemObject: ");
    }
}
#endif

}}
