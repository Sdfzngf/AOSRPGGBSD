/**
 * @brief 游戏
 *
 */
module;

#include <atomic>
#include <memory>

export module Engine.Game;

import Engine.Utils.Script.ScriptManager;
import Engine.Utils.Data.DataManager;
import Engine.GUI.GUIManager;

export namespace Engine {
class Game {
private:
    std::atomic<std::shared_ptr<Engine::Utils::Data::DataManager>> DM;
    std::atomic<std::shared_ptr<Engine::Utils::Script::ScriptManager>> SM;
    std::atomic<std::shared_ptr<Engine::GUI::GUIManager>> GM;
    std::atomic<bool> Running = false;
    std::atomic<int> wH, wW;

public:
    void StartUp();
    void MainLoop();
    void ShutDown();
};
}
