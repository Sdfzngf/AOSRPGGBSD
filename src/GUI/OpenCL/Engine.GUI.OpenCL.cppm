/**
 * @brief GUI模块-OpenCL渲染、计算（已弃用）
 *
 */
module;

#include <CL/cl.h>
#include <CL/opencl.hpp>
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

export module Engine.GUI.OpenCL;

import Engine.Utils.Logger;
import Engine.Utils.Logger.LogLevel;
import Engine.i18n;

using Engine::i18n::fmt;
using Engine::Utils::Logger::Log;
using Engine::Utils::Logger::LogLevel;

export namespace Engine::GUI::OpenCL {
const int numElements = 32;
bool _init { false };
std::vector<cl::Platform> _clPlatforms { };
cl::Platform _currentPlatform;

auto OpenCLEnvInit() -> char
{
    if (!_init) {
        cl::Platform::get(&_clPlatforms);

        cl::Platform plat;
        int index = 0;
        for (auto& p : _clPlatforms) {
            std::string platver = p.getInfo<CL_PLATFORM_VERSION>();
            if (platver.find("OpenCL 2.") != std::string::npos || platver.find("OpenCL 3.") != std::string::npos) {
                std::vector<cl::Device> devices;
                p.getDevices(CL_DEVICE_TYPE_ALL, &devices);
                if (!devices.empty()) {
                    plat = p;
                    break;
                }
            }
            index++;
        }

        if (plat() == nullptr) {
            Log(fmt("未找到支持 OpenCL 2.0 或较新版本的设备平台"), LogLevel::ERROR);
            return -1;
        }

        _currentPlatform = cl::Platform::setDefault(plat);
        if (_currentPlatform != plat) {
            Log(fmt("设置 OpenCL 平台失败"));
            return -2;
        }

        _init = true;
    }
    return 0;
}

auto GetPlatformIDs() -> std::vector<cl_platform_id>
{
    cl_uint num_platforms = 0;
    clGetPlatformIDs(0, nullptr, &num_platforms);
    std::vector<cl_platform_id> platforms(num_platforms);
    clGetPlatformIDs(num_platforms, platforms.data(), nullptr);
    return platforms;
}

auto GetPlatformInfo(cl_platform_id platform, cl_program_info param_name) -> std::string
{
    size_t size_ret = 0;
    clGetPlatformInfo(platform, param_name, 0, nullptr, &size_ret);
    std::string info("empty");
    info.resize(size_ret);
    clGetPlatformInfo(platform, param_name, info.length(), reinterpret_cast<char*>(info.data()), nullptr);
    return info;
}

auto GetAllPlatformInfo(cl_platform_id platform) -> std::vector<std::pair<std::string, std::string>>
{
    std::vector<std::pair<std::string, std::string>> ret { };
    ret.emplace_back("CL_PLATFORM_NAME", GetPlatformInfo(platform, CL_PLATFORM_NAME));
    ret.emplace_back("CL_PLATFORM_VERSION", GetPlatformInfo(platform, CL_PLATFORM_VERSION));
    ret.emplace_back("CL_PLATFORM_VENDOR", GetPlatformInfo(platform, CL_PLATFORM_VENDOR));
    ret.emplace_back("CL_PLATFORM_PROFILE", GetPlatformInfo(platform, CL_PLATFORM_PROFILE));
    ret.emplace_back("CL_PLATFORM_EXTENSIONS", GetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS));
    return ret;
}

auto GetAllDeviceIDs(cl_platform_id& platform, cl_device_id& device) -> void
{
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, nullptr);
}

