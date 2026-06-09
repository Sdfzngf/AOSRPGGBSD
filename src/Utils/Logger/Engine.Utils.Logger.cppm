/**
 * @brief 日志记录器
 * 
 */
module;

#include <iostream>
#include <string>
#include <type_traits>

import Engine.Utils.Time;
import Engine.i18n;

export module Engine.Utils.Logger;
export import Engine.Utils.Logger.LogLevel;

using Engine::i18n::locale;

export namespace Engine {
    namespace Utils {
        namespace Logger {
            #define __log_pref() if (loglevel < Engine::Utils::Logger::CurrentLogLevel && loglevel != Engine::Utils::Logger::LogLevel::SUCCESS) {\
            return 1;\
            }\
            if ((loglevel != Engine::Utils::Logger::LogLevel::NOTIME) && (loglevel != Engine::Utils::Logger::LogLevel::NOTIMEANDLEVEL)) {\
                printf("[%f]",Engine::Utils::Time::GetAppRunningTime());\
            }\
            switch (loglevel) {\
                case Engine::Utils::Logger::LogLevel::DEBUG:\
                    printf("\033[1;37m[DEBUG]\033[0m: ");\
                    break;\
                case Engine::Utils::Logger::LogLevel::INFO:\
                    printf("\033[1;36m[INFO]\033[0m: ");\
                    break;\
                case Engine::Utils::Logger::LogLevel::WARN:\
                    printf("\033[1;33m[WARN]\033[0m: ");\
                    break;\
                case Engine::Utils::Logger::LogLevel::ERROR:\
                    printf("\033[1;35m[ERROR]\033[0m: ");\
                    break;\
                case Engine::Utils::Logger::LogLevel::CRITICAL:\
                    printf("\033[1;31m[CRITICAL]\033[0m: ");\
                    break;\
                case Engine::Utils::Logger::LogLevel::SUCCESS:\
                    printf("\033[1;32m[SUCCESS]\033[0m: ");\
                    break;\
                case Engine::Utils::Logger::LogLevel::WTF:\
                    printf("\033[1;2;4;7;5;32m[WTF]: ");\
                    break;\
                default:\
                    break;\
            }

