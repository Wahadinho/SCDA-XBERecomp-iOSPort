#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
#include "app/app.h"
#include "core/input.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <SDL3/SDL.h>
#include <imgui.h>

namespace app {
#if defined(__APPLE__) && TARGET_OS_IPHONE
namespace {

enum Kind { kButton = 0, kStickLeft = 1, kStickRight = 2, kDpad = 3 };
constexpr uint32_t kUp = 0x1, kDown = 0x2, kLeft = 0x4, kRight = 0x8;
constexpr uint32_t kStart = 0x10, kBack = 0x20, kLs = 0x40, kRs = 0x80;
constexpr uint32_t kLb = 0x100, kRb = 0x200;
constexpr uint32_t kA = 0x1000, kB = 0x2000, kX = 0x4000, kY = 0x8000;
constexpr uint32_t kLt = 0x10000, kRt = 0x20000;

struct ControlDef {
    Kind kind;
    uint32_t mask;
    const char* label;
    const char* title;
    ImU32 color;
    float fx, fy, fr;
};

constexpr ControlDef kDefaultControls[] = {
    {kButton, kLt, "LT", "LT", IM_COL32(255, 255, 255, 255), 0.065f, 0.25f, 0.062f},
    {kButton, kLb, "LB", "LB", IM_COL32(255, 255, 255, 255), 0.175f, 0.25f, 0.058f},
    {kStickLeft, 0, "", "Left Stick", IM_COL32(255, 255, 255, 255), 0.145f, 0.64f, 0.150f},
    {kButton, kLs, "LS", "LS", IM_COL32(255, 255, 255, 255), 0.065f, 0.89f, 0.052f},
    {kDpad, 0, "", "D-Pad", IM_COL32(255, 255, 255, 255), 0.285f, 0.82f, 0.125f},
    {kButton, kBack, "BACK", "BACK", IM_COL32(255, 255, 255, 255), 0.36f, 0.075f, 0.046f},
    {kButton, kStart, "START", "START", IM_COL32(255, 255, 255, 255), 0.64f, 0.075f, 0.046f},
    {kButton, kRb, "RB", "RB", IM_COL32(255, 255, 255, 255), 0.825f, 0.25f, 0.058f},
    {kButton, kRt, "RT", "RT", IM_COL32(255, 255, 255, 255), 0.935f, 0.25f, 0.062f},
    {kButton, kY, "Y", "Y", IM_COL32(242, 199, 68, 255), 0.875f, 0.385f, 0.056f},
    {kButton, kX, "X", "X", IM_COL32(62, 142, 234, 255), 0.815f, 0.485f, 0.056f},
    {kButton, kB, "B", "B", IM_COL32(229, 86, 75, 255), 0.935f, 0.485f, 0.056f},
    {kButton, kA, "A", "A", IM_COL32(98, 196, 84, 255), 0.875f, 0.585f, 0.056f},
    {kStickRight, 0, "", "Right Stick", IM_COL32(255, 255, 255, 255), 0.695f, 0.77f, 0.150f},
    {kButton, kRs, "RS", "RS", IM_COL32(255, 255, 255, 255), 0.935f, 0.89f, 0.052f},
};

constexpr int kNumControls = int(std::size(kDefaultControls));
struct ControlState { float fx, fy, scale; };
ControlState g_ctrl[kNumControls];
bool g_loaded = false, g_hidden = false, g_edit_mode = false, g_custom_pos = false;
float g_touch_scale = 1.0f;
int g_touch_opacity = 65;
int g_drag_idx = -1, g_selected_idx = 12;
ImVec2 g_drag_offset(0, 0);
bool g_prev_touch_down = false;

std::string LayoutFile() { return (DataDir() / "touch_layout.cfg").string(); }

void SaveLayout() {
    std::ofstream f(LayoutFile());
    if (!f) return;
    f << "scale " << g_touch_scale << "\nopacity " << g_touch_opacity << "\ncustom " << int(g_custom_pos) << "\n";
    for (int i = 0; i < kNumControls; i++)
        f << "ctrl " << i << " " << g_ctrl[i].fx << " " << g_ctrl[i].fy << " " << g_ctrl[i].scale << "\n";
}

void ResetLayout() {
    for (int i = 0; i < kNumControls; i++)
        g_ctrl[i] = {kDefaultControls[i].fx, kDefaultControls[i].fy, 1.0f};
    g_touch_scale = 1.0f;
    g_touch_opacity = 65;
    g_custom_pos = false;
    SaveLayout();
}

void EnsureLoaded() {
    if (g_loaded) return;
    g_loaded = true;
    for (int i = 0; i < kNumControls; i++)
        g_ctrl[i] = {kDefaultControls[i].fx, kDefaultControls[i].fy, 1.0f};
    int base_op = input::TouchOpacity();
    if (base_op > 0) g_touch_opacity = base_op;
    std::ifstream f(LayoutFile());
    if (!f) return;
    std::string line;
    while (std::getline(f, line)) {
        std::istringstream iss(line);
        std::string key;
        iss >> key;
        if (key == "scale") iss >> g_touch_scale;
        else if (key == "opacity") iss >> g_touch_opacity;
        else if (key == "custom") { int c = 0; iss >> c; g_custom_pos = (c != 0); }
        else if (key == "ctrl") {
            int idx = -1; float x = 0, y = 0, sc = 1.0f;
            if (iss >> idx >> x >> y && idx >= 0 && idx < kNumControls) {
                if (!(iss >> sc)) sc = 1.0f;
                g_ctrl[idx] = {std::clamp(x, 0.02f, 0.98f), std::clamp(y, 0.04f, 0.96f), std::clamp(sc, 0.50f, 1.80f)};
            }
        }
    }
    g_touch_scale = std::clamp(g_touch_scale, 0.55f, 1.50f);
    g_touch_opacity = std::clamp(g_touch_opacity, 15, 100);
}

struct ActiveFinger { int control_idx = -1; uint64_t down_ms = 0; };
std::unordered_map<SDL_FingerID, ActiveFinger> g_fingers;
uint32_t g_linger_buttons = 0;
uint64_t g_linger_until = 0, g_top_btn_cooldown = 0;

int16_t AxisFromNorm(float v) {
    return int16_t(std::lround(std::clamp(v, -1.0f, 1.0f) * 32767.0f));
}

}  // namespace

void DrawTouchControls() {
    EnsureLoaded();
    (void)input::TouchOpacity();

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 disp = io.DisplaySize;
    if (SDL_Window* w = SDL_GetKeyboardFocus() ? SDL_GetKeyboardFocus() : SDL_GetMouseFocus()) {
        int ww = 0, wh = 0;
        if (SDL_GetWindowSize(w, &ww, &wh) && ww > 0 && wh > 0) disp = ImVec2(float(ww), float(wh));
    }
    if (disp.x <= 1.0f || disp.y <= 1.0f) return;

    float ref_h = std::min(disp.y, 430.0f) * g_touch_scale;
    float safe_pad_x = (disp.x / disp.y > 1.65f) ? disp.x * 0.032f : disp.x * 0.015f;
    float usable_w = disp.x - 2.0f * safe_pad_x;

    auto center_of = [&](int idx) {
        if (!g_custom_pos && idx >= 9 && idx <= 12) {
            ImVec2 hub(safe_pad_x + 0.875f * usable_w, 0.49f * disp.y);
            float arm = 0.135f * ref_h;
            if (idx == 9) return ImVec2(hub.x, hub.y - arm);
            if (idx == 10) return ImVec2(hub.x - arm, hub.y);
            if (idx == 11) return ImVec2(hub.x + arm, hub.y);
            return ImVec2(hub.x, hub.y + arm);
        }
        return ImVec2(safe_pad_x + g_ctrl[idx].fx * usable_w, g_ctrl[idx].fy * disp.y);
    };
    auto radius_of = [&](int idx) { return kDefaultControls[idx].fr * ref_h * g_ctrl[idx].scale; };

    std::vector<std::pair<SDL_FingerID, ImVec2>> touch_list;
    std::unordered_map<SDL_FingerID, ImVec2> pts;
    int dev_count = 0;
    if (SDL_TouchID* devs = SDL_GetTouchDevices(&dev_count)) {
        for (int d = 0; d < dev_count; d++) {
            int f_count = 0;
            if (SDL_Finger** fingers = SDL_GetTouchFingers(devs[d], &f_count)) {
                for (int f = 0; f < f_count; f++) {
                    if (fingers[f]) {
                        ImVec2 p(fingers[f]->x * disp.x, fingers[f]->y * disp.y);
                        pts[fingers[f]->id] = p;
                        touch_list.emplace_back(fingers[f]->id, p);
                    }
                }
                SDL_free(fingers);
            }
        }
        SDL_free(devs);
    }

    if (MenuOpen()) {
        bool down = !touch_list.empty();
        if (down) {
            ImVec2 pt = touch_list[0].second;
            io.AddMousePosEvent(pt.x, pt.y);
        }
        if (down != g_prev_touch_down) {
            io.AddMouseButtonEvent(0, down);
            g_prev_touch_down = down;
        }
    } else if (g_prev_touch_down) {
        io.AddMouseButtonEvent(0, false);
        g_prev_touch_down = false;
    }

    uint64_t now = SDL_GetTicks();
    float top_h = std::clamp(ref_h * 0.072f, 24.0f, 34.0f);
    float top_w = top_h * 2.35f, top_gap = top_h * 0.35f;
    float top_y = std::max(8.0f, disp.y * 0.018f);
    float start_top_x = (disp.x - (top_w * 2.0f + top_gap)) * 0.5f;

    ImVec2 menu_min(start_top_x, top_y), menu_max(start_top_x + top_w, top_y + top_h);
    ImVec2 hide_min(start_top_x + top_w + top_gap, top_y), hide_max(hide_min.x + top_w, top_y + top_h);

    auto hit_box = [](ImVec2 p, ImVec2 a, ImVec2 b) {
        return p.x >= a.x - 6.0f && p.x <= b.x + 6.0f && p.y >= a.y - 6.0f && p.y <= b.y + 6.0f;
    };

    bool menu_pressed = false, hide_pressed = false;
    for (const auto& [fid, pt] : touch_list) {
        if (hit_box(pt, menu_min, menu_max)) menu_pressed = true;
        if (hit_box(pt, hide_min, hide_max)) hide_pressed = true;
    }

    if (now > g_top_btn_cooldown) {
        if (menu_pressed) {
            g_top_btn_cooldown = now + 320;
            if (MenuOpen() && g_edit_mode) { g_edit_mode = false; SaveLayout(); }
            ToggleMenu();
        } else if (hide_pressed && !MenuOpen()) {
            g_top_btn_cooldown = now + 320;
            g_hidden = !g_hidden;
            g_fingers.clear();
        }
    }

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    float bar_alpha = g_hidden ? 0.32f : 0.68f;
    auto draw_top_pill = [&](ImVec2 a, ImVec2 b, const char* txt, bool active) {
        dl->AddRectFilled(a, b, IM_COL32(12, 16, 24, int((active ? 220 : 165) * bar_alpha)), top_h * 0.35f);
        dl->AddRect(a, b, IM_COL32(255, 255, 255, int((active ? 230 : 120) * bar_alpha)), top_h * 0.35f, 0, 1.5f);
        ImVec2 ts = ImGui::CalcTextSize(txt);
        dl->AddText(ImVec2((a.x + b.x - ts.x) * 0.5f, (a.y + b.y - ts.y) * 0.5f),
                    IM_COL32(255, 255, 255, int(235 * bar_alpha)), txt);
    };

    draw_top_pill(menu_min, menu_max, MenuOpen() ? "CLOSE" : "MENU", MenuOpen() || menu_pressed);
    draw_top_pill(hide_min, hide_max, g_hidden ? "SHOW" : "HIDE", g_hidden || hide_pressed);

    if (MenuOpen() && g_edit_mode) {
        input::SetTouch(0, 0, 0, 0, 0, 0, 0);
        if (touch_list.empty()) {
            if (g_drag_idx >= 0) { SaveLayout(); g_drag_idx = -1; }
        } else if (!io.WantCaptureMouse) {
            ImVec2 pt = touch_list[0].second;
            if (g_drag_idx < 0) {
                float best_d2 = 1e30f;
                for (int i = 0; i < kNumControls; i++) {
                    ImVec2 c = center_of(i);
                    float r = radius_of(i) * 1.25f;
                    float dx = pt.x - c.x, dy = pt.y - c.y, d2 = dx * dx + dy * dy;
                    if (d2 <= r * r && d2 < best_d2) {
                        best_d2 = d2; g_drag_idx = i; g_selected_idx = i;
                        g_drag_offset = ImVec2(c.x - pt.x, c.y - pt.y);
                    }
                }
            }
            if (g_drag_idx >= 0) {
                if (!g_custom_pos) {
                    for (int k = 9; k <= 12; k++) {
                        ImVec2 ck = center_of(k);
                        g_ctrl[k].fx = (ck.x - safe_pad_x) / usable_w;
                        g_ctrl[k].fy = ck.y / disp.y;
                    }
                    g_custom_pos = true;
                }
                g_ctrl[g_drag_idx].fx = std::clamp(((pt.x + g_drag_offset.x) - safe_pad_x) / usable_w, 0.02f, 0.98f);
                g_ctrl[g_drag_idx].fy = std::clamp((pt.y + g_drag_offset.y) / disp.y, 0.05f, 0.95f);
            }
        }
    }

    if ((g_hidden && !g_edit_mode) || (MenuOpen() && !g_edit_mode)) {
        g_fingers.clear();
        if (!MenuOpen()) input::SetTouch(0, 0, 0, 0, 0, 0, 0);
        return;
    }

    uint32_t buttons = 0;
    int16_t lx = 0, ly = 0, rx = 0, ry = 0;
    ImVec2 l_knob(0, 0), r_knob(0, 0);

    if (!g_edit_mode) {
        for (auto it = g_fingers.begin(); it != g_fingers.end();) {
            if (pts.find(it->first) == pts.end()) {
                int idx = it->second.control_idx;
                if (idx >= 0 && kDefaultControls[idx].kind == kButton && (now - it->second.down_ms) < 60) {
                    g_linger_buttons |= kDefaultControls[idx].mask;
                    g_linger_until = now + 60;
                }
                it = g_fingers.erase(it);
            } else ++it;
        }
        for (const auto& [fid, pt] : pts) {
            if (hit_box(pt, menu_min, menu_max) || hit_box(pt, hide_min, hide_max)) continue;
            auto it = g_fingers.find(fid);
            if (it == g_fingers.end()) {
                int best = -1; float best_d2 = 1e30f;
                for (int i = 0; i < kNumControls; i++) {
                    ImVec2 c = center_of(i);
                    float r = radius_of(i) * 1.22f;
                    float dx = pt.x - c.x, dy = pt.y - c.y, d2 = dx * dx + dy * dy;
                    if (d2 <= r * r && d2 < best_d2) { best = i; best_d2 = d2; }
                }
                it = g_fingers.emplace(fid, ActiveFinger{best, now}).first;
            }
            int idx = it->second.control_idx;
            if (idx < 0) continue;
            const ControlDef& ctrl = kDefaultControls[idx];
            ImVec2 c = center_of(idx);
            float r = radius_of(idx), dx = pt.x - c.x, dy = pt.y - c.y, dist = std::sqrt(dx * dx + dy * dy);
            if (ctrl.kind == kButton) buttons |= ctrl.mask;
            else if (ctrl.kind == kStickLeft || ctrl.kind == kStickRight) {
                float nx = r > 0 ? dx / r : 0, ny = r > 0 ? dy / r : 0, mag = std::sqrt(nx * nx + ny * ny);
                if (mag > 1.0f) { nx /= mag; ny /= mag; }
                if (ctrl.kind == kStickLeft) { lx = AxisFromNorm(nx); ly = AxisFromNorm(-ny); l_knob = ImVec2(nx * r, ny * r); }
                else { rx = AxisFromNorm(nx); ry = AxisFromNorm(-ny); r_knob = ImVec2(nx * r, ny * r); }
            } else if (ctrl.kind == kDpad && dist > r * 0.22f) {
                if (std::abs(dx) > std::abs(dy)) buttons |= (dx < 0 ? kLeft : kRight);
                else buttons |= (dy < 0 ? kUp : kDown);
            }
        }
        if (now < g_linger_until) buttons |= g_linger_buttons;
        else g_linger_buttons = 0;
        input::SetTouch(buttons & 0xFFFF, (buttons & kLt) ? 255 : 0, (buttons & kRt) ? 255 : 0, lx, ly, rx, ry);
    }

    float alpha = g_edit_mode ? 0.92f : (g_touch_opacity / 100.0f);
    float stroke = std::max(1.8f, ref_h * 0.0042f);
    for (int i = 0; i < kNumControls; i++) {
        const ControlDef& ctrl = kDefaultControls[i];
        ImVec2 c = center_of(i);
        float r = radius_of(i);
        bool sel = (g_edit_mode && g_selected_idx == i);
        bool act = (!g_edit_mode && ((ctrl.kind == kButton && (buttons & ctrl.mask)) ||
                   (ctrl.kind == kStickLeft && (lx || ly)) || (ctrl.kind == kStickRight && (rx || ry)) ||
                   (ctrl.kind == kDpad && (buttons & (kUp | kDown | kLeft | kRight))))) || (g_edit_mode && g_drag_idx == i);
        ImVec4 col = sel ? ImVec4(0.30f, 0.95f, 0.45f, 1.0f) : (g_edit_mode ? ImVec4(1.0f, 0.65f, 0.15f, 1.0f) : ImGui::ColorConvertU32ToFloat4(ctrl.color));
        ImU32 lc = ImGui::ColorConvertFloat4ToU32(ImVec4(col.x, col.y, col.z, ((act || sel) ? 0.95f : 0.48f) * alpha));
        ImU32 bg = IM_COL32(10, 14, 20, int(165 * alpha));
        dl->AddCircleFilled(c, r, bg, 32);
        dl->AddCircleFilled(c, r, ImGui::ColorConvertFloat4ToU32(ImVec4(col.x, col.y, col.z, ((act || sel) ? 0.36f : 0.14f) * alpha)), 32);
        dl->AddCircle(c, r, lc, 32, sel ? stroke * 1.6f : stroke);
        if (ctrl.kind == kStickLeft || ctrl.kind == kStickRight) {
            ImVec2 kb = (ctrl.kind == kStickLeft) ? l_knob : r_knob;
            dl->AddCircleFilled(ImVec2(c.x + kb.x, c.y + kb.y), r * 0.42f, lc, 24);
        } else if (ctrl.kind == kDpad) {
            float arm = r * 0.55f;
            dl->AddLine(ImVec2(c.x - arm, c.y), ImVec2(c.x + arm, c.y), lc, stroke * 1.5f);
            dl->AddLine(ImVec2(c.x, c.y - arm), ImVec2(c.x, c.y + arm), lc, stroke * 1.5f);
        } else if (ctrl.label[0]) {
            ImVec2 ts = ImGui::CalcTextSize(ctrl.label);
            dl->AddText(ImVec2(c.x - ts.x * 0.5f, c.y - ts.y * 0.5f), lc, ctrl.label);
        }
    }
}

bool TouchEditMode() { return MenuOpen() && g_edit_mode; }

void DrawTouchSettingsTab() {
    EnsureLoaded();
    if (g_edit_mode) {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, 52.0f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
        ImGui::SetNextWindowBgAlpha(0.94f);
        ImGui::Begin("##drag_bar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
        int sel = std::clamp(g_selected_idx, 0, kNumControls - 1);
        ImGui::Text("Selected: %s", kDefaultControls[sel].title);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::SliderFloat("Size", &g_ctrl[sel].scale, 0.50f, 1.80f, "%.2fx")) SaveLayout();
        ImGui::SameLine();
        if (ImGui::Button("Done Editing")) { g_edit_mode = false; SaveLayout(); }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) ResetLayout();
        ImGui::End();
        return;
    }
    ImGui::Spacing();
    ImGui::TextWrapped("Tap Edit Layout to drag buttons or resize individual buttons.");
    ImGui::Spacing();
    if (ImGui::SliderFloat("Global Scale", &g_touch_scale, 0.60f, 1.40f, "%.2fx")) SaveLayout();
    if (ImGui::SliderInt("Button Opacity", &g_touch_opacity, 20, 100, "%d%%")) SaveLayout();
    ImGui::Spacing();
    if (ImGui::Button("Edit Layout (Drag & Resize Buttons)", ImVec2(290.0f, 38.0f))) {
        g_edit_mode = true;
        g_hidden = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Layout", ImVec2(140.0f, 38.0f))) {
        ResetLayout();
    }
}
#else
void DrawTouchControls() {}
bool TouchEditMode() { return false; }
void DrawTouchSettingsTab() {}
#endif
}  // namespace app
