/**
 * @brief 开发者控制台的主活动
 *
 */
module;

#include <format>
#include <string>
#include <vector>

// 引入 replxx 头文件
#include <replxx.hxx>

export module Engine.Utils.DevConsole:MainAct;

import Engine.Utils.Arg.MArg;
import Engine.Utils.Logger;
import Engine.i18n;

using Engine::i18n::locale;
using Engine::Utils::Logger::Log;

export namespace Engine::Utils {
// 支持引号的简易解析
constexpr const char* helpmsg = "AOSRPGGBSD v0.1\n"
                                "命令行参数:\n"
                                "    console     进入交互式控制台\n"
                                "    help        输出本条信息\n"
                                "控制台参数:\n"
                                "    help        获取帮助信息\n"
                                "    listde      输出当前已加载的资源条目\n"
                                "    exit        退出交互式控制台\n"
                                "\n";

class DevConsole {
public:
    static auto parseCommandLine(const std::string& line) -> std::vector<std::string>
    {
        std::vector<std::string> args;
        std::string current;
        bool inQuotes = false;
        for (char ch : line) {
            if (ch == '"') {
                inQuotes = !inQuotes;
            } else if (ch == ' ' && !inQuotes) {
                if (!current.empty()) {
                    args.push_back(current);
                    current.clear();
                }
            } else {
                current += ch;
            }
        }
        if (!current.empty()) {
            args.push_back(current);
        }
        return args;
    }
    static auto MainAct(Engine::Utils::Arg::MArg mp) -> int
    {
        Logger::Log("DevConsole::MainAct()", Logger::LogLevel::DEBUG);

        replxx::Replxx replxx;

        std::unordered_map<std::string, std::function<void(const std::vector<std::string>&)>> handlers;
        bool exitRequested = false;

        handlers["help"] = [&](const auto& args) -> void {
            replxx.print("%s", std::string(locale(helpmsg)).c_str()); // NOLINT
        };
        handlers["exit"] = [&](const auto& args) -> void {
            exitRequested = true;
        };

        std::vector<std::string> commandNames;
        for (const auto& pair : handlers) {
            commandNames.push_back(pair.first); // NOLINT
        }

        auto completer = [&](const std::string& input, int& /*context_index*/)
            -> std::vector<replxx::Replxx::Completion> {
            std::vector<replxx::Replxx::Completion> completions;

            // 简单的空格分割（忽略引号，因为补全场景较少）
            std::vector<std::string> tokens;
            std::istringstream iss(input);
            std::string token;
            while (iss >> token)
                tokens.push_back(token);

            size_t numTokens = tokens.size();
            if (numTokens == 0) {
                // 输入为空，补全所有命令
                for (const auto& cmd : commandNames) {
                    completions.emplace_back(cmd);
                }
            } else if (numTokens == 1 && input.find(' ') == std::string::npos) {
                // 正在输入命令
                const std::string& prefix = tokens[0];
                for (const auto& cmd : commandNames) {
                    if (cmd.starts_with(prefix)) {
                        completions.emplace_back(cmd);
                    }
                }
            } else {

                const std::string& cmd = tokens[0];

                const std::string& lastToken = tokens.back();
                bool inParam = (input.back() != ' ');
                // if (inParam && cmd == "console") {
                //     static const std::vector<std::string> options = { "--port", "--help" };
                //     for (const auto& opt : options) {
                //         if (opt.starts_with(lastToken) == 0) {
                //             completions.emplace_back(opt);
                //         }
                //     }
                // }
            }
            return completions;
        };
        replxx.set_completion_callback(completer);

        std::string line;
        while (!exitRequested) {
            const char* cinput = replxx.input("DEV>>> ");
            if (cinput == nullptr) {
                replxx.print("\n"); // NOLINT
                break;
            }

            line = cinput;
            if (line.empty())
                continue;

            if (line[0] != ' ')
                replxx.history_add(line);

            auto args = parseCommandLine(line);
            if (args.empty())
                continue;

            const std::string& cmd = args[0];
            auto it = handlers.find(cmd);
            if (it != handlers.end()) {
                it->second(args);
            } else {
                replxx.print("%s\n", std::format(locale("未知的命令: {}，输入 \"help\" 获取帮助信息"), cmd).c_str()); // NOLINT
            }
        }
        return 0;
    }
};
}
