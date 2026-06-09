module;

#include <chrono>

export module Engine.Utils.Time;

export namespace Engine {
    namespace Utils {
        namespace Time {
            auto LaunchTime = std::chrono::steady_clock::now();
            inline constexpr auto GetAppRunningTime(){
                return std::chrono::duration<double>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - LaunchTime
                )).count();
            }
        }
    }
}
