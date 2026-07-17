/**
 * @brief Lua
 *
 */
module;

#include <cassert>
#include <lua.hpp>
#include <sol/sol.hpp>
#include <sol/state.hpp>
#include <string>
#include <type_traits>

export module Engine.Utils.Script.Lua;

import Engine.Utils.Logger;

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
