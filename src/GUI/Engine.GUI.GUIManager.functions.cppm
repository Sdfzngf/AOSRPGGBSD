module;

#include "SDL3/SDL_iostream.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_surface.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <variant>
#include <vector>

module Engine.GUI.GUIManager:functions;

import Engine.Utils.Logger;
import Engine.Utils.Logger.LogLevel;
import Engine.GUI.GUIManager;
import Engine.GUI.GUIManager.Cmd;
import Engine.Utils.Data.DataEntry.EntryType;

using Engine::Utils::Logger::Log;

export namespace Engine::GUI {
[[nodiscard]] auto GUIManager::Init(std::string guilib) -> int
{
    Log("GUIManager::Init()");
    if (guilib == "SDL") {
        SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            Log([&guilib]() -> std::string { return ::Engine::i18n::fmt("无法初始化{}: {}", "SDL3", SDL_GetError()); }, ::Engine::Utils::Logger::LogLevel::ERROR);
            return 1;
        }
        if (!TTF_Init()) {
            Log([&guilib]() -> std::string { return ::Engine::i18n::fmt("无法初始化{}: {}", "SDL_TTF", SDL_GetError()); }, ::Engine::Utils::Logger::LogLevel::ERROR);
            return 1;
        }
        glb = GUIlib::_l_SDL;
    } else {
        Log([&guilib]() -> std::string { return ::Engine::i18n::fmt("不支持的图形库: {}", guilib); }, ::Engine::Utils::Logger::LogLevel::ERROR);
        return 2;
    }
    Log([&guilib]() -> std::string { return ::Engine::i18n::fmt("初始化图形库 {} 成功", guilib); });
    init = true;
    return 0;
}

auto GUIManager::BindWH(std::atomic<int>* ww, std::atomic<int>* wh) -> void
{
    wW = ww;
    wH = wh;
}

auto GUIManager::BindDM(std::shared_ptr<::Engine::Utils::Data::DataManager> ddmm) -> void
{
    DM_ = ddmm;
}