            /**
             * @brief 记录日志
             * 
             * @param content 日志内容
             * @param loglevel 日志等级
             * @param end 结束符，默认为换行符
             * @return int 看心情的返回值
             */
            int Log(std::string content, const Engine::Utils::Logger::LogLevel loglevel=Engine::Utils::Logger::LogLevel::DEBUG, char end='\n') {
                __log_pref();
                printf("%s\033[0m%c",content.c_str(),end);
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
            int Log(const char* content, const Engine::Utils::Logger::LogLevel loglevel=Engine::Utils::Logger::LogLevel::DEBUG, char end='\n') {
                __log_pref();
                printf("%s\033[0m%c",content,end);
                return 0;
            }

            /**
             * @brief 回调函数版本的Log
             * 
             * @param callcallback 返回日志内容的回调函数
             * @param loglevel 日志等级，如果你好奇为什么要用callback，答案是如果要用Log(std::format(locale("abcd: {}"),efg));的格式的话，不管Log()是否输出，std::format(locale("abcd: {}"),efg)都会被执行一次，用callback虽然稍显复杂，但还是会快一点
             * @param end 结束符，默认为换行符
             * @return int 看心情的返回值
             */
            int Log(std::string(*callcallback)(), const Engine::Utils::Logger::LogLevel loglevel=Engine::Utils::Logger::LogLevel::DEBUG, char end='\n') {
                __log_pref();
                printf("%s\033[0m%c",callcallback().c_str(),end);
                return 0;
            }

            /**
             * @brief 回调函数版本的Log
             * 
             * @param callcallback 返回日志内容的回调函数
             * @param loglevel 日志等级，如果你好奇为什么要用callback，答案是如果要用Log(std::format(locale("abcd: {}"),efg));的格式的话，不管Log()是否输出，std::format(locale("abcd: {}"),efg)都会被执行一次，用callback虽然稍显复杂，但还是会快一点
             * @param end 结束符，默认为换行符
             * @return int 看心情的返回值
             */
            int Log(const char*(*callcallback)(), const Engine::Utils::Logger::LogLevel loglevel=Engine::Utils::Logger::LogLevel::DEBUG, char end='\n') {
                __log_pref();
                printf("%s\033[0m%c",callcallback(),end);
                return 0;
            }

            /**
             * @brief 回调函数版本的Log，但是更通用了一些
             * 
             * @param callcallback 返回日志内容的回调函数
             * @param loglevel 日志等级，如果你好奇为什么要用callback，答案是如果要用Log(std::format(locale("abcd: {}"),efg));的格式的话，不管Log()是否输出，std::format(locale("abcd: {}"),efg)都会被执行一次，用callback虽然稍显复杂，但还是会快一点
             * @param end 结束符，默认为换行符
             * @return int 看心情的返回值
             */
            template <typename T>
            int Log(T callcallback, const Engine::Utils::Logger::LogLevel loglevel=Engine::Utils::Logger::LogLevel::DEBUG, char end='\n') {
                __log_pref();
                std::string msg = callcallback();
                printf("%s\033[0m%c",msg.c_str(),end);
                return 0;
            }


            int LogHex(){
                //我待会再来写这个
                return 0;
            }

            /**
             * @brief 测试日志等级显示和时间显示的函数
             * 
             * @return int 
             */
            int __loglevel_test__() {
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

            /**
             * @brief 压力一个函数？！
             * 
             * @return int 
             */
            int __logger_stress_test(){
                Log("int __logger_stress_test()");
                constexpr int lp=6;

                //测试1
                Log([lp](){return std::format(locale("666: {}"),lp);});
                Log(std::format(locale("111: {}"),lp));

                //更改日志等级，不输出任何东西
                Log("CurrentLogLevel = LogLevel::NOOPT");
                Engine::Utils::Logger::CurrentLogLevel=Engine::Utils::Logger::LogLevel::NOOPT;

                //测试表达式
                Log("testing int Log(std::string content, const Engine::Utils::Logger::LogLevel loglevel=Engine::Utils::Logger::LogLevel::DEBUG, char end='\\n')",Engine::Utils::Logger::LogLevel::NOOPT);

                auto a=Engine::Utils::Time::GetAppRunningTime();
                for (int i=0;i<2000000;i++){
                    Log(std::format(locale("111: {}"),6));
                }
                auto b=Engine::Utils::Time::GetAppRunningTime();
                Log("succeed,time "+std::to_string(b-a),Engine::Utils::Logger::LogLevel::NOOPT);
                //测试带变量的情况
                a=Engine::Utils::Time::GetAppRunningTime();
                for (int i=0;i<2000000;i++){
                    Log(std::format(locale("111: {}"),lp));
                }
                b=Engine::Utils::Time::GetAppRunningTime();
                Log("succeed,time "+std::to_string(b-a),Engine::Utils::Logger::LogLevel::NOOPT);
                //测试 callback
                Log("int Log(std::string(*callcallback)(), const Engine::Utils::Logger::LogLevel loglevel=Engine::Utils::Logger::LogLevel::DEBUG, char end='\\n')",Engine::Utils::Logger::LogLevel::NOOPT);
                auto c=Engine::Utils::Time::GetAppRunningTime();
                for (int i=0;i<2000000;i++){
                    Log([](){return std::format(locale("111: {}"),1);});
                }
                auto d=Engine::Utils::Time::GetAppRunningTime();
                Log("succeed,time "+std::to_string(d-c),Engine::Utils::Logger::LogLevel::NOOPT);
                //测试带捕获的callback
                Log("int Log(std::string(*callcallback)(), const Engine::Utils::Logger::LogLevel loglevel=Engine::Utils::Logger::LogLevel::DEBUG, char end='\\n')",Engine::Utils::Logger::LogLevel::NOOPT);
                c=Engine::Utils::Time::GetAppRunningTime();
                for (int i=0;i<2000000;i++){
                    Log([lp](){return std::format(locale("111: {}"),lp);});
                }
                d=Engine::Utils::Time::GetAppRunningTime();
                Log("succeed,time "+std::to_string(d-c),Engine::Utils::Logger::LogLevel::NOOPT);

                //输出日志的情况
                Log("CurrentLogLevel = LogLevel::DEBUG");
                Engine::Utils::Logger::CurrentLogLevel=Engine::Utils::Logger::LogLevel::DEBUG;
                Log("testing int Log(std::string content, const Engine::Utils::Logger::LogLevel loglevel=Engine::Utils::Logger::LogLevel::DEBUG, char end='\\n')");
                //测试普通方法
                auto e=Engine::Utils::Time::GetAppRunningTime();
                for (int i=0;i<2000000;i++){
                    Log(std::format(locale("111: {}"),1),Engine::Utils::Logger::LogLevel::DEBUG,'\r');
                }
                auto f=Engine::Utils::Time::GetAppRunningTime();
                Log("\nsucceed,time "+std::to_string(f-e));
                //测试带变量的普通方法
                e=Engine::Utils::Time::GetAppRunningTime();
                for (int i=0;i<2000000;i++){
                    Log(std::format(locale("111: {}"),lp),Engine::Utils::Logger::LogLevel::DEBUG,'\r');
                }
                f=Engine::Utils::Time::GetAppRunningTime();
                Log("\nsucceed,time "+std::to_string(f-e));

                //测试callback
                Log("int Log(std::string(*callcallback)(), const Engine::Utils::Logger::LogLevel loglevel=Engine::Utils::Logger::LogLevel::DEBUG, char end='\\n')");
                auto g=Engine::Utils::Time::GetAppRunningTime();
                for (int i=0;i<2000000;i++){
                    Log([](){return std::format(locale("111: {}"),1);},Engine::Utils::Logger::LogLevel::DEBUG,'\r');
                }
                auto h=Engine::Utils::Time::GetAppRunningTime();
                Log("\nsucceed,time "+std::to_string(h-g));
                //测试带捕获的callback
                Log("int Log(std::string(*callcallback)(), const Engine::Utils::Logger::LogLevel loglevel=Engine::Utils::Logger::LogLevel::DEBUG, char end='\\n')");
                g=Engine::Utils::Time::GetAppRunningTime();
                for (int i=0;i<2000000;i++){
                    Log([lp](){return std::format(locale("111: {}"),lp);},Engine::Utils::Logger::LogLevel::DEBUG,'\r');
                }
                h=Engine::Utils::Time::GetAppRunningTime();
                Log("\nsucceed,time "+std::to_string(h-g));

                return 1919810;
            }
        }
    }
}
