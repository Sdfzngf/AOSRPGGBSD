import Engine.Utils.Logger;
import Engine.Utils.Data.DataManager;
import Engine.Utils.Data.DataEntry;
import Engine.Utils.Data.DB;
import Engine.Game;

Engine::Utils::Data::DataManager GameDM;
Engine::Game g;

int main() {
    Engine::Utils::Logger::Log("int main()", Engine::Utils::Logger::LogLevel::DEBUG);
    //Engine::Utils::Logger::__logger_stress_test();
    g.StartUp();
    g.MainLoop();
    g.ShutDown();
}
