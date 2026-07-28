/**
 * @brief GUI模块-OpenCL渲染、计算（已弃用）
 *
 */
module;

#include <CL/cl.h>
#include <string>
#include <vector>

export module Engine.GUI.OpenCL;

export namespace Engine::GUI::OpenCL {
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

auto _testclhpp() -> int
{
    return 0;
}
}
