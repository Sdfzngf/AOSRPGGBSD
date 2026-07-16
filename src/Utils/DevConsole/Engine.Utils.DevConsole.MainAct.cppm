/**
 * @brief 开发者控制台的主活动
 *
 */
module;

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iomanip>
#include <replxx.hxx>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

export module Engine.Utils.DevConsole:MainAct;

import Engine.Utils.Arg.MArg;
import Engine.Utils.Logger;
import Engine.i18n;
import Engine.Utils.Data.DataManager;
import Engine.Utils.Data.DataEntry;
import Engine.Basics.TextUtils;

using Engine::Basics::TextUtils::IsLikelyUtf8;
using Engine::Basics::TextUtils::IsMostlyPrintable;
using Engine::Basics::TextUtils::kHexSummaryBytes;
using Engine::Basics::TextUtils::kMaxShowBytes;
using Engine::Basics::TextUtils::TypeColorFromIndex;
using Engine::Basics::TextUtils::TypeNameFromIndex;
using Engine::i18n::locale;
using Engine::Utils::Logger::Log;

Engine::Utils::Data::DataManager ddm;

/**
 * @brief 将条目类型文本解析为内部类型编号。
 * @param typeText 类型文本或数字编号。
 * @return 对应的类型编号，未识别时默认返回 String 类型。
 */
static auto ParseEntryType(const std::string& typeText) -> uint32_t
{
    if (typeText == "0" || typeText == "binary" || typeText == "Binary")
        return 0;
    if (typeText == "1" || typeText == "string" || typeText == "String")
        return 1;
    if (typeText == "2" || typeText == "list" || typeText == "List")
        return 2;
    if (typeText == "3" || typeText == "script" || typeText == "Script")
        return 3;
    if (typeText == "4" || typeText == "png" || typeText == "PNG")
        return 4;
    if (typeText == "5" || typeText == "svg" || typeText == "SVG")
        return 5;
    return 0;
}

/**
 * @brief 从文件读取原始字节。
 * @param path 文件路径。
 * @return 成功时返回包含文件内容的缓冲区；失败时返回 nullptr。
 */
static auto LoadFileBytes(const std::string& path) -> std::shared_ptr<uint8_t[]>
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return nullptr;
    }

    file.seekg(0, std::ios::end);
    const auto fileSize = file.tellg();
    if (fileSize < 0) {
        return nullptr;
    }
    file.seekg(0, std::ios::beg);

    auto buffer = std::make_shared<uint8_t[]>(static_cast<size_t>(fileSize));
    if (fileSize > 0) {
        file.read(reinterpret_cast<char*>(buffer.get()), fileSize);
        if (!file) {
            return nullptr;
        }
    }
    return buffer;
}

/**
 * @brief 解析控制台输入行。
 * @param line 原始输入行。
 * @return 解析后的参数列表。
 */
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

/**
 * @brief 添加一个条目到当前内存数据库。
 *
 * @param args 参数
 * @param replxx replxx
 *
 * 支持两种输入形式：
 * - add <name> <data> [type]
 * - add <name> --file <path> [type]
 */
void Add(const auto& args, replxx::Replxx& replxx, std::string filename = "") // NOLINT
{
    if (args.size() < 3) {
        replxx.print("%s\n", std::string(locale("用法: add <name> <data> [type] 或 add <name> --file <path> [type]")).c_str()); // NOLINT
        return;
    }

    const std::string name = filename + args.at(1);
    uint32_t type = 1;
    std::shared_ptr<uint8_t[]> dataBuffer;
    uint32_t dataSize = 0;

    if (args.size() >= 4 && (args.at(2) == "--file" || args.at(2) == "-f")) {
        const std::string& path = args.at(3);
        if (args.size() >= 5) {
            type = ParseEntryType(args.at(4));
        }

        dataBuffer = LoadFileBytes(path);
        if (!dataBuffer && !std::ifstream(path, std::ios::binary)) {
            replxx.print("%s\n", Engine::i18n::fmt("无法打开文件: {}", path).c_str()); // NOLINT
            return;
        }

        std::ifstream file(path, std::ios::binary);
        file.seekg(0, std::ios::end);
        const auto fileSize = file.tellg();
        if (fileSize < 0) {
            replxx.print("%s\n", Engine::i18n::fmt("无法读取文件大小: {}", path).c_str()); // NOLINT
            return;
        }
        dataSize = static_cast<uint32_t>(fileSize);
        if (dataSize == 0) {
            dataBuffer = std::make_shared<uint8_t[]>(0);
        }
    } else {
        const std::string& dataText = args.at(2);
        if (args.size() >= 4) {
            type = ParseEntryType(args.at(3));
        }

        dataSize = static_cast<uint32_t>(dataText.size());
        dataBuffer = std::make_shared<uint8_t[]>(dataText.size());
        if (!dataText.empty()) {
            std::ranges::copy(dataText, dataBuffer.get());
        }
    }

    auto entry = std::make_shared<Engine::Utils::Data::DataEntry>(name, dataSize, type, dataBuffer);
    ddm.InsertEntry(name, entry);
    replxx.print("%s\n", Engine::i18n::fmt("已添加条目: {}", name).c_str()); // NOLINT
}

