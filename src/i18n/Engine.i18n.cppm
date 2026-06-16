/**
 * @brief 国际化模块
 *
 */
module;

#include <string>
#include <string_view>

export module Engine.i18n;

export namespace Engine::i18n {
constexpr auto locale(const std::string& text) -> std::string
{
    return text;
}
constexpr auto locale(const char* text) -> std::string_view
{
    return text;
}
}
