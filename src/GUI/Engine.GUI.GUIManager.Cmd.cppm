module;

#include <cstdint>
#include <string>
#include <variant>

export module Engine.GUI.GUIManager.Cmd;

export namespace Engine::GUI {
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
    double angle;
    float acenter_x, acenter_y;
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
struct CmdDrawSVG {
    std::string resname;
    float x, y;
    int w, h;
    float angle, acenter_x, acenter_y;
    int z_order = 0;
};

using RenderCommand = std::variant<
    CmdSetBackground,
    CmdRect,
    CmdDebugText,
    CmdSetTitle,
    CmdSetLogicalSize, CmdDisableLogicalSize, CmdFillRect, CmdText, CmdDrawSVG>;

inline auto get_z_order(const RenderCommand& cmd) -> int
{
    return std::visit([](const auto& c) -> int { return c.z_order; }, cmd);
}
}
