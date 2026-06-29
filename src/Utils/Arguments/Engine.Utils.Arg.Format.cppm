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

export namespace Engine::Utils::Arg {
extern const std::unordered_map<std::string, std::function<void(Engine::Utils::Arg::MArg&)>>& arg_actions;
// 将命令行参数转为数组
auto FormatParam(const int argc, const char* argv[], const char* envp[]) -> MArg
{
    MArg Mp;
    for (int i : std::views::iota(1, argc)) {
        if (auto it = arg_actions.find(argv[i]); it != arg_actions.end()) {
            it->second(Mp);
        } else {
            Engine::Utils::Logger::Log(std::format(Engine::i18n::locale("未知参数 \"{}\""), argv[i]), Engine::Utils::Logger::LogLevel::WARN);
        }
    }
    return Mp;
}
}
