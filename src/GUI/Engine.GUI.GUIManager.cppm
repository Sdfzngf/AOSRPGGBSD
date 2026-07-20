/**
 * @brief GUI模块
 *
 */
module;

#include "SDL3/SDL_iostream.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"
#include <SDL3/SDL.h>
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

export module Engine.GUI.GUIManager;

import Engine.Utils.Logger;
import Engine.Utils.Logger.LogLevel;
import Engine.i18n;
import Engine.Utils.Data.DataManager;
import Engine.Utils.Data.DataEntry;

using Engine::Utils::Logger::Log;

export namespace Engine::GUI {
enum class GUIlib : uint8_t {
    _l_nul = 0,
    _l_SDL = 1
};

// ── 命令队列 ──

struct CmdSetBackground {
    uint8_t r, g, b, a;
    int z_order = 0;
};
struct CmdRect {
    float x, y, w, h;
    uint8_t r, g, b, a;
    int z_order = 0;
};
struct CmdDebugText {
    std::string s;
    float x, y;
    uint8_t r, g, b, a;
    float size;
    int z_order = 0;
};
struct CmdText {
    std::string s;
    std::string font;
    float x, y;
    uint8_t r, g, b, a;
    uint8_t br, bg, bb, ba;
    float size;
    int quality;
    int z_order = 0;
};
struct CmdSetTitle {
    std::string title;
    int z_order = 0;
};
struct CmdSetLogicalSize {
    int w, h;
    int z_order = 0;
};
struct CmdDisableLogicalSize {
    int z_order = 0;
};
struct CmdFillRect {
    float x, y, w, h;
    uint8_t r, g, b, a;
    int z_order = 0;
};

using RenderCommand = std::variant<
    CmdSetBackground,
    CmdRect,
    CmdDebugText,
    CmdSetTitle,
    CmdSetLogicalSize, CmdDisableLogicalSize, CmdFillRect, CmdText>;

inline auto get_z_order(const RenderCommand& cmd) -> int
{
    return std::visit([](const auto& c) -> int { return c.z_order; }, cmd);
}

class GUIManager {
private:
    bool init = false;
    std::atomic<std::shared_ptr<SDL_Window>> window;
    std::atomic<std::shared_ptr<SDL_Renderer>> renderer;
    GUIlib glb = GUIlib::_l_nul;
    int lW = -1, lH = -1;
    std::atomic<int>* wW; // NOLINT
    std::atomic<int>* wH; // NOLINT
    std::atomic<std::shared_ptr<Engine::Utils::Data::DataManager>> DM_;

    // 双缓冲命令队列（线程安全，生产者写 back，消费者读 front）
    std::vector<RenderCommand> cmd_queue_front_;
    std::vector<RenderCommand> cmd_queue_back_;
    mutable std::mutex cmd_mtx_;

public:
    [[nodiscard]] auto Init(std::string guilib = "SDL") -> int
    {
        Log("GUIManager::Init()");
        if (guilib == "SDL") {
            SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
            if (!SDL_Init(SDL_INIT_VIDEO)) {
                Log([&guilib]() -> std::string { return Engine::i18n::fmt("无法初始化{}: {}", "SDL3", SDL_GetError()); }, Engine::Utils::Logger::LogLevel::ERROR);
                return 1;
            }
            if (!TTF_Init()) {
                Log([&guilib]() -> std::string { return Engine::i18n::fmt("无法初始化{}: {}", "SDL_TTF", SDL_GetError()); }, Engine::Utils::Logger::LogLevel::ERROR);
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

    auto BindWH(std::atomic<int>* ww, std::atomic<int>* wh) -> void
    {
        wW = ww;
        wH = wh;
    }

    auto BindDM(std::shared_ptr<Engine::Utils::Data::DataManager> ddmm) -> void
    {
        DM_ = ddmm;
    }

    [[nodiscard]] auto CreateWindow(const std::string& name) -> int
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

    // ── 命令队列接口（任意线程安全） ──

    /// 从任意线程推入渲染命令（写入 back 缓冲）
    auto PushCommand(RenderCommand cmd) -> void
    {
        std::lock_guard lock(cmd_mtx_);
        cmd_queue_back_.push_back(std::move(cmd));
    }

    /// 清空命令队列（不执行）
    auto ClearQueue() -> void
    {
        std::lock_guard lock(cmd_mtx_);
        cmd_queue_front_.clear();
        cmd_queue_back_.clear();
    }

    /// 消费全部命令（必须在主线程调用），swap 前后缓冲后按 z_order 排序执行
    auto FlushCommands() -> void
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
                    // int ww = 0, hh = 0;
                    // SDL_GetWindowSize(window.load().get(), &ww, &hh);
                    // SDL_FRect bg { .x = 0, .y = 0, .w = (float)ww, .h = (float)hh };
                    // SDL_SetRenderDrawColor(renderer.load().get(), c.r, c.g, c.b, c.a);
                    // SDL_RenderFillRect(renderer.load().get(), &bg);
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
                    Text(c.s, c.font, c.x, c.y, c.r, c.g, c.b, c.a, c.br, c.bg, c.bb, c.ba, c.size, c.quality);
            },
                       cmd);
        }
        cmd_queue_front_.clear(); // 执行完毕，清空
    }