[[nodiscard]] auto GUIManager::CreateWindow(const std::string& name) -> int
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

        if (!SDL_CreateWindowAndRenderer(name.c_str(), wW->load(), wH->load(), SDL_WINDOW_RESIZABLE,
                                         &rawWindow, &rawRenderer)) {
            Log([]() -> std::string { return ::Engine::i18n::fmt("SDL: 无法创建 window/renderer: {}", SDL_GetError()); }, ::Engine::Utils::Logger::LogLevel::ERROR);
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

auto GUIManager::PushCommand(RenderCommand cmd) -> void
{
    std::lock_guard lock(cmd_mtx_);
    cmd_queue_back_.push_back(std::move(cmd));
}

/// 清空命令队列（不执行）
auto GUIManager::ClearQueue() -> void
{
    std::lock_guard lock(cmd_mtx_);
    cmd_queue_front_.clear();
    cmd_queue_back_.clear();
}

auto GUIManager::FlushCommands() -> void
{
    {
        std::lock_guard lock(cmd_mtx_);
        cmd_queue_front_.swap(cmd_queue_back_);
        cmd_queue_back_.clear(); // 清空旧数据，防止命令累积
    }
    if (cmd_queue_front_.empty())
        return;

    // 多于 1 条命令时才排序
    if (cmd_queue_front_.size() > 1) {
        std::ranges::stable_sort(cmd_queue_front_, std::ranges::less { }, &get_z_order);
    }

    for (const auto& cmd : cmd_queue_front_) {
        std::visit([this](const auto& c) -> void {
            using T = std::decay_t<decltype(c)>;
            if constexpr (std::is_same_v<T, CmdSetBackground>) {
                SetBackground(c.r, c.g, c.b, c.a);
            } else if constexpr (std::is_same_v<T, CmdRect>)
                Rect(c.x, c.y, c.w, c.h, c.r, c.g, c.b, c.a);
            else if constexpr (std::is_same_v<T, CmdDebugText>)
                DebugText(c.s, c.x, c.y, c.r, c.g, c.b, c.a, c.size);
            else if constexpr (std::is_same_v<T, CmdSetTitle>)
                SetWindowTitle(c.title);
            else if constexpr (std::is_same_v<T, CmdSetLogicalSize>)
                SetLogicalSize(c.w, c.h);
            else if constexpr (std::is_same_v<T, CmdDisableLogicalSize>)
                DisableLogicalSize();
            else if constexpr (std::is_same_v<T, CmdFillRect>)
                FillRect(c.x, c.y, c.w, c.h, c.r, c.g, c.b, c.a);
            else if constexpr (std::is_same_v<T, CmdText>)
                Text(c.s, c.font, c.x, c.y, c.r, c.g, c.b, c.a, c.br, c.bg, c.bb, c.ba, c.size, c.quality, c.angle, c.acenter_x, c.acenter_y);
            else if constexpr (std::is_same_v<T, CmdDrawSVG>)
                DrawSVG(c.resname, c.x, c.y, c.w, c.h, c.angle, c.acenter_x, c.acenter_y);
        },
                   cmd);
    }
    cmd_queue_front_.clear(); // 执行完毕，清空
}
auto GUIManager::SetLogicalSize(int w, int h) -> void
{
    switch (glb) {
    case GUIlib::_l_SDL: {
        lW = w;
        lH = h;
        *wW = w;
        *wH = h;
        SDL_SetRenderLogicalPresentation(renderer.load().get(), lW, lH, SDL_LOGICAL_PRESENTATION_LETTERBOX);
        return;
    }
    case GUIlib::_l_nul: {
        return;
    }
    }
}
auto GUIManager::DisableLogicalSize() -> void
{
    switch (glb) {
    case GUIlib::_l_SDL: {
        lW = -1;
        lH = -1;
        int wwww = 640, hhhh = 480;
        SDL_GetWindowSize(window.load().get(), &wwww, &hhhh);
        *wW = wwww;
        *wH = hhhh;
        SDL_SetRenderLogicalPresentation(renderer.load().get(), lW, lH, SDL_LOGICAL_PRESENTATION_DISABLED);
        return;
    }
    case GUIlib::_l_nul: {
        return;
    }
    }
}
auto GUIManager::Update(std::atomic<bool>& running) -> void
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
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                if (lW == -1)
                    if (event.window.windowID == SDL_GetWindowID(window.load().get())) {
                        int www = 0, hhh = 0;
                        SDL_GetWindowSize(window.load().get(), &www, &hhh);
                        wW->store(www);
                        wH->store(hhh);
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

auto GUIManager::SetBackground(uint8_t r, uint8_t g, uint8_t b, uint8_t a) -> void
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

auto GUIManager::SetWindowTitle(const std::string& tl) -> void
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
auto GUIManager::Rect(float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) -> void
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

auto GUIManager::FillRect(float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) -> void
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
        SDL_RenderFillRect(renderer.load().get(), &R);
        return;
    }
    case GUIlib::_l_nul:
        return;
    }
}

auto GUIManager::DebugText(const std::string& s, const float& x, const float& y, uint8_t r, uint8_t g, uint8_t b, uint8_t a, float size) -> void
{
    switch (glb) {
    case GUIlib::_l_SDL: {
        SDL_SetRenderDrawColor(renderer.load().get(), r, g, b, a);
        SDL_SetRenderScale(renderer.load().get(), size, size);
        SDL_RenderDebugText(renderer.load().get(), x / size, y / size, s.c_str());
        SDL_SetRenderScale(renderer.load().get(), 1.0f, 1.0f);
        return;
    }
    case GUIlib::_l_nul:
        return;
    }
}

