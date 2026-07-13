/**
 * @brief Main.cpp，程序入口，没了。
 *
 */

#include <lua.hpp>
#include <string>

import Engine.Utils.Logger;
import Engine.Utils.Data.DataManager;
import Engine.Utils.Data.DataEntry;
import Engine.Utils.Data.DB;
import Engine.Game;
import Engine.Basics.Memory.MemoryStream;
import Engine.Basics.Memory;
import Engine.Utils.Data.DataEntry.EntryType;
import Engine.Utils.Arg.Format;
import Engine.Utils.Arg.MArg;
import Engine.Utils.DevConsole;
import Engine.i18n;

Engine::Game g;

/**
 * @brief main函数
 *
 * @return int 默认是0,但程序真正的返回值在Game::ShutDown()里
 */
auto main(int argc, char* argv[]) -> int
{
    Engine::Utils::Logger::Log("int main()", Engine::Utils::Logger::LogLevel::DEBUG);
    Engine::Utils::Arg::MArg mp = Engine::Utils::Arg::FormatParam(argc, const_cast<const char**>(argv), nullptr);
    if (mp._dev_console) {
        return Engine::Utils::DevConsole::MainAct(mp);
    } else if (mp._help) {
        Engine::Utils::Logger::Log(std::string(Engine::i18n::locale(Engine::Utils::helpmsg)), Engine::Utils::Logger::LogLevel::NOTIMEANDLEVEL);
        return 0;
    } else if (mp._pack) {
        return Engine::Utils::DevConsole::PackData(mp);
    }
    g.StartUp();
    g.MainLoop();
    g.ShutDown();
}
