/**
 * @brief 无尽的循环，，，
 *
 */
module;

module Engine.Game:MainLoop;

import Engine.Game;
import Engine.Utils.Logger;
import Engine.i18n;
import Engine.Utils.Time;

using Engine::Utils::Logger::Log;

export namespace Engine {
auto Engine::Game::MainLoop() -> void
{
    Log("void Engine::Game::MainLoop()");
    double prevTime = 0.0;
    double deltaTime = 0.0;
    double FPS = 0.0;
    float x = 0.0f, y = 0.0f;
    char xr = 1, yr = 1;
    while (Running) {
        prevTime = Engine::Utils::Time::GetAppRunningTime();
        GM.SetWindowTitle(Engine::i18n::nfmt("Height: {},Width: {}, FPS: {}", wW.load(), wH.load(), static_cast<int>(FPS)));

        GM.SetBackground(100, 100, 100, 255);
        GM.Rect(0, 0, wW, wH, 255, 0, 0, 0);
        GM.Rect(x, y, 92, 28, 0, 0, 0, 0);
        GM.BasicText("DVD", x / 4, y / 4, 255, 255, 255, 255, 4);

        if (x < 0) {
            x = 0;
            xr = 1;
        }
        if (x + 92 > wW) {
            x = wW - 92;
            xr = -1;
        }
        if (y + 28 > wH) {
            y = wH - 28;
            yr = -1;
        }
        if (y < 0) {
            y = 0;
            yr = 1;
        }
        x += deltaTime * xr * 100;
        y += deltaTime * yr * 100;

        GM.Update(Running, wW, wH);
        deltaTime = Engine::Utils::Time::GetAppRunningTime() - prevTime;
        FPS = 1 / deltaTime;
    }
}
}
