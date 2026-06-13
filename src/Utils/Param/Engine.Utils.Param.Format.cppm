/**
 * @brief Engine.Utils.Param.Format.cppm，格式化命令行参数。
 *
 */
module;

#include <format>
#include <functional>
#include <ranges>
#include <unordered_map>

export module Engine.Utils.Param.Format;

import Engine.Utils.Param.MParam;
import Engine.Utils.Logger;
import Engine.i18n;

// 保存命令行参数对应动作的map
const std::unordered_map<std::string, std::function<void(Engine::Utils::Param::MParam&)>> param_actions = {
    { "--test-param1", [](Engine::Utils::Param::MParam& mp) -> void { mp._test_param1 = true; } },
    { "--test-param2", [](Engine::Utils::Param::MParam& mp) -> void { mp._test_param2 = true; } },
    { "--test-param3", [](Engine::Utils::Param::MParam& mp) -> void { mp._test_param3 = true; } }
};

export namespace Engine::Utils::Param {

// 将命令行参数转为数组
auto FormatParam(const int argc, const char* argv[], const char* envp[]) -> MParam
{
    MParam Mp;
    for (int i : std::views::iota(1, argc)) {
        if (auto it = param_actions.find(argv[i]); it != param_actions.end()) {
            it->second(Mp);
        } else {
            Engine::Utils::Logger::Log(std::format(Engine::i18n::locale("Unknown Param \"{}\""), argv[i]), Engine::Utils::Logger::LogLevel::WARN);
        }
    }
    return Mp;
}
}
