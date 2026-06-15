/**
 * @brief 开发者控制台的主活动
 *
 */
module;

#include <iostream>
#include <string>

module Engine.Utils.DevConsole:MainAct;

import Engine.Utils.Arg.MArg;
import Engine.Utils.Logger;

export namespace Engine::Utils {
class DevConsole {
public:
    static auto MainAct(Engine::Utils::Arg::MArg mp) -> int
    {
        Engine::Utils::Logger::Log("DevConsole::MainAct()", Engine::Utils::Logger::LogLevel::DEBUG);
        std::string cmd = "";
        std::string tmp = "";
        std::cout << "DEV>>> ";
        while (std::cin >> tmp) {
            cmd += tmp;
            if (std::cin.get() == '\n') {
                std::cout << cmd << '\n';
                if (cmd == "exit") {
                    return 0;
                }
                cmd = "";
                std::cout << "DEV>>> ";
                continue;
            }
        }

        return 0;
    }
};
}
