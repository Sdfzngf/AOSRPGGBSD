/**
 * @brief 开发者控制台的主活动
 *
 */
module;

module Engine.Utils.DevConsole:MainAct;

import Engine.Utils.Arg.MArg;
import Engine.Utils.Logger;

export namespace Engine::Utils {
class DevConsole {
public:
    static auto MainAct(Engine::Utils::Arg::MArg mp) -> int
    {
        Engine::Utils::Logger::Log("DevConsole::MainAct()", Engine::Utils::Logger::LogLevel::DEBUG);
        return 0;
    }
};
}
