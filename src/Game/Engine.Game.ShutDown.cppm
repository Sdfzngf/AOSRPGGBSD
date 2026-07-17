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
auto Engine::Game::ShutDown() -> void
{
    Log("void Engine::Game::ShutDown()");
    // 优雅关闭所有 Worker
    SM.load().get()->ShutdownWorkers();
}
}