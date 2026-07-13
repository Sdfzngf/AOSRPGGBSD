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
    std::string ppara = "";
    for (int i : std::views::iota(1, argc)) {
        if (auto it = arg_actions.find(argv[i]); it != arg_actions.end()) {
            ppara = it->first;
            it->second(Mp);
        } else {
            if (ppara == "pack")
                Mp._pack_arg.emplace_back(argv[i]);
            else
                Engine::Utils::Logger::Log(Engine::i18n::fmt("未知参数 \"{}\"", argv[i]), Engine::Utils::Logger::LogLevel::WARN);
        }
    }
    return Mp;
}
}
