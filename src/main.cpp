/**
 * @brief Main.cpp，程序入口，没了。
 *
 */
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

Engine::Game g;

/**
 * @brief main函数
 *
 * @return int 默认是0,但程序真正的返回值在Game::ShutDown()里
 */
auto main(const int argc, const char* argv[], const char* envp[]) -> int // NOLINT
{
    Engine::Utils::Logger::Log("int main()", Engine::Utils::Logger::LogLevel::DEBUG);
    Engine::Utils::Arg::MArg mp = Engine::Utils::Arg::FormatParam(argc, argv, envp);
    if (mp._dev_console) {
        return Engine::Utils::DevConsole::MainAct(mp);
    }
    g.StartUp();
    g.MainLoop();
    g.ShutDown();
}
