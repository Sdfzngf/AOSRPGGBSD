/**
 * @brief 关闭游戏
 * 
 */
module;

module Engine.Game:ShutDown;

import Engine.Game;
import Engine.Utils.Logger;

using Engine::Utils::Logger::Log;

export namespace Engine {
    void Engine::Game::ShutDown(){
        Log("void Engine::Game::ShutDown()");
    }
}
