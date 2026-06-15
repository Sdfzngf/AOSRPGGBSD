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

// 靠北啦clang什么时候能支持反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射
// 反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射
// 反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射反射

export namespace Engine::Utils::Arg {
class Actions {
public:
    static auto Get() -> const std::unordered_map<std::string, std::function<void(Engine::Utils::Arg::MArg&)>>&;
};
// 将命令行参数转为数组
auto FormatParam(const int argc, const char* argv[], const char* envp[]) -> MArg
{
    MArg Mp;
    for (int i : std::views::iota(1, argc)) {
        if (auto it = Actions::Get().find(argv[i]); it != Actions::Get().end()) {
            it->second(Mp);
        } else {
            Engine::Utils::Logger::Log(std::format(Engine::i18n::locale("Unknown Argument \"{}\""), argv[i]), Engine::Utils::Logger::LogLevel::WARN);
        }
    }
    return Mp;
}
}
