/**
 * @brief GUI模块
 *
 */
module;

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

export module Engine.GUI.GUIManager;

import Engine.Utils.Logger;
import Engine.Utils.Logger.LogLevel;
import Engine.i18n;
import Engine.Utils.Data.DataManager;
import Engine.Utils.Data.DataEntry;
import Engine.GUI.GUIManager.Cmd;
import Engine.Sound.SoundManager;

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
    std::atomic<int>* wW; // NOLINT
    std::atomic<int>* wH; // NOLINT
    std::atomic<std::shared_ptr<Engine::Utils::Data::DataManager>> DM_;
    std::atomic<std::shared_ptr<Engine::Sound::SoundManager>> MM_;

    // 双缓冲命令队列（线程安全，生产者写 back，消费者读 front）
    std::vector<RenderCommand> cmd_queue_front_;
    std::vector<RenderCommand> cmd_queue_back_;
    mutable std::mutex cmd_mtx_;

public:
    [[nodiscard]] auto Init(std::string guilib = "SDL") -> int;

    auto BindWH(std::atomic<int>* ww, std::atomic<int>* wh) -> void;

    auto BindDM(std::shared_ptr<Engine::Utils::Data::DataManager> ddmm) -> void;

    auto BindMM(std::shared_ptr<::Engine::Sound::SoundManager> mm) -> void;

    [[nodiscard]] auto CreateWindow(const std::string& name) -> int;

    /// 从任意线程推入渲染命令（写入 back 缓冲）
    auto PushCommand(RenderCommand cmd) -> void;

    /// 清空命令队列（不执行）
    auto ClearQueue() -> void;

    /// 消费全部命令（必须在主线程调用），swap 前后缓冲后按 z_order 排序执行
    auto FlushCommands() -> void;

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
               float _ptsize,
               int _quality,
               float _angle, float _acenter_x, float _acenter_y,
               int _z = 0) -> void
    {
        PushCommand(CmdText { .s = te, .font = fname, .x = _x, .y = _y, .r = _r, .g = _g, .b = _b, .a = _a, .br = _br, .bg = _bg, .bb = _bb, .ba = _ba, .size = _ptsize, .quality = _quality, .angle = _angle, .acenter_x = _acenter_x, .acenter_y = _acenter_y, .z_order = _z });
    }
    auto DrawSVGM(const std::string& _resname, float _x, float _y, int _w, int _h, float _angle, float _acenter_x, float _acenter_y, int _z)
    {
        PushCommand(CmdDrawSVG { .resname = _resname, .x = _x, .y = _y, .w = _w, .h = _h, .angle = _angle, .acenter_x = _acenter_x, .acenter_y = _acenter_y, .z_order = _z });
    }

    auto SetLogicalSize(int w, int h) -> void;

    auto DisableLogicalSize() -> void;

    auto Update(std::atomic<bool>& running) -> void;

    auto SetBackground(uint8_t r, uint8_t g, uint8_t b, uint8_t a) -> void;

    auto SetWindowTitle(const std::string& tl) -> void;

    auto Rect(float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) -> void;

    auto FillRect(float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) -> void;

    auto DebugText(const std::string& s, const float& x, const float& y, uint8_t r, uint8_t g, uint8_t b, uint8_t a, float size) -> void;

    auto Text(const std::string& te, const std::string& fname,
              float _x, float _y,
              uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a,
              uint8_t _br, uint8_t _bg, uint8_t _bb, uint8_t _ba,
              float ptsize,
              int quality,
              double angle, float acenter_x, float acenter_y) -> void;

    auto DrawSVG(const std::string& _resname, float _x, float _y, int _w, int _h, float _angle, float _acenter_x, float _acenter_y) -> void;
};

}
