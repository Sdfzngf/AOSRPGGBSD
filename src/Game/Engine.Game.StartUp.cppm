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
import Engine.GUI.GUIManager;
import Engine.Sound.SoundManager;
import Engine.i18n;

using Engine::Utils::Logger::Log;

export namespace Engine {

auto Engine::Game::StartUp() -> void
{
    Log("void Engine::Game::StartUp()");

    DM.store(std::make_shared<Engine::Utils::Data::DataManager>());
    SM.store(std::make_shared<Engine::Utils::Script::ScriptManager>());
    GM.store(std::make_shared<Engine::GUI::GUIManager>());
    MM.store(std::make_shared<Engine::Sound::SoundManager>());

    SM.load()->BindDataManager(DM);
    SM.load()->BindGUIManager(GM);
    GM.load()->BindDM(DM.load());
    MM.load()->BindDM(DM.load());
    SM.load()->BindSndManager(MM);
    GM.load()->BindMM(MM.load());

    std::string myth = Engine::Basics::Random::rand_str(256);
    DM.load()->CreateSnapshotAll(myth);
    DM.load()->MountDB("./Lang/Lang.dat");

    std::fstream file("./Lang/config.txt");
    std::string lang = "zh-CN";
    if (file) {
        std::getline(file, lang);
    }
    file.close();

    lang = std::string("__EG_i18n__@") + lang;

    auto i18nEntry = DM.load()->GetEntry(lang);
    if (i18nEntry) {
        i18nEntry->Read([&](const std::shared_ptr<uint8_t[]>& data) -> void {
            std::string jsonContent(reinterpret_cast<const char*>(data.get()), i18nEntry->Size.load());
            Engine::i18n::Init("zh-CN", jsonContent);
        });
    }

    SM.load()->OpenLibs();
    SM.load()->SetupMainDMAPI();
    SM.load()->SetupSndLuaAPI();
    SM.load()->SetupGUILuaAPI();

    DM.load()->MountDB("./Test/worker.dat");
    DM.load()->MountDB("./Game/BGM.dat");
    DM.load()->MountDB("./Game/SFX.dat");
    DM.load()->MountDB("./Game/startup.dat");
    DM.load()->MountDB("./Game/fonts.dat");
    // SM.load().get()->RunScript(std::string("__Engine_Test_Worker__@workertest.lua"));

    if (GM.load()->Init("SDL") != 0)
        return;

    MM.load()->Init();
    int result = MM.load()->LoadSound("__Engine_BGM__@ITERATION.wav", "testWav");
    if (result == 0) {
        int re2 = MM.load()->CreateTrack("TestTrack");
        if (re2 == 0) {
            int re3 = MM.load()->SetTrackAudio("TestTrack", "testWav");
            if (re3 == 0) {
                int re4 = MM.load()->PlayLoopTrack("TestTrack", -1);
            }
        }
    }
    int resu2 = MM.load()->PlaySoundEffect("__Engine_SFX__@end.mp3");
    wW = 640;
    wH = 480;

    GM.load()->BindWH(&wW, &wH);

    if (GM.load()->CreateWindow("Game") != 0)
        return;

    GM.load()->SetLogicalSizeM(1280, 720);

    SM.load()->RunScript(std::string("__Engine_StartUp__@startup.lua"));

    GM.load()->FlushCommands();

    Running = true;

    DM.load()->MountDB("./Game/background.dat");
    SM.load()->CreateWorker("background_renderer", "__Engine_Background__@renderer.lua");
}
}
