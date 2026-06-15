/**
 * @brief Engine.Utils.Param.Format.cppm，格式化命令行参数。
 *
 */
module;

#include <format>
#include <functional>
#include <ranges>
#include <unordered_map>

export module Engine.Utils.Arg.Format;

import Engine.Utils.Arg.MArg;
import Engine.Utils.Logger;
import Engine.i18n;

// 保存命令行参数对应动作的map
const std::unordered_map<std::string, std::function<void(Engine::Utils::Arg::MArg&)>> param_actions = {
    { "--test-param1", [](Engine::Utils::Arg::MArg& mp) -> void { mp._test_param1 = true; } },
    { "--test-param2", [](Engine::Utils::Arg::MArg& mp) -> void { mp._test_param2 = true; } },
    { "--test-param3", [](Engine::Utils::Arg::MArg& mp) -> void { mp._test_param3 = true; } }
};

export namespace Engine::Utils::Arg {

// 将命令行参数转为数组
auto FormatParam(const int argc, const char* argv[], const char* envp[]) -> MArg
{
    MArg Mp;
    for (int i : std::views::iota(1, argc)) {
        if (auto it = param_actions.find(argv[i]); it != param_actions.end()) {
            it->second(Mp);
        } else {
            Engine::Utils::Logger::Log(std::format(Engine::i18n::locale("Unknown Argument \"{}\""), argv[i]), Engine::Utils::Logger::LogLevel::WARN);
        }
    }
    return Mp;
}
}
