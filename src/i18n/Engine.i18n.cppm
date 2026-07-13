/**
 * @brief 国际化模块 —— 运行时 JSON 翻译表（接口声明）
 *
 * 源字符串作 key，nlohmann/json 解析，shared_mutex 保护读多写少场景。
 * 翻译数据由调用方以 JSON 字符串传入（避免与 DataManager 循环依赖）。
 */
module;

#include <format>
#include <string>
#include <utility>

export module Engine.i18n;

export namespace Engine::i18n {

/**
 * @brief 加载翻译表（启动时调用）
 *
 * @param lang 语言代码，如 "zh-CN"、"en-US"
 * @param json_content 完整的 JSON 键值对字符串
 */
void Init(const std::string& lang, const std::string& json_content);

/**
 * @brief 运行时切换语言
 *
 * @param lang 语言代码
 * @param json_content 完整的 JSON 键值对字符串
 */
void SwitchLanguage(const std::string& lang, const std::string& json_content);

/**
 * @brief 返回当前语言代码
 */
auto CurrentLanguage() -> std::string;

/**
 * @brief 翻译字符串
 *
 * 命中的 key 返回翻译值；未命中返回 "??原始字符串??" 作为开发期标记。
 *
 * @param text 源语言字符串（即 key）
 * @return 翻译后的字符串
 */
auto locale(const std::string& text) -> std::string;
auto locale(const char* text) -> std::string;

/**
 * @brief 翻译 + 格式化（替代 Engine::i18n::fmt(...), args)）
 *
 * 因为 std::format 要求格式字符串为编译时常量，runtime i18n 必须用 vformat。
 *
 * @param key 源语言字符串（翻译 key + format 模板）
 * @param args 格式化参数
 * @return 翻译并格式化后的字符串
 */
template <typename... Args>
auto fmt(const std::string& key, Args&&... args) -> std::string
{
    return std::vformat(locale(key), std::make_format_args(args...));
}

template <typename... Args>
auto fmt(const char* key, Args&&... args) -> std::string
{
    return std::vformat(locale(key), std::make_format_args(args...));
}

} // namespace Engine::i18n