auto GUIManager::DrawSVG(const std::string& _resname, float _x, float _y, int _w, int _h, float _angle, float _acenter_x, float _acenter_y) -> void
{
    switch (glb) {
    case GUIlib::_l_SDL: {
        struct _c_sdl_svg {
            std::shared_ptr<::Engine::Utils::Data::DataEntry> _c_et;
            std::shared_ptr<SDL_Texture> _c_c;
            std::atomic<int> _c_w = 0, _c_h = 0;
        };
        auto rf = DM_.load()->GetEntry(_resname);
        if (!rf || rf.get()->Type.load() != static_cast<uint8_t>(Engine::Utils::Data::EntryType::svg))
            return;
        const std::string cacheKey = _resname + "::ptr::SDLIMG_SVG_TEXTURE[C]<-NOSNAPSHOT";
        auto sEntry = DM_.load()->GetEntry(cacheKey);
        if (!sEntry) {
            auto et = std::make_shared<::Engine::Utils::Data::DataEntry>();
            et->New<_c_sdl_svg>();
            et->Size = sizeof(_c_sdl_svg);
            et->SetName(cacheKey);
            et->Type.store(static_cast<uint8_t>(Engine::Utils::Data::EntryType::Binary));
            bool success = et->Write([&rf, &_w, &_h, this](std::shared_ptr<uint8_t[]>& d) -> bool {
                auto ptsvg = std::reinterpret_pointer_cast<_c_sdl_svg>(d);
                ptsvg->_c_et = rf;
                SDL_IOStream* sio = SDL_IOFromConstMem(ptsvg->_c_et->Data.load().get(),
                                                       ptsvg->_c_et->Size.load());
                if (!sio) {
                    return false;
                }
                SDL_Surface* sur = IMG_LoadSizedSVG_IO(sio, _w, _h);
                SDL_CloseIO(sio);
                sio = nullptr;
                if (!sur) {
                    return false;
                }
                SDL_Texture* tx = SDL_CreateTextureFromSurface(renderer.load().get(), sur);
                SDL_DestroySurface(sur);
                if (!tx) {
                    return false;
                }
                ptsvg->_c_c = std::shared_ptr<SDL_Texture>(tx, SDL_DestroyTexture);
                ptsvg->_c_w = _w;
                ptsvg->_c_h = _h;
                return true;
            });
            if (!success)
                return;
            DM_.load()->InsertEntry(cacheKey, et);
            sEntry = et;
        }
        auto tsvg = std::reinterpret_pointer_cast<_c_sdl_svg>(sEntry->Data.load());
        if (tsvg->_c_w != _w || tsvg->_c_h != _h || !tsvg) {
            SDL_IOStream* sio = SDL_IOFromConstMem(tsvg->_c_et->Data.load().get(),
                                                   tsvg->_c_et->Size.load());
            if (!sio) {
                return;
            }
            SDL_Surface* sur = IMG_LoadSizedSVG_IO(sio, _w, _h);
            SDL_CloseIO(sio);
            sio = nullptr;
            if (!sur) {
                return;
            }
            SDL_Texture* tx = SDL_CreateTextureFromSurface(renderer.load().get(), sur);
            SDL_DestroySurface(sur);
            if (!tx) {
                return;
            }
            tsvg->_c_c = std::shared_ptr<SDL_Texture>(tx, SDL_DestroyTexture);
            tsvg->_c_w = _w;
            tsvg->_c_h = _h;
        }
        SDL_FRect spos {
            .x = 0,
            .y = 0,
            .w = static_cast<float>(tsvg->_c_c->w),
            .h = static_cast<float>(tsvg->_c_c->h)
        };
        SDL_FRect dpos {
            .x = _x,
            .y = _y,
            .w = static_cast<float>((_w == 0) ? tsvg->_c_c->w : _w),
            .h = static_cast<float>((_h == 0) ? tsvg->_c_c->h : _h)
        };
        SDL_FPoint center {
            .x = _acenter_x,
            .y = _acenter_y
        };
        SDL_RenderTextureRotated(renderer.load().get(), tsvg->_c_c.get(), &spos, &dpos, _angle, &center, SDL_FLIP_NONE);
        return;
    }
    case GUIlib::_l_nul:
        return;
    }
}