// 代码来源：KhronosGroup/OpenCL-CLHPP的示例
// 将于多次提交后删除
auto _testclhpp() -> int
{
    OpenCLEnvInit();

    // C++11 raw string literal for the first kernel
    std::string kernel1 { R"CLC(
        global int globalA;
        kernel void updateGlobal()
        {
          globalA = 80;
        }
    )CLC" };

    // Raw string literal for the second kernel
    std::string kernel2 { R"CLC(
        typedef struct { global int *bar; } Foo;
        kernel void vectorAdd(global const Foo* aNum, global const int *inputA, global const int *inputB,
                              global int *output, int val, write_only pipe int outPipe, queue_t childQueue)
        {
          output[get_global_id(0)] = inputA[get_global_id(0)] + inputB[get_global_id(0)] + val + *(aNum->bar);
          write_pipe(outPipe, &val);
          queue_t default_queue = get_default_queue();
          ndrange_t ndrange = ndrange_1D(get_global_size(0)/2, get_global_size(0)/2);

          // Have a child kernel write into third quarter of output
          enqueue_kernel(default_queue, CLK_ENQUEUE_FLAGS_WAIT_KERNEL, ndrange,
            ^{
                output[get_global_size(0)*2 + get_global_id(0)] =
                  inputA[get_global_size(0)*2 + get_global_id(0)] + inputB[get_global_size(0)*2 + get_global_id(0)] + globalA;
            });

          // Have a child kernel write into last quarter of output
          enqueue_kernel(childQueue, CLK_ENQUEUE_FLAGS_WAIT_KERNEL, ndrange,
            ^{
                output[get_global_size(0)*3 + get_global_id(0)] =
                  inputA[get_global_size(0)*3 + get_global_id(0)] + inputB[get_global_size(0)*3 + get_global_id(0)] + globalA + 2;
            });
        }
    )CLC" };

    std::vector<std::string> programStrings;
    programStrings.push_back(kernel1);
    programStrings.push_back(kernel2);

    cl::Program vectorAddProgram(programStrings);
    try {
        vectorAddProgram.build("-cl-std=CL3.0");
    } catch (...) {
        // Print build info for all devices
        cl_int buildErr = CL_SUCCESS;
        auto buildInfo = vectorAddProgram.getBuildInfo<CL_PROGRAM_BUILD_LOG>(&buildErr);
        for (auto& pair : buildInfo) {
            std::cerr << pair.second << std::endl
                      << std::endl;
        }

        return 1;
    }

    typedef struct {
        int* bar;
    } Foo;

    // Get and run kernel that initializes the program-scope global
    // A test for kernels that take no arguments
    auto program2Kernel = cl::KernelFunctor<>(vectorAddProgram, "updateGlobal");
    program2Kernel(
        cl::EnqueueArgs(
            cl::NDRange(1)));

    //////////////////
    // SVM allocations

    auto anSVMInt = cl::allocate_svm<int, cl::SVMTraitCoarse<>>();
    *anSVMInt = 5;
    cl::SVMAllocator<Foo, cl::SVMTraitCoarse<cl::SVMTraitReadOnly<>>> svmAllocReadOnly;
    auto fooPointer = cl::allocate_pointer<Foo>(svmAllocReadOnly);
    fooPointer->bar = anSVMInt.get();
    cl::SVMAllocator<int, cl::SVMTraitCoarse<>> svmAlloc;
    std::vector<int, cl::SVMAllocator<int, cl::SVMTraitCoarse<>>> inputA(numElements, 1, svmAlloc);
    cl::coarse_svm_vector<int> inputB(numElements, 2, svmAlloc);

    //////////////
    // Traditional cl_mem allocations

    std::vector<int> output(numElements, 0xdeadbeef);
    cl::Buffer outputBuffer(output.begin(), output.end(), false);
    cl::Pipe aPipe(sizeof(cl_int), numElements / 2);

    // Default command queue, also passed in as a parameter
    cl::DeviceCommandQueue defaultDeviceQueue = cl::DeviceCommandQueue::makeDefault(
        cl::Context::getDefault(), cl::Device::getDefault());

    auto vectorAddKernel = cl::KernelFunctor<
        decltype(fooPointer)&,
        int*,
        cl::coarse_svm_vector<int>&,
        cl::Buffer,
        int,
        cl::Pipe&,
        cl::DeviceCommandQueue>(vectorAddProgram, "vectorAdd");

    // Ensure that the additional SVM pointer is available to the kernel
    // This one was not passed as a parameter
    vectorAddKernel.setSVMPointers(anSVMInt);

    cl_int error(0);
    vectorAddKernel(
        cl::EnqueueArgs(
            cl::NDRange(numElements / 2),
            cl::NDRange(numElements / 2)),
        fooPointer,
        inputA.data(),
        inputB,
        outputBuffer,
        3,
        aPipe,
        defaultDeviceQueue,
        error);

    cl::copy(outputBuffer, output.begin(), output.end());

    cl::Device d = cl::Device::getDefault();

    std::cout << "Output:\n";
    for (int i = 1; i < numElements; ++i) {
        std::cout << "\t" << output[i] << "\n";
    }
    std::cout << "\n\n";
    return 0;
}
}
