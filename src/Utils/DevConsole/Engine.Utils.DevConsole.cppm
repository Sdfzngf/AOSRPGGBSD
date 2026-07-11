/**
 * @brief 开发者控制台。。。一定是用来开发的
 *
 */
module;

export module Engine.Utils.DevConsole;

import Engine.Utils.Arg.MArg;

export namespace Engine::Utils {
extern const char* helpmsg;
class DevConsole {
public:
    static auto MainAct(const Engine::Utils::Arg::MArg& mp) -> int;
    static auto PackData(const Engine::Utils::Arg::MArg& mp) -> int;
};
}
