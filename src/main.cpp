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
import Engine.Utils.Param.Format;
import Engine.Utils.Param.MParam;

Engine::Utils::Data::DataManager GameDM;
Engine::Game g;

#include <format>

/**
 * @brief main函数
 *
 * @return int 默认是0,但程序真正的返回值在Game::ShutDown()里
 */
auto main(const int argc, const char* argv[], const char* envp[]) -> int // NOLINT
{
    Engine::Utils::Logger::Log("int main()", Engine::Utils::Logger::LogLevel::DEBUG);
    g.StartUp();
    g.MainLoop();
    g.ShutDown();
    // std::shared_ptr<Engine::Utils::Data::DataEntry> enn(new Engine::Utils::Data::DataEntry("nishigay",16,static_cast<uint32_t>(Engine::Utils::Data::EntryType::Binary),std::make_shared<uint8_t[]>(16)));
    // enn.get()->Write([](std::shared_ptr<uint8_t[]> data) {
    //     Engine::Basics::Memory::MemoryStream ms(data,16);
    //     ms<<"nishigaynishiga";
    // });
    // GameDM.InsertEntry(enn);
    //
    // std::shared_ptr<Engine::Utils::Data::DataEntry> enb(new Engine::Utils::Data::DataEntry("yuanshen",591,static_cast<uint32_t>(Engine::Utils::Data::EntryType::Binary),std::make_shared<uint8_t[]>(591)));
    // enb.get()->Write([](std::shared_ptr<uint8_t[]> data) {
    //     Engine::Basics::Memory::MemoryStream ms(data,591);
    //     ms<<"诶😯？云朵☁️😄，哒↘哒↗哒↘哒↗哒↘，好想玩原神😨，云☁️原神😙，当当当当当😊，看精彩纷纷👍🎊😆，云☁️原神😄，呜呜呜呜呜，好想玩原神😭😭😭云☁️原神，朋友已就位😊😃😆，一起玩原神，云☁️原神！啊啊啊啊啊😙，好想玩原神😙云☁️原神，哈哈哈哈哈🤣🤣🤣，一起玩原神，云☁️原神，好好好想，🤩想玩玩原神😋网页云端，低功耗不失真😌，WiFi网线🥰，都可以60帧😍，来来来来👏，进入云☁️原神";
    // });
    // GameDM.InsertEntry(enb);
    // GameDM.SaveDB<true>("./a.db", "NMSL苏安地方不勤务部白求恩妇科权威恢复iqh", [](std::string text,float prog){
    //      Engine::Utils::Logger::Log(std::format("{},,{}",text,prog), Engine::Utils::Logger::LogLevel::DEBUG);
    // });
    // GameDM.MountDB("./a.db");
    // GameDM.GetEntry("yuanshen").get()->Read([](const std::shared_ptr<uint8_t[]>& data) -> void {
    //     Engine::Utils::Logger::Log(std::string(reinterpret_cast<const char*>(data.get())));
    // });
    // GameDM.GetEntry("nishigay").get()->Read([](const std::shared_ptr<uint8_t[]>& data) -> void {
    //     Engine::Utils::Logger::Log(std::string(reinterpret_cast<const char*>(data.get())));
    // });
}
