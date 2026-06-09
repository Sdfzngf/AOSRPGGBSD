/**
 * @brief 管理时间的模块
 * 
 */
module;

#include <chrono>

export module Engine.Utils.Time;

export namespace Engine {
    namespace Utils {
        namespace Time {
            auto LaunchTime = std::chrono::steady_clock::now();
            
            /**
             * @brief 获取程序运行时间
             * 
             * @return constexpr auto 从程序启动到现在的时间，单位为毫秒
             */
            inline constexpr auto GetAppRunningTime(){
                return std::chrono::duration<double>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - LaunchTime
                )).count();
            }
        }
    }
}
