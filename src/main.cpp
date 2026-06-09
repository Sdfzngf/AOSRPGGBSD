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
    // int sz=64;
    // uint8_t* block=new uint8_t[sz];
    // Engine::Basics::Memory::MemoryStream ms(block,sz);
    // Engine::Basics::Memory::dump_hex(block,sz);
    //
    // Engine::Utils::Logger::Log("ibb", Engine::Utils::Logger::LogLevel::DEBUG);
    // ms<<"NMSL";
    // Engine::Basics::Memory::dump_hex(block,sz);
    //
    // Engine::Utils::Logger::Log("ibb", Engine::Utils::Logger::LogLevel::DEBUG);
    // ms<<"123456789)10";
    // Engine::Basics::Memory::dump_hex(block,sz);
    //
    // struct test{
    //     char a='H';
    //     int v=114514;
    //     char b='c';
    // };
    // test a;
    // ms<<a;
    // ms.SetPointer(16);
    // test b;
    // ms>>b;
    // Engine::Basics::Memory::dump_hex(block,sz);
    // Engine::Utils::Logger::Log(std::format("{},{},{}",b.a,b.v,b.b), Engine::Utils::Logger::LogLevel::DEBUG);
}