    // ── 命令队列推送接口（C++ 调用，推入队列由 FlushCommands 消费） ──

    auto SetBackgroundM(uint8_t r, uint8_t g, uint8_t b, uint8_t a, int z = 0) -> void
    {
        PushCommand(CmdSetBackground { .r = r, .g = g, .b = b, .a = a, .z_order = z });
    }
    auto RectM(float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a, int z = 0) -> void
    {
        PushCommand(CmdRect { .x = x, .y = y, .w = w, .h = h, .r = r, .g = g, .b = b, .a = a, .z_order = z });
    }
    auto DebugTextM(const std::string& s, float x, float y, uint8_t r, uint8_t g, uint8_t b, uint8_t a, float size, int z = 0) -> void
    {
        PushCommand(CmdDebugText { .s = s, .x = x, .y = y, .r = r, .g = g, .b = b, .a = a, .size = size, .z_order = z });
    }
    auto SetTitleM(const std::string& title, int z = 0) -> void
    {
        PushCommand(CmdSetTitle { .title = title, .z_order = z });
    }
    auto SetLogicalSizeM(int w, int h, int z = 0) -> void
    {
        PushCommand(CmdSetLogicalSize { .w = w, .h = h, .z_order = z });
    }
    auto DisableLogicalSizeM() -> void
    {
        PushCommand(CmdDisableLogicalSize { });
    }
    auto FillRectM(float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a, int z = 0) -> void
    {
        PushCommand(CmdFillRect { .x = x, .y = y, .w = w, .h = h, .r = r, .g = g, .b = b, .a = a, .z_order = z });
    }
    auto TextM(const std::string& te, const std::string& fname,
               float _x, float _y,
               uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a,
               uint8_t _br, uint8_t _bg, uint8_t _bb, uint8_t _ba,
               float ptsize,
               int quality, int z = 0) -> void
    {
        PushCommand(CmdText { .s = te, .font = fname, .x = _x, .y = _y, .r = _r, .g = _g, .b = _b, .a = _a, .br = _br, .bg = _bg, .bb = _bb, .ba = _ba, .size = ptsize, .quality = quality, .z_order = z });
    }

    // ── 直接渲染接口（主线程 C++ 调用，不走队列） ──

    auto SetLogicalSize(int w, int h) -> void
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

    auto DisableLogicalSize() -> void
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

    auto Update(std::atomic<bool>& running) -> void
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

    auto FillRect(float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) -> void
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

    auto DebugText(const std::string& s, const float& x, const float& y, uint8_t r, uint8_t g, uint8_t b, uint8_t a, float size) -> void
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

    auto Text(const std::string& te, const std::string& fname,
              float _x, float _y,
              uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a,
              uint8_t _br, uint8_t _bg, uint8_t _bb, uint8_t _ba,
              float ptsize,
              int quality) -> void
    {
        switch (glb) {
        case GUIlib::_l_SDL: {
            struct font {
                std::shared_ptr<Engine::Utils::Data::DataEntry> et;
                std::map<float, std::shared_ptr<TTF_Font>> _f;
                bool _finit = false;
            };

            auto rf = DM_.load()->GetEntry(fname);
            if (!rf)
                return;

            const std::string cacheKey = fname + "::ptr::SDL_TTF<-NOSNAPSHOT";
            auto fEntry = DM_.load()->GetEntry(cacheKey);

            // 若缓存条目不存在，则创建
            if (!fEntry) {
                auto et = std::make_shared<Engine::Utils::Data::DataEntry>();
                et->New<font>();
                bool success = et->Write([&rf, ptsize](std::shared_ptr<uint8_t[]>& d) -> bool {
                    auto fffff = std::reinterpret_pointer_cast<font>(d).get();

                    fffff->et = rf;

                    SDL_IOStream* sio = SDL_IOFromConstMem(fffff->et->Data.load().get(),
                                                           fffff->et->Size.load());
                    if (!sio) {
                        // fffff->~font(); // 必须手动析构
                        return false;
                    }
                    TTF_Font* tfb = TTF_OpenFontIO(sio, true, ptsize);
                    if (!tfb) {
                        SDL_CloseIO(sio);
                        // fffff->~font();
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
            auto fffff = reinterpret_cast<font*>(fEntry->Data.load().get());

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
                    SDL_CloseIO(sio);
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
                    SDL_RenderTexture(renderer.load().get(), tex, &spos, &dpos);
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
};

}
