/**
 * @brief 国际化模块
 * 
 */
module;

#include <string>

export module Engine.i18n;

export namespace Engine {
    namespace i18n{
        inline std::string locale(const std::string text){
            return text;
        }
        inline constexpr std::string_view locale(const char* text){
            return text;
        }
    }
}
