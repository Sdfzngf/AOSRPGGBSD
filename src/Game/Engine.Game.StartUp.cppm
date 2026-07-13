/**
 * @brief 启动！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
 *
 */
module;

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>

module Engine.Game:StartUp;

import Engine.Game;
import Engine.Utils.Logger;
import Engine.Utils.Data.DataManager;
import Engine.Utils.Data.DataEntry;
import Engine.Basics.Random;
import Engine.Utils.Script.ScriptManager;
import Engine.i18n;

using Engine::Utils::Logger::Log;

export namespace Engine {

Engine::Utils::Data::DataManager DM;
Engine::Utils::Script::ScriptManager SM;

auto Engine::Game::StartUp() -> void
{
    Log("void Engine::Game::StartUp()");
    std::string myth = Engine::Basics::Random::rand_str(256);
    DM.CreateSnapshotAll(myth);
    DM.MountDB("./Game/startup.dat");
    DM.MountDB("./Lang/Lang.dat");

    std::fstream file("./Lang/config.txt");
    std::string lang = "zh-CN";
    if (file) {
        std::getline(file, lang);
    }
    file.close();

    lang = std::string("i18n.rres@") + lang;

    auto i18nEntry = DM.GetEntry(lang);
    if (i18nEntry) {
        i18nEntry->Read([&](const std::shared_ptr<uint8_t[]>& data) -> void {
            std::string jsonContent(reinterpret_cast<const char*>(data.get()), i18nEntry->Size.load());
            Engine::i18n::Init("zh-CN", jsonContent);
        });
    }

    SM.L.OpenLibs();
    SM.RunScript(DM.GetEntry(std::string("startup.rres@startup.lua")));
    DM.MountDB("./Game/startup.dat");
}
}
