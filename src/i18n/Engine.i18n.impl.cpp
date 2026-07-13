/**
 * @brief 国际化模块 —— 运行时 JSON 翻译表（实现）
 *
 * 将 nlohmann/json 和内部状态隔离在实现单元，避免接口膨胀。
 */
module;

#include <nlohmann/json.hpp>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

module Engine.i18n;

namespace Engine::i18n {

namespace {
    std::unordered_map<std::string, std::string> table_;
    std::shared_mutex mu_;
    std::string current_lang_;
}

void Init(const std::string& lang, const std::string& json_content)
{
    std::unique_lock lock(mu_);
    auto j = nlohmann::json::parse(json_content);
    table_.clear();
    for (auto& [key, val] : j.items()) {
        table_[key] = val.get<std::string>();
    }
    current_lang_ = lang;
}

void SwitchLanguage(const std::string& lang, const std::string& json_content)
{
    std::unique_lock lock(mu_);
    auto j = nlohmann::json::parse(json_content);
    table_.clear();
    for (auto& [key, val] : j.items()) {
        table_[key] = val.get<std::string>();
    }
    current_lang_ = lang;
}

auto CurrentLanguage() -> std::string
{
    std::shared_lock lock(mu_);
    return current_lang_;
}

auto locale(const std::string& text) -> std::string
{
    std::shared_lock lock(mu_);
    auto it = table_.find(text);
    if (it != table_.end()) {
        return it->second;
    }
    return "??" + text + "??";
}

auto locale(const char* text) -> std::string
{
    std::shared_lock lock(mu_);
    std::string key(text);
    auto it = table_.find(key);
    if (it != table_.end()) {
        return it->second;
    }
    return "??" + key + "??";
}

} // namespace Engine::i18n
