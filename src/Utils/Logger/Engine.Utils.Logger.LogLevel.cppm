module;

export module Engine.Utils.Logger.LogLevel;

export namespace Engine {
    namespace Utils {
        namespace Logger {
            // 日志等级
            enum class LogLevel {
                DEBUG =0,
                INFO = 1,
                WARN = 2,
                ERROR = 3,
                CRITICAL = 4,
                WTF = 5,
                SUCCESS = 6,
                NOLEVEL = 7,
                NOTIME = 8,
                NOTIMEANDLEVEL = 9,
                NOOPT = 10
            };
            inline LogLevel CurrentLogLevel = LogLevel::DEBUG;
        }
    }
}
