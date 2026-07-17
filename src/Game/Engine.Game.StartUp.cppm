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

auto Engine::Game::StartUp() -> void
{
    Log("void Engine::Game::StartUp()");

    DM.store(std::make_shared<Engine::Utils::Data::DataManager>());
    SM.store(std::make_shared<Engine::Utils::Script::ScriptManager>());
    SM.load().get()->BindDataManager(DM);
    GM.store(std::make_shared<Engine::GUI::GUIManager>());

    std::string myth = Engine::Basics::Random::rand_str(256);
    DM.load().get()->CreateSnapshotAll(myth);
    DM.load().get()->MountDB("./Game/startup.dat");
    DM.load().get()->MountDB("./Lang/Lang.dat");

    std::fstream file("./Lang/config.txt");
    std::string lang = "zh-CN";
    if (file) {
        std::getline(file, lang);
    }
    file.close();

    lang = std::string("__EG_i18n__@") + lang;

    auto i18nEntry = DM.load().get()->GetEntry(lang);
    if (i18nEntry) {
        i18nEntry->Read([&](const std::shared_ptr<uint8_t[]>& data) -> void {
            std::string jsonContent(reinterpret_cast<const char*>(data.get()), i18nEntry->Size.load());
            Engine::i18n::Init("zh-CN", jsonContent);
        });
    }

    SM.load().get()->OpenLibs();
    SM.load().get()->SetupMainDMAPI();
    SM.load().get()->RunScript(std::string("__Engine_StartUp__@startup.lua"));

    DM.load().get()->MountDB("./Test/worker.dat");
    SM.load().get()->RunScript(std::string("__Engine_Test_Worker__@workertest.lua"));

    if (GM.load().get()->Init("SDL") != 0)
        return;

    wW = 640;
    wH = 480;

    if (GM.load().get()->CreateWindow("Game", wW, wH) != 0)
        return;

    Running = true;
}
}