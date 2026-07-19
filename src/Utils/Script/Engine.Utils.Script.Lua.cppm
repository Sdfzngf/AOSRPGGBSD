/**
 * @brief Lua
 *
 */
module;

#include <cassert>
#include <chrono>
#include <lua.hpp>
#include <sol/sol.hpp>
#include <sol/state.hpp>
#include <string>
#include <thread>
#include <type_traits>

export module Engine.Utils.Script.Lua;

import Engine.Utils.Logger;
import Engine.i18n;

using Engine::i18n::locale;

namespace {
auto lua_print(sol::variadic_args va, sol::this_state ts) -> void
{
    std::string result;
    for (auto v : va) {
        if (!result.empty())
            result += '\t';
        result += v.as<std::string>();
    }
    Engine::Utils::Logger::Log("[Lua Script]: " + result,
                               Engine::Utils::Logger::LogLevel::INFO);
}
}

export namespace Engine::Utils::Script {
class LuaState {
public:
    sol::state state_;
    LuaState()
        : state_()
    {
    }
    ~LuaState() = default;
    LuaState(const LuaState&) = delete;
    auto operator=(const LuaState&) -> LuaState& = delete;
    LuaState(LuaState&&) = delete;
    auto operator=(LuaState&&) -> LuaState& = delete;

    /// 打开标准库并注入自定义 print（桥接到 Logger）
    void OpenLibs()
    {
        state_.open_libraries();
        state_.set_function("print", lua_print);
        state_.set_function("sleep", [](int ms) -> void {
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        });
        state_.set_function("i18n_fmt", [](const std::string& key, sol::variadic_args args) -> std::string {
            std::string result = locale(key);

            std::vector<std::string> arg_strs;
            arg_strs.reserve(args.size());
            for (auto arg : args) {
                if (arg.is<std::string>()) {
                    arg_strs.push_back(arg.as<std::string>());
                } else if (arg.is<int>()) {
                    arg_strs.push_back(std::to_string(arg.as<int>()));
                } else if (arg.is<double>()) {
                    arg_strs.push_back(std::to_string(arg.as<double>()));
                } else if (arg.is<bool>()) {
                    arg_strs.emplace_back(arg.as<bool>() ? "true" : "false");
                } else {
                    arg_strs.emplace_back("[unsupported]");
                }
            }

            size_t pos = 0;
            for (const auto& s : arg_strs) {
                pos = result.find("{}", pos);
                if (pos == std::string::npos)
                    break;
                result.replace(pos, 2, s);
                pos += s.length();
            }

            return result;
        });
    }

    template <typename T>
    void DoString(T code)
    {
        static_assert(std::is_convertible_v<T, std::string_view>,
                      "DoString requires a string-like type");
        try {
            state_.safe_script(std::string_view(code));
        } catch (const sol::error& e) {
            Engine::Utils::Logger::Log(std::string("[Lua Error]: ") + e.what(),
                                       Engine::Utils::Logger::LogLevel::ERROR);
        }
    }

    /// 用指定长度执行 Lua 代码（数据可能不是 null 终止的）
    void DoBuffer(const char* data, size_t size)
    {
        try {
            sol::load_result load = state_.load_buffer(data, size, "script");
            if (!load.valid()) {
                sol::error err = load;
                throw err;
            }
            sol::protected_function_result result = load();
            if (!result.valid()) {
                sol::error err = result;
                throw err;
            }
        } catch (const sol::error& e) {
            Engine::Utils::Logger::Log(std::string("[Lua Error]: ") + e.what(),
                                       Engine::Utils::Logger::LogLevel::ERROR);
        }
    }

    auto get_state() -> sol::state& { return state_; }
};
}; // namespace Engine::Utils::Script
