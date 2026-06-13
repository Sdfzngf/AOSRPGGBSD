module;

#include <format>
#include <functional>
#include <ranges>
#include <unordered_map>

export module Engine.Utils.Param.Format;

import Engine.Utils.Param.MParam;
import Engine.Utils.Logger;
import Engine.i18n;

export namespace Engine::Utils::Param {
std::unordered_map<std::string, std::function<void(MParam&)>> _param_actions = {
    { "--test-param1", [](MParam& mp) -> void { mp._test_param1 = true; } },
    { "--test-param2", [](MParam& mp) -> void { mp._test_param2 = true; } },
    { "--test-param3", [](MParam& mp) -> void { mp._test_param3 = true; } }
};
auto FormatParam(const int argc, const char* argv[], const char* envp[]) -> MParam
{
    MParam Mp;
    for (int i : std::views::iota(1, argc)) {
        if (auto it = _param_actions.find(argv[i]); it != _param_actions.end()) {
            it->second(Mp);
        } else {
            Engine::Utils::Logger::Log(std::format(Engine::i18n::locale("Unknown Param \"{}\""), argv[i]), Engine::Utils::Logger::LogLevel::WARN);
        }
    }
    return Mp;
}
}
