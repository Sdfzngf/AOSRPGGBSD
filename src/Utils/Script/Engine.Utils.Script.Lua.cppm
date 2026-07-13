/**
 * @brief Lua
 *
 */
module;

#include <atomic>
#include <cassert>
#include <lua.hpp>
#include <memory>
#include <string>
#include <type_traits>

export module Engine.Utils.Script.Lua;

import Engine.Utils.Logger;

// ── Lua print → Logger 桥接 ──
// extern "C" 放在 export module 之后（模块 purview），可直接调用 Logger
extern "C" {
static auto _lua_log_print(lua_State* L) -> int
{
    int n = lua_gettop(L);
    lua_getglobal(L, "tostring");
    std::string result;
    for (int i = 1; i <= n; i++) {
        lua_pushvalue(L, -1); // tostring
        lua_pushvalue(L, i);
        lua_call(L, 1, 1);
        const char* s = lua_tostring(L, -1);
        if (s == nullptr)
            return luaL_error(L, "'tostring' must return a string to 'print'"); // NOLINT
        if (i > 1)
            result += '\t';
        result += s;
        lua_pop(L, 1);
    }
    Engine::Utils::Logger::Log(std::string("[Lua Script]: ") + result, Engine::Utils::Logger::LogLevel::INFO);
    return 0;
}
}

export namespace Engine::Utils::Script {
class LuaState {
public:
    std::atomic<std::shared_ptr<lua_State>> L;

    LuaState()
        : L(std::shared_ptr<lua_State>(luaL_newstate(), [](lua_State* l) -> void { lua_close(l); }))
    {
    }

    /// 打开标准库并注入自定义 print（桥接到 Logger）
    void OpenLibs()
    {
        auto* state = L.load().get();
        luaL_openlibs(state);
        lua_register(state, "print", _lua_log_print);
    }

    template <typename T>
    void DoString(T s)
    {
        const char* code = nullptr;
        if constexpr (std::is_same_v<std::string, T>) {
            code = s.c_str();
        } else if constexpr (std::is_same_v<const char*, T>) {
            code = s;
        } else {
            assert(false);
        }
        int r = luaL_dostring(L.load().get(), code);
        if (r != LUA_OK) {
            Engine::Utils::Logger::Log(std::string("[Lua Error]: ") + lua_tostring(L.load().get(), -1),
                                       Engine::Utils::Logger::LogLevel::ERROR);
            lua_pop(L.load().get(), 1);
        }
    }

    /// 用指定长度执行 Lua 代码（数据可能不是 null 终止的）
    void DoBuffer(const char* data, size_t size)
    {
        int r = luaL_loadbuffer(L.load().get(), data, size, "script");
        if (r != LUA_OK) {
            Engine::Utils::Logger::Log(std::string("[Lua Error]: ") + lua_tostring(L.load().get(), -1),
                                       Engine::Utils::Logger::LogLevel::ERROR);
            lua_pop(L.load().get(), 1);
            return;
        }
        r = lua_pcall(L.load().get(), 0, 0, 0);
        if (r != LUA_OK) {
            Engine::Utils::Logger::Log(std::string("[Lua Error]: ") + lua_tostring(L.load().get(), -1),
                                       Engine::Utils::Logger::LogLevel::ERROR);
            lua_pop(L.load().get(), 1);
        }
    }
};
}; // namespace Engine::Utils::Script
