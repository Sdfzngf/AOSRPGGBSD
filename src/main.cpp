/**
 * @brief Main.cpp，程序入口，没了。
 * 
 */
import Engine.Utils.Logger;
import Engine.Utils.Data.DataManager;
import Engine.Utils.Data.DataEntry;
import Engine.Utils.Data.DB;
import Engine.Game;

Engine::Utils::Data::DataManager GameDM;
Engine::Game g;

/**
 * @brief main函数
 * 
 * @return int 默认是0,但程序真正的返回值在Game::ShutDown()里
 */
int main() {
    Engine::Utils::Logger::Log("int main()", Engine::Utils::Logger::LogLevel::DEBUG);
    //Engine::Utils::Logger::__logger_stress_test();
    g.StartUp();
    g.MainLoop();
    g.ShutDown();
}