void SaveDB(const auto& args, replxx::Replxx& replxx)
{
    if (args.size() < 2) {
        replxx.print("%s\n", std::string(locale("用法: savedb <path> [desc]")).c_str()); // NOLINT
        return;
    }
    std::string desc = args.size() >= 3 ? args.at(2) : std::string();
    int r = ddm.SaveDB(args.at(1), desc);
    if (r != 0)
        replxx.print("%s\n", Engine::i18n::fmt("保存失败: {}", r).c_str()); // NOLINT
    else
        replxx.print("%s\n", std::string(locale("保存成功")).c_str()); // NOLINT
}

export namespace Engine::Utils {

constexpr const char* helpmsg = "AOSRPGGBSD v0.1\n"
                                "命令行参数:\n"
                                "    console     进入交互式控制台\n"
                                "    pack        打包数据: pack <source>... <target> <desc>\n"
                                "    help        输出本条信息\n"
                                "控制台参数:\n"
                                "    help        获取帮助信息\n"
                                "    add         添加条目: add <name> <data> [type]\n"
                                "                或 add <name> --file <path> [type]\n"
                                "    listde      输出当前已加载的资源条目\n"
                                "    show        显示条目内容: show <name>\n"
                                "    rm          删除条目: rm <name>\n"
                                "    load        加载数据库: load <path>\n"
                                "    savedb      保存数据库: savedb <path> [desc]\n"
                                "    exit        退出交互式控制台\n"
                                "\n";
std::unordered_map<std::string, std::function<void(const std::vector<std::string>&)>> handlers;
class DevConsole {

public:
    /**
     * @brief 启动开发者控制台主循环。
     * @param mp 解析后的命令行参数。
     * @return 程序退出码。
     */
    static auto MainAct(const Engine::Utils::Arg::MArg& mp) -> int
    {
        Logger::Log("DevConsole::MainAct()", Logger::LogLevel::DEBUG);

        replxx::Replxx replxx;

        bool exitRequested = false;

        handlers["help"] = [&](const auto& args) -> void {
            replxx.print("%s", std::string(locale(helpmsg)).c_str()); // NOLINT
        };
        handlers["exit"] = [&](const auto& args) -> void {
            exitRequested = true;
        };
        handlers["listde"] = [&](const auto& args) -> void {
            for (auto& i : ddm.GetList()) {
                replxx.print("%s\n", i.c_str()); // NOLINT
            }
        };

        handlers["add"] = [&](const auto& args) -> void {
            Add(args, replxx);
        };
        handlers["load"] = [&](const auto& args) -> void {
            if (args.size() < 2) {
                replxx.print("%s\n", std::string(locale("用法: load <path>")).c_str()); // NOLINT
                return;
            }
            int r = ddm.MountDB(args.at(1));
            if (r != 0)
                replxx.print("%s\n", Engine::i18n::fmt("加载失败: {}", r).c_str()); // NOLINT
            else
                replxx.print("%s\n", std::string(locale("加载成功")).c_str()); // NOLINT
        };
        handlers["savedb"] = [&](const auto& args) -> void {
            SaveDB(args, replxx);
        };
        handlers["show"] = [&](const auto& args) -> void {
            if (args.size() < 2) {
                replxx.print("%s\n", std::string(locale("用法: show <name>")).c_str()); // NOLINT
                return;
            }
            auto ent = ddm.GetEntry(args.at(1));
            if (!ent) {
                replxx.print("%s\n", Engine::i18n::fmt("未找到条目: {}", args.at(1)).c_str()); // NOLINT
                return;
            }
            try {
                const auto t = ent->Type.load();
                std::string typeDisp = std::format("{}{}{}", TypeColorFromIndex(t), TypeNameFromIndex(t), "\x1b[0m");
                replxx.print("%s\n", Engine::i18n::fmt("类型: {}", typeDisp).c_str()); // NOLINT
                ent->Read([&](const std::shared_ptr<uint8_t[]>& data) -> void {
                    uint32_t sz = ent->GetSize();
                    replxx.print("%s\n", Engine::i18n::fmt("条目: {} 大小: {} 字节", args.at(1), sz).c_str()); // NOLINT
                    auto show = std::min(sz, static_cast<uint32_t>(kMaxShowBytes));

                    // 先探测是否可能是 UTF-8 文本且大多数字节可打印
                    if (IsLikelyUtf8(data.get(), show) && IsMostlyPrintable(data.get(), show)) {
                        // 打印文本视图（截断显示），用绿色高亮
                        std::string s(reinterpret_cast<const char*>(data.get()), show);
                        std::string colored = std::format("\x1b[32m{}\x1b[0m", s);
                        replxx.print("%s\n", colored.c_str()); // NOLINT
                        if (show < sz)
                            replxx.print("%s\n", Engine::i18n::fmt("... (显示 {} 字节 / 总共 {} 字节)", show, sz).c_str()); // NOLINT
                    } else {
                        // 打印 hex 摘要，用黄色高亮
                        std::ostringstream oss;
                        oss << std::hex << std::setfill('0');
                        uint32_t hexShow = show < static_cast<uint32_t>(kHexSummaryBytes) ? show : static_cast<uint32_t>(kHexSummaryBytes);
                        for (uint32_t i = 0; i < hexShow; ++i) {
                            oss << std::setw(2) << static_cast<int>(data.get()[i]);
                            if (i + 1 != hexShow)
                                oss << ' ';
                        }
                        std::string hexs = oss.str();
                        std::string colored = std::format("\x1b[33m{}\x1b[0m", hexs);
                        replxx.print("%s\n", colored.c_str()); // NOLINT
                        if (show < sz)
                            replxx.print("%s\n", Engine::i18n::fmt("... (hex 显示 {} 字节 / 总共 {} 字节)", hexShow, sz).c_str()); // NOLINT
                    }
                });
            } catch (const std::exception& e) {
                replxx.print("%s\n", std::string(e.what()).c_str()); // NOLINT
            }
        };
        handlers["rm"] = [&](const auto& args) -> void {
            if (args.size() < 2) {
                replxx.print("%s\n", std::string(locale("用法: rm <name>")).c_str()); // NOLINT
                return;
            }
            bool ok = ddm.RemoveEntry(args.at(1));
            if (ok)
                replxx.print("%s\n", Engine::i18n::fmt("已删除: {}", args.at(1)).c_str()); // NOLINT
            else
                replxx.print("%s\n", Engine::i18n::fmt("未找到条目: {}", args.at(1)).c_str()); // NOLINT
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
                const std::string& prefix = tokens.front();
                for (const auto& cmd : commandNames) {
                    if (cmd.starts_with(prefix)) {
                        completions.emplace_back(cmd);
                    }
                }
            } else {

                const std::string& cmd = tokens.front();

                const std::string& lastToken = tokens.back();
                bool inParam = (input.back() != ' ');
                if (inParam && (cmd == "show" || cmd == "rm")) {
                    auto names = ddm.GetList();
                    for (const auto& n : names) {
                        if (n.starts_with(lastToken))
                            completions.emplace_back(n);
                    }
                }
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

            if (!line.empty() && line.front() != ' ')
                replxx.history_add(line);

            auto args = parseCommandLine(line);
            if (args.empty())
                continue;

            const std::string& cmd = args.front();
            auto it = handlers.find(cmd);
            if (it != handlers.end()) {
                it->second(args);
            } else {
                replxx.print("%s\n", Engine::i18n::fmt("未知的命令: {}，输入 \"help\" 获取帮助信息", cmd).c_str()); // NOLINT
            }
        }
        return 0;
    }
    static auto PackData(const Engine::Utils::Arg::MArg& mp) -> int
    {
        if (mp._pack) {
            if (mp._pack_arg.size() < 3) {
                Log(Engine::i18n::fmt("{0}: 至少需要 {1} 个参数", "pack", "3"), Logger::LogLevel::ERROR);
                return 1;
            }
            int j = 0;
            replxx::Replxx replxx;
            for (auto& i : mp._pack_arg) {
                if (j > mp._pack_arg.size() - 3) {
                    break;
                }
                std::fstream file(i, std::ios_base::in);
                if (!file) {
                    Log(Engine::i18n::fmt("{0}: 无法打开文件 {1}", "pack", i), Logger::LogLevel::ERROR);
                    return 2;
                }
                // 获取 .rres 文件所在目录，用于解析 --file 相对路径
                const std::filesystem::path rresDir = std::filesystem::absolute(i).parent_path();
                std::string filename = std::filesystem::path(i).filename();
                filename += "@";
                std::string line;
                while (std::getline(file, line)) {
                    const char* cinput = line.c_str();
                    if (cinput == nullptr) {
                        replxx.print("\n"); // NOLINT
                        break;
                    }

                    line = cinput;
                    if (line.empty())
                        continue;

                    auto args = parseCommandLine(line);
                    if (args.empty())
                        continue;

                    // 将 --file 的相对路径基于 .rres 文件目录解析
                    for (size_t a = 2; a + 1 < args.size(); ++a) {
                        if ((args[a] == "--file" || args[a] == "-f") && !args[a + 1].empty() && args[a + 1][0] != '/') {
                            args[a + 1] = (rresDir / args[a + 1]).lexically_normal().string();
                        }
                    }

                    const std::string& cmd = args.front();
                    if (cmd == "add") {
                        Add(args, replxx, filename);
                    }
                }
                file.close();
                j++;
            }
            SaveDB(std::vector<std::string> {
                       "savedb",
                       mp._pack_arg.at(mp._pack_arg.size() - 2),
                       mp._pack_arg.at(mp._pack_arg.size() - 1)

                   },
                   replxx);
        }
        return 0;
    }
};
}