auto GUIManager::Text(const std::string& te, const std::string& fname,
                      float _x, float _y,
                      uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a,
                      uint8_t _br, uint8_t _bg, uint8_t _bb, uint8_t _ba,
                      float ptsize,
                      int quality,
                      double angle, float acenter_x, float acenter_y) -> void
{
    switch (glb) {
    case GUIlib::_l_SDL: {
        struct _c_sdl_ttf_font {
            std::shared_ptr<::Engine::Utils::Data::DataEntry> et;
            std::map<float, std::shared_ptr<TTF_Font>> _f;
            std::atomic<bool> _finit = false;
        };

        auto rf = DM_.load()->GetEntry(fname);
        if (!rf || rf.get()->Type.load() != static_cast<uint8_t>(Engine::Utils::Data::EntryType::Font))
            return;

        const std::string cacheKey = fname + "::ptr::SDL_TTF_TEXTURE[C]<-NOSNAPSHOT";
        auto fEntry = DM_.load()->GetEntry(cacheKey);

        // 若缓存条目不存在，则创建
        if (!fEntry) {
            auto et = std::make_shared<::Engine::Utils::Data::DataEntry>();
            et->New<_c_sdl_ttf_font>();
            et->Size = sizeof(_c_sdl_ttf_font);
            et->SetName(cacheKey);
            et->Type.store(static_cast<uint8_t>(Engine::Utils::Data::EntryType::Binary));
            bool success = et->Write([&rf, ptsize](std::shared_ptr<uint8_t[]>& d) -> bool {
                auto fffff = std::reinterpret_pointer_cast<_c_sdl_ttf_font>(d);

                fffff->et = rf;

                SDL_IOStream* sio = SDL_IOFromConstMem(fffff->et->Data.load().get(),
                                                       fffff->et->Size.load());
                if (!sio) {
                    return false;
                }
                TTF_Font* tfb = TTF_OpenFontIO(sio, true, ptsize);
                if (!tfb) {
                    SDL_CloseIO(sio);
                    return false;
                }
                fffff->_f[ptsize] = std::shared_ptr<TTF_Font>(tfb, TTF_CloseFont);
                return true;
            });
            if (!success)
                return;
            DM_.load()->InsertEntry(cacheKey, et);
            fEntry = et; // 继续使用新创建的条目
        }

        // 获取字体对象
        auto fffff = std::reinterpret_pointer_cast<_c_sdl_ttf_font>(fEntry->Data.load());

        // 确保当前字号已缓存
        auto it = fffff->_f.find(ptsize);
        if (it == fffff->_f.end()) {
            // 未缓存，动态创建
            SDL_IOStream* sio = SDL_IOFromConstMem(fffff->et->Data.load().get(),
                                                   fffff->et->Size.load());
            if (!sio)
                return;
            TTF_Font* tfb = TTF_OpenFontIO(sio, true, ptsize);
            if (!tfb) {
                return;
            }
            auto newFont = std::shared_ptr<TTF_Font>(tfb, TTF_CloseFont);
            auto [insertIt, inserted] = fffff->_f.emplace(ptsize, newFont);
            it = insertIt;
        }

        // 获取字体原始指针
        TTF_Font* fontPtr = it->second.get();

        // 渲染文本
        SDL_Color c { .r = _r, .g = _g, .b = _b, .a = _a };
        SDL_Color bc { .r = _br, .g = _bg, .b = _bb, .a = _ba };
        SDL_Surface* sur = nullptr;

        switch (quality) {
        default:
            [[fallthrough]];
        case 0:
            sur = TTF_RenderText_Solid(fontPtr, te.c_str(), te.length(), c);
            break;
        case 1:
            sur = TTF_RenderText_Shaded(fontPtr, te.c_str(), te.length(), c, bc);
            break;
        case 2:
            sur = TTF_RenderText_Blended(fontPtr, te.c_str(), te.length(), c);
            break;
        case 3:
            sur = TTF_RenderText_LCD(fontPtr, te.c_str(), te.length(), c, bc);
            break;
        }

        if (sur) {
            if (auto tex = SDL_CreateTextureFromSurface(renderer.load().get(), sur)) {
                SDL_FRect spos {
                    .x = 0,
                    .y = 0,
                    .w = static_cast<float>(tex->w),
                    .h = static_cast<float>(tex->h)
                };
                SDL_FRect dpos {
                    .x = _x,
                    .y = _y,
                    .w = static_cast<float>(tex->w),
                    .h = static_cast<float>(tex->h)
                };
                SDL_FPoint center {
                    .x = acenter_x,
                    .y = acenter_y
                };
                SDL_RenderTextureRotated(renderer.load().get(), tex, &spos, &dpos, angle, &center, SDL_FLIP_NONE);
                SDL_DestroyTexture(tex);
            }
            SDL_DestroySurface(sur);
        }
        return;
    }
    case GUIlib::_l_nul:
        return;
    }
}
}
