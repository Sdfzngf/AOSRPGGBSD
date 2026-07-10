/**
 * @brief 处理参数
 *
 */
module;

#include <functional>
#include <string>
#include <unordered_map>

module Engine.Utils.Arg.Format:Actions;

import Engine.Utils.Logger;
import Engine.i18n;
import Engine.Utils.Arg.MArg;
import Engine.Utils.DevConsole;

using Engine::i18n::locale;

export namespace Engine::Utils::Arg {
const std::unordered_map<std::string, std::function<void(Engine::Utils::Arg::MArg&)>>& arg_actions = { // NOLINT
    { "--test-param1", [](Engine::Utils::Arg::MArg& mp) -> void { mp._test_param1 = true; } },
    { "--test-param2", [](Engine::Utils::Arg::MArg& mp) -> void { mp._test_param2 = true; } },
    { "--test-param3", [](Engine::Utils::Arg::MArg& mp) -> void { mp._test_param3 = true; } },
    { "console", [](Engine::Utils::Arg::MArg& mp) -> void { mp._dev_console = true; } },
    { "help", [](Engine::Utils::Arg::MArg& mp) -> void { mp._help = true; } }
};
}
