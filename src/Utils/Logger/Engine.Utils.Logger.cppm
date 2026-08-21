/**
 * @brief 日志记录器
 *
 */
module;

#include <atomic>
#include <iostream>
#include <string>

export module Engine.Utils.Logger;

import Engine.Utils.Time;
import Engine.i18n;
import Engine.Basics.Thread;
export import Engine.Utils.Logger.LogLevel;

using Engine::i18n::locale;

// NOLINTBEGIN
export namespace Engine::Utils::Logger {
#define _log_pref()                                                                                                                                  \
    if (loglevel < Engine::Utils::Logger::CurrentLogLevel.load(std::memory_order_relaxed) && loglevel != Engine::Utils::Logger::LogLevel::SUCCESS) { \
        return 1;                                                                                                                                    \
    }                                                                                                                                                \
    if ((loglevel != Engine::Utils::Logger::LogLevel::NOTIME) && (loglevel != Engine::Utils::Logger::LogLevel::NOTIMEANDLEVEL)) {                    \
        std::string tname = Basics::Thread::WhatsMyName();                                                                                           \
        if (tname != "") {                                                                                                                           \
            tname = std::string("][") + tname;                                                                                                       \
        }                                                                                                                                            \
        if (Engine::Basics::Thread::AmIWorker()) {                                                                                                   \
            tname += "][Worker";                                                                                                                     \
        }                                                                                                                                            \
        printf("[%f][%s%s]", Engine::Utils::Time::GetAppRunningTime(), Engine::Basics::Thread::GetTidStr().c_str(), tname.c_str());                  \
    }                                                                                                                                                \
    switch (loglevel) {                                                                                                                              \
    case Engine::Utils::Logger::LogLevel::DEBUG:                                                                                                     \
        printf("\033[1;37m[DEBUG]\033[0m: ");                                                                                                        \
        break;                                                                                                                                       \
    case Engine::Utils::Logger::LogLevel::INFO:                                                                                                      \
        printf("\033[1;36m[INFO]\033[0m: ");                                                                                                         \
        break;                                                                                                                                       \
    case Engine::Utils::Logger::LogLevel::WARN:                                                                                                      \
        printf("\033[1;33m[WARN]\033[0m: ");                                                                                                         \
        break;                                                                                                                                       \
    case Engine::Utils::Logger::LogLevel::ERROR:                                                                                                     \
        printf("\033[1;35m[ERROR]\033[0m: ");                                                                                                        \
        break;                                                                                                                                       \
    case Engine::Utils::Logger::LogLevel::CRITICAL:                                                                                                  \
        printf("\033[1;31m[CRITICAL]\033[0m: ");                                                                                                     \
        break;                                                                                                                                       \
    case Engine::Utils::Logger::LogLevel::SUCCESS:                                                                                                   \
        printf("\033[1;32m[SUCCESS]\033[0m: ");                                                                                                      \
        break;                                                                                                                                       \
    case Engine::Utils::Logger::LogLevel::WTF:                                                                                                       \
        printf("\033[1;2;4;7;5;32m[WTF]: ");                                                                                                         \
        break;                                                                                                                                       \
    default:                                                                                                                                         \
        break;                                                                                                                                       \
    }

/**
 * @brief 记录日志
 *
 * @param content 日志内容
 * @param loglevel 日志等级
 * @param end 结束符，默认为换行符
 * @return int 看心情的返回值
 */
auto Log(std::string content, const Engine::Utils::Logger::LogLevel loglevel = Engine::Utils::Logger::LogLevel::DEBUG, char end = '\n') -> int
{
    _log_pref();
    printf("%s\033[0m%c", content.c_str(), end);
    return 0;
}

/**
 * @brief 记录日志
 *
 * @param content 日志内容
 * @param loglevel 日志等级
 * @param end 结束符，默认为换行符
 * @return int 看心情的返回值
 */
auto Log(const char* content, const Engine::Utils::Logger::LogLevel loglevel = Engine::Utils::Logger::LogLevel::DEBUG, char end = '\n') -> int
{
    _log_pref();
    printf("%s\033[0m%c", content, end);
    return 0;
}

/**
 * @brief 记录日志
 *
 * @param content 日志内容（pair）
 * @param loglevel 日志等级
 * @param end 结束符，默认为换行符
 * @return int 看心情的返回值
 */
auto Log(std::pair<std::string, std::string> content, const Engine::Utils::Logger::LogLevel loglevel = Engine::Utils::Logger::LogLevel::DEBUG, char end = '\n') -> int
{
    _log_pref();
    printf("%s: %s\033[0m%c", content.first.c_str(), content.second.c_str(), end);
    return 0;
}

/**
 * @brief 回调函数版本的Log
 *
 * @param callcallback 返回日志内容的回调函数
 * @param loglevel 日志等级，如果你好奇为什么要用callback，答案是如果要用Log(Engine::i18n::fmt("abcd: {}", efg));的格式的话，不管Log()是否输出，std::format(locale("abcd: {}"),efg)都会被执行一次，用callback虽然稍显复杂，但还是会快一点
 * @param end 结束符，默认为换行符
 * @return int 看心情的返回值
 */
auto Log(std::string (*callcallback)(), const Engine::Utils::Logger::LogLevel loglevel = Engine::Utils::Logger::LogLevel::DEBUG, char end = '\n') -> int
{
    _log_pref();
    printf("%s\033[0m%c", callcallback().c_str(), end);
    return 0;
}

/**
 * @brief 回调函数版本的Log
 *
 * @param callcallback 返回日志内容的回调函数
 * @param loglevel 日志等级，如果你好奇为什么要用callback，答案是如果要用Log(Engine::i18n::fmt("abcd: {}", efg));的格式的话，不管Log()是否输出，std::format(locale("abcd: {}"),efg)都会被执行一次，用callback虽然稍显复杂，但还是会快一点
 * @param end 结束符，默认为换行符
 * @return int 看心情的返回值
 */
auto Log(const char* (*callcallback)(), const Engine::Utils::Logger::LogLevel loglevel = Engine::Utils::Logger::LogLevel::DEBUG, char end = '\n') -> int
{
    _log_pref();
    printf("%s\033[0m%c", callcallback(), end);
    return 0;
}

/**
 * @brief 回调函数版本的Log，但是更通用了一些
 *
 * @param callcallback 返回日志内容的回调函数
 * @param loglevel 日志等级，如果你好奇为什么要用callback，答案是如果要用Log(Engine::i18n::fmt("abcd: {}", efg));的格式的话，不管Log()是否输出，std::format(locale("abcd: {}"),efg)都会被执行一次，用callback虽然稍显复杂，但还是会快一点
 * @param end 结束符，默认为换行符
 * @return int 看心情的返回值
 */
template <typename T>
auto Log(T callcallback, const Engine::Utils::Logger::LogLevel loglevel = Engine::Utils::Logger::LogLevel::DEBUG, char end = '\n') -> int
{
    _log_pref();
    std::string msg = callcallback();
    printf("%s\033[0m%c", msg.c_str(), end);
    return 0;
}

auto LogHex() -> int
{
    // 我待会再来写这个
    // 没错
    // 对
    return 0;
}

/**
 * @brief 测试日志等级显示和时间显示的函数
 *
 * @return int
 */
auto _loglevel_test_() -> int
{
    std::cout << Engine::Utils::Time::GetAppRunningTime() << "test\n";
    Log("Ciallo", Engine::Utils::Logger::LogLevel::WTF);
    Log("DEBUG", Engine::Utils::Logger::LogLevel::DEBUG);
    Log("INFO", Engine::Utils::Logger::LogLevel::INFO);
    Log("WARN", Engine::Utils::Logger::LogLevel::WARN);
    Log("ERROR", Engine::Utils::Logger::LogLevel::ERROR);
    Log("CRITICAL", Engine::Utils::Logger::LogLevel::CRITICAL);
    Log("SUCCESS", Engine::Utils::Logger::LogLevel::SUCCESS);
    return 114514;
}
}
// NOLINTEND
