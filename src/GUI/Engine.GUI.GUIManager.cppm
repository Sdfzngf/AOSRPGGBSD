/**
 * @brief GUI模块
 *
 */
module;

#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"
#include <SDL3/SDL.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

export module Engine.GUI.GUIManager;

import Engine.Utils.Logger;
import Engine.Utils.Logger.LogLevel;
import Engine.i18n;

using Engine::Utils::Logger::Log;

export namespace Engine::GUI {
enum class GUIlib : uint8_t {
    _l_nul = 0,
    _l_SDL = 1
};

class GUIManager {
private:
    bool init = false;
    std::atomic<std::shared_ptr<SDL_Window>> window;
    std::atomic<std::shared_ptr<SDL_Renderer>> renderer;
    GUIlib glb = GUIlib::_l_nul;
    int lW = -1, lH = -1;

public:
    [[nodiscard]] auto Init(std::string guilib = "SDL") -> int
    {
        Log("GUIManager::Init()");
        if (guilib == "SDL") {
            SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
            if (!SDL_Init(SDL_INIT_VIDEO)) {
                Log([&guilib]() -> std::string { return Engine::i18n::fmt("无法初始化{}: {}", guilib, SDL_GetError()); }, Engine::Utils::Logger::LogLevel::ERROR);
                return 1;
            }
            glb = GUIlib::_l_SDL;
        } else {
            Log([&guilib]() -> std::string { return Engine::i18n::fmt("不支持的图形库: {}", guilib); }, Engine::Utils::Logger::LogLevel::ERROR);
            return 2;
        }
        Log([&guilib]() -> std::string { return Engine::i18n::fmt("初始化图形库 {} 成功", guilib); });
        init = true;
        return 0;
    }

    [[nodiscard]] auto CreateWindow(const std::string& name, const int& width, const int& height) -> int
    {
        if (!init)
            if (Init() != 0)
                return 2;

        switch (glb) {
        case GUIlib::_l_SDL: {
            window = nullptr;
            renderer = nullptr;

            SDL_Window* rawWindow = nullptr;
            SDL_Renderer* rawRenderer = nullptr;

            if (!SDL_CreateWindowAndRenderer(name.c_str(), width, height, SDL_WINDOW_RESIZABLE,
                                             &rawWindow, &rawRenderer)) {
                Log([]() -> std::string { return Engine::i18n::fmt("SDL: 无法创建 window/renderer: {}", SDL_GetError()); }, Engine::Utils::Logger::LogLevel::ERROR);
                return 1;
            }

            window = std::shared_ptr<SDL_Window>(rawWindow, SDL_DestroyWindow);
            renderer = std::shared_ptr<SDL_Renderer>(rawRenderer, SDL_DestroyRenderer);
            break;
        }
        case GUIlib::_l_nul:
            return 3;
        }
        return 0;
    }

    auto SetLogicalSize(int w, int h)
    {
        switch (glb) {
        case GUIlib::_l_SDL: {
            lW = w;
            lH = h;
            SDL_SetRenderLogicalPresentation(renderer.load().get(), lW, lH, SDL_LOGICAL_PRESENTATION_LETTERBOX);
            return;
        }
        case GUIlib::_l_nul: {
            return;
        }
        }
    }

    auto Update(std::atomic<bool>& running, std::atomic<int>& ww, std::atomic<int>& wh) -> void
    {
        switch (glb) {
        case GUIlib::_l_SDL: {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_WINDOW_EXPOSED:
                    // SDL_SetRenderDrawColor(renderer.load().get(), 255, 0, 255, 255);
                    // SDL_RenderClear(renderer.load().get());
                    //
                    // SDL_RenderPresent(renderer.load().get());
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    if (lW == -1)
                        if (event.window.windowID == SDL_GetWindowID(window.load().get())) {
                            int www = 0, hhh = 0;
                            SDL_GetWindowSize(window.load().get(), &www, &hhh);
                            ww = www;
                            wh = hhh;
                        }
                    break;
                default:
                    break;
                }
            }
            SDL_RenderPresent(renderer.load().get());
            break;
        }
        case GUIlib::_l_nul:
            running = false;
            return;
        }
    }

    auto SetBackground(uint8_t r, uint8_t g, uint8_t b, uint8_t a) -> void
    {
        switch (glb) {
        case GUIlib::_l_SDL: {
            SDL_SetRenderDrawColor(renderer.load().get(), r, g, b, a);
            SDL_RenderClear(renderer.load().get());
            return;
        }
        case GUIlib::_l_nul:
            return;
        }
    }

    auto SetWindowTitle(const std::string& tl) -> void
    {
        switch (glb) {
        case GUIlib::_l_SDL: {
            SDL_SetWindowTitle(window.load().get(), tl.c_str());
            return;
        }
        case GUIlib::_l_nul:
            return;
        }
    }

    auto Rect(float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) -> void
    {
        switch (glb) {
        case GUIlib::_l_SDL: {
            SDL_FRect R {
                .x = x,
                .y = y,
                .w = w,
                .h = h
            };
            SDL_SetRenderDrawColor(renderer.load().get(), r, g, b, a);
            SDL_RenderRect(renderer.load().get(), &R);
            return;
        }
        case GUIlib::_l_nul:
            return;
        }
    }

    auto BasicText(const std::string& s, const float& x, const float& y, uint8_t r, uint8_t g, uint8_t b, uint8_t a, float size)
    {
        switch (glb) {
        case GUIlib::_l_SDL: {
            SDL_SetRenderDrawColor(renderer.load().get(), r, g, b, a);
            SDL_SetRenderScale(renderer.load().get(), size, size);
            SDL_RenderDebugText(renderer.load().get(), x, y, s.c_str());
            SDL_SetRenderScale(renderer.load().get(), 1.0f, 1.0f);
            return;
        }
        case GUIlib::_l_nul:
            return;
        }
    }
};
}
