/**
 * @brief 开发者控制台的主活动
 *
 */
module;

#include <format>
#include <iostream>
#include <print>
#include <string>

module Engine.Utils.DevConsole:MainAct;

import Engine.Utils.Arg.MArg;
import Engine.Utils.Logger;
import Engine.i18n;

using Engine::i18n::locale;
using Engine::Utils::Logger::Log;

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
                if (cmd == "exit") {
                    return 0;
                } else if (cmd == "help") {
                    std::print("NO HELP FOR YOU!\n");
                } else {
                    Log([&cmd]() -> std::string { return std::format(locale("未知的命令: {}"), cmd); }, Engine::Utils::Logger::LogLevel::NOTIMEANDLEVEL);
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
