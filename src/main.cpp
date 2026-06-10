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

#include <memory>
#include <cstdint>
#include <format>

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
    std::shared_ptr<Engine::Utils::Data::DataEntry> enn(new Engine::Utils::Data::DataEntry("test1",16,static_cast<uint32_t>(Engine::Utils::Data::EntryType::Binary),std::make_shared<uint8_t[]>(16)));
    enn.get()->Write([](std::shared_ptr<uint8_t[]> data) {
        Engine::Basics::Memory::MemoryStream ms(data,16);
        ms<<"abcdefghijklmno";
    });
    GameDM.InsertEntry(enn);
    GameDM.SaveDB("./a.db", "NMSL", [](std::string text,float prog){
         Engine::Utils::Logger::Log(std::format("{},,{}",text,prog), Engine::Utils::Logger::LogLevel::DEBUG);
    });
}
