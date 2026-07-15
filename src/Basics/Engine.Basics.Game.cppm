/**
 * @brief 游戏
 *
 */
module;

#include <atomic>

export module Engine.Game;

import Engine.Utils.Script.ScriptManager;
import Engine.Utils.Data.DataManager;
import Engine.GUI.GUIManager;

export namespace Engine {
class Game {
private:
    Engine::Utils::Data::DataManager DM;
    Engine::Utils::Script::ScriptManager SM;
    Engine::GUI::GUIManager GM;
    std::atomic<bool> Running = false;
    std::atomic<int> wH, wW;

public:
    void StartUp();
    void MainLoop();
    void ShutDown();
};
}
