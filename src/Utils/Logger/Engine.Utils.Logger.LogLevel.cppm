/**
 * @brief 日志等级枚举
 * 
 */
module;

export module Engine.Utils.Logger.LogLevel;

export namespace Engine {
    namespace Utils {
        namespace Logger {
            
            /**
             * @brief 日志等级
             * 
             */
            enum class LogLevel {
                DEBUG =0,
                INFO = 1,
                WARN = 2,
                ERROR = 3,
                CRITICAL = 4,
                WTF = 5,
                SUCCESS = 6,
                NOLEVEL = 7,//不在日志中显示日志等级
                NOTIME = 8,//不在日志中显示时间
                NOTIMEANDLEVEL = 9,//不在日志中显示时间和日志等级
                NOOPT = 10//不输出日志
            };
            inline LogLevel CurrentLogLevel = LogLevel::DEBUG;
        }
    }
}
