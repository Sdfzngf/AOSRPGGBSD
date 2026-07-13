/**
 * @brief 管理时间的模块
 *
 */
module;

#include <chrono>

export module Engine.Utils.Time;

export namespace Engine::Utils::Time {
auto LaunchTime = std::chrono::steady_clock::now();

/**
 * @brief 获取程序运行时间
 *
 * @return constexpr auto 从程序启动到现在的时间，单位为秒（微秒精度）
 */
inline auto GetAppRunningTime()
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - LaunchTime).count();
}
}
