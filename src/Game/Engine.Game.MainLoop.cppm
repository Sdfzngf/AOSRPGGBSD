/**
 * @brief 无尽的循环，，，
 *
 */
module;

module Engine.Game:MainLoop;

import Engine.Game;
import Engine.Utils.Logger;

using Engine::Utils::Logger::Log;

export namespace Engine {
auto Engine::Game::MainLoop() -> void
{
    Log("void Engine::Game::MainLoop()");
}
}
