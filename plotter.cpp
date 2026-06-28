#include <algorithm>
#include <cmath>
#include <cstdio>
#include <exception>
#include <functional>
#include <deque>
#include <unordered_map>
#include <string>
#include <vector>
#include "raylib.h"
#include "parser.hpp"
#include "eval.hpp"

#define plot(f, c) plot_impl(f, c, #f)
#define MAX_INTERSECTIONS 256

const float SIDEBAR_W = 260.0f;            // left panel; plot area is to its right
const float WIDTH     = 1260.0f;           // SIDEBAR_W + 1000px plot area
const float HEIGHT    = 1000.0f;
const float PLOT_CX   = SIDEBAR_W + (WIDTH - SIDEBAR_W) / 2.0f;  // plot-area center x
const float CELL_SIZE = 40.0f;
const float A = 1;
const float B = -1;
const float C = -1;
const Color USER_COLORS[] = { ORANGE, PINK, SKYBLUE, LIME, GOLD, VIOLET };
const int   NUM_USER_COLORS = 6;

using MathFunction = std::function<float(float)>;

struct Plot {
    MathFunction func;
    Color        color;
    std::string  name;
    bool         active = true;
};

struct ExprRow {
    std::string source;        // the live text you're editing
    Expr        expr;          // cached parse of source
    bool        ok = false;    // did source parse this edit?
    std::string error;         // last parse error (for display)
    Color       color;
    bool        active = true;
    bool        focused = false;
};

float zoom = 1.0f;
Vector2 pan = {0, 0};

// Vertical anchor for the bottom UI bar. Set in main() to the visible screen
// height so the bar stays on-screen even when the window is taller than the display.
float uiBottom = HEIGHT;

std::deque<ExprRow> rows;
std::vector<Plot> plots;
std::vector<Vector2> intersectionPoints;
std::unordered_map<std::string, bool> activeStates;

void      reparse(ExprRow &r);
Vector2   toScreen(Vector2 world);
Vector2   toWorld(Vector2 screen);
float     worldRange();
float     quadratic(float x);
float     linear1(float x);
float     linear2(float x);
bool      Button(Rectangle rect, const char *label, MouseButton m = MOUSE_BUTTON_LEFT);
bool      ResetButton();
bool      ClearButton();
void      plot_impl(MathFunction f, Color color, const char *name);
float     niceStep(float minPx);
void      DrawCoordinatePlane();
void      DrawIntersections();
void      DrawRoots();
void      DrawMouseHover();
void      DrawSidebar();



void DrawSidebar() {
    const int W = (int)SIDEBAR_W, rowH = 40, pad = 8, sw = 20, xw = 24;
    DrawRectangle(0, 0, W, HEIGHT, Fade(LIGHTGRAY, 0.3f));

    int  deleteIdx = -1;   // defer mutations until after the loop (don't invalidate indices)
    bool addRow    = false;

    for (int i = 0; i < (int)rows.size(); ++i) {
        ExprRow &r = rows[i];
        float y = pad + i * rowH;

        // color swatch — click toggles visibility (#8 color, #9 toggle)
        Rectangle swatch{(float)pad, y + 6, (float)sw, (float)sw};
        if (CheckCollisionPointRec(GetMousePosition(), swatch) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            r.active = !r.active;
        DrawRectangleRec(swatch, r.active ? r.color : Fade(r.color, 0.35f));
        DrawRectangleLinesEx(swatch, 1, BLACK);

        // delete (×) (#10)
        Rectangle del{(float)(W - pad - xw), y + 4, (float)xw, (float)(rowH - 12)};
        if (Button(del, "x")) deleteIdx = i;

        // editable text box (#6) — click to focus, type to edit live
        float bx = pad + sw + 6;
        Rectangle box{bx, y + 4, (float)(W - pad - xw - 6) - bx, (float)(rowH - 12)};
        if (CheckCollisionPointRec(GetMousePosition(), box) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            for (int k = 0; k < (int)rows.size(); ++k) rows[k].focused = (k == i);
        DrawRectangleRec(box, RAYWHITE);
        DrawRectangleLinesEx(box, r.focused ? 2 : 1, r.focused ? BLUE : GRAY);
        Color txt = (!r.ok && !r.source.empty()) ? RED : (r.active ? BLACK : GRAY);
        DrawText(r.source.c_str(), box.x + 6, box.y + 6, 18, txt);

        if (r.focused) {
            bool changed = false;
            int ch;
            while ((ch = GetCharPressed()) > 0) { r.source += (char)ch; changed = true; }
            if (IsKeyPressed(KEY_BACKSPACE) && !r.source.empty()) { r.source.pop_back(); changed = true; }
            if (changed) reparse(r);
        }
    }

    // + add a new row (#10)
    Rectangle add{(float)pad, (float)(pad + rows.size()*rowH), (float)(W - 2*pad), (float)(rowH - 12)};
    if (Button(add, "+ add")) addRow = true;

    // apply deferred mutations
    if (deleteIdx >= 0) {
        rows.erase(rows.begin() + deleteIdx);
        plots.clear();   // plots hold lambdas capturing the erased row; drop them this frame
    }
    if (addRow) {
        for (auto &rr : rows) rr.focused = false;
        rows.push_back(ExprRow{"", {}, false, "", USER_COLORS[rows.size() % NUM_USER_COLORS]});
        rows.back().focused = true;
    }
}



void reparse(ExprRow &r) {
    if (r.source.empty()) { r.ok = false; return; }
    try {
        Parser p{lex(r.source), 0};
        r.expr = p.Parse();
        r.ok = true;  r.error.clear();
    } catch (std::exception &e) {
        r.error = e.what();  r.ok = false;
    }
}

Vector2 toScreen(Vector2 world) {
    return Vector2{
        PLOT_CX  + (world.x - pan.x) * zoom * CELL_SIZE,
        HEIGHT/2 - (world.y - pan.y) * zoom * CELL_SIZE
    };
}

Vector2 toWorld(Vector2 screen) {
    return Vector2{
        (screen.x - PLOT_CX)  / (zoom * CELL_SIZE) + pan.x,
        (HEIGHT/2  - screen.y) / (zoom * CELL_SIZE) + pan.y,
    };
}

// Half-extent of the visible world, derived from the window so the grid and
// curves fill the whole window at any size (was hardcoded to 10 for an 800px window).
float worldRange() {
    return (std::max(WIDTH - SIDEBAR_W, HEIGHT) / 2.0f) / (zoom * CELL_SIZE);
}

float quadratic(float x) { return A*x*x + B*x + C; }
float linear1(float x)   { return x; }
float linear2(float x)   { return 3*x + 4; }

bool Button(Rectangle rect, const char *label, MouseButton m) {
    bool hovered = CheckCollisionPointRec(GetMousePosition(), rect);
    bool clicked = hovered && IsMouseButtonPressed(m);
    DrawRectangleRec(rect, hovered ? LIGHTGRAY : RAYWHITE);
    DrawRectangleLinesEx(rect, 1, BLACK);
    if (label[0]) {
        const int font_size = 16;
        int tw = MeasureText(label, font_size);
        DrawText(label,
                 rect.x + (rect.width  - tw)        / 2.0f,
                 rect.y + (rect.height - font_size) / 2.0f,
                 font_size, BLACK);
    }
    return clicked;
}

bool ResetButton() {
    return Button(Rectangle{WIDTH - 70, uiBottom - 40, 60, 28}, "Reset");
}

bool ClearButton() {
    return Button(Rectangle{WIDTH - 70, uiBottom - 40 - 28 - 10, 60, 28}, "Clear");
}

void plot_impl(MathFunction f, Color color, const char *name) {
    auto it = activeStates.find(name);
    bool active = (it != activeStates.end()) ? it->second : true;

    if (active) {
        float range = worldRange();
        float step  = 1.0f / (zoom * CELL_SIZE);
        float T  = range;                                     // "large" magnitude, scales with zoom
        float lo = pan.y - 2 * range, hi = pan.y + 2 * range; // clamp band keeps screen coords sane

        float rawPrev = f(pan.x - range);
        Vector2 prev  = toScreen(Vector2{pan.x - range, std::clamp(rawPrev, lo, hi)});
        for (float x = pan.x - range; x <= pan.x + range; x += step) {
            float raw    = f(x);
            Vector2 curr = toScreen(Vector2{x, std::clamp(raw, lo, hi)});
            // Skip only true discontinuities: non-finite, or an asymptote crossing
            // (sign flip with large magnitude). Steep on-branch segments are kept and
            // clipped to the window, so spikes reach the screen edge cleanly.
            bool jump = (rawPrev > 0) != (raw > 0) && (fabsf(rawPrev) > T || fabsf(raw) > T);
            bool ok   = std::isfinite(rawPrev) && std::isfinite(raw) && !jump;
            if (ok) DrawLineEx(prev, curr, 2, color);
            prev = curr; rawPrev = raw;
        }
    }

    plots.push_back(Plot{f, color, name, active});
}

// Smallest "nice" world-step (1, 2, 5 x 10^n) that keeps labels at least minPx apart.
float niceStep(float minPx) {
    float raw  = minPx / (zoom * CELL_SIZE);
    float mag  = powf(10.0f, floorf(log10f(raw)));
    float n    = raw / mag;
    float nice = (n <= 1.0f) ? 1.0f : (n <= 2.0f) ? 2.0f : (n <= 5.0f) ? 5.0f : 10.0f;
    return nice * mag;
}

void DrawCoordinatePlane() {
    float range = worldRange();
    float step  = niceStep(50.0f);

    // Faint gridlines at every labeled step, so each gridline gets a tick + label.
    for (float x = ceilf((pan.x - range) / step) * step; x <= pan.x + range; x += step)
        DrawLineV(toScreen(Vector2{x, pan.y + range}), toScreen(Vector2{x, pan.y - range}), LIGHTGRAY);
    for (float y = ceilf((pan.y - range) / step) * step; y <= pan.y + range; y += step)
        DrawLineV(toScreen(Vector2{pan.x - range, y}), toScreen(Vector2{pan.x + range, y}), LIGHTGRAY);

    // Bold axes.
    Vector2 origin = toScreen(Vector2{0, 0});
    DrawLineEx(Vector2{origin.x, 0}, Vector2{origin.x, HEIGHT}, 5, BLACK);
    DrawLineEx(Vector2{SIDEBAR_W, origin.y}, Vector2{WIDTH, origin.y},  5, BLACK);

    // Ticks + number labels: fixed pixel size in screen space, anchored to the
    // axes and clamped so they stay visible when an axis is panned off-screen.
    const float TICK = 5.0f;   // half-length of a tick, in pixels
    const int   FS   = 14;     // label font size
    float axisY = std::min(std::max(origin.y, 0.0f), HEIGHT);        // x-axis row, clamped
    float axisX = std::min(std::max(origin.x, SIDEBAR_W), WIDTH);    // y-axis column, clamped to plot area

    // x-axis ticks/labels
    for (float x = ceilf((pan.x - range) / step) * step; x <= pan.x + range; x += step) {
        if (fabsf(x) < step * 0.5f) continue;                 // skip origin
        float sx = toScreen(Vector2{x, 0}).x;
        DrawLineEx(Vector2{sx, axisY - TICK}, Vector2{sx, axisY + TICK}, 2, BLACK);
        char buf[24]; snprintf(buf, sizeof buf, "%g", x);
        int tw = MeasureText(buf, FS);
        float ly = axisY + TICK + 2;
        if (ly > HEIGHT - FS) ly = axisY - TICK - 2 - FS;     // flip above if near bottom
        DrawText(buf, (int)(sx - tw / 2.0f), (int)ly, FS, DARKGRAY);
    }

    // y-axis ticks/labels
    for (float y = ceilf((pan.y - range) / step) * step; y <= pan.y + range; y += step) {
        if (fabsf(y) < step * 0.5f) continue;
        float sy = toScreen(Vector2{0, y}).y;
        DrawLineEx(Vector2{axisX - TICK, sy}, Vector2{axisX + TICK, sy}, 2, BLACK);
        char buf[24]; snprintf(buf, sizeof buf, "%g", y);
        int tw = MeasureText(buf, FS);
        float lx = axisX + TICK + 4;
        if (lx > WIDTH - tw) lx = axisX - TICK - 4 - tw;      // flip left if near right edge
        DrawText(buf, (int)lx, (int)(sy - FS / 2.0f), FS, DARKGRAY);
    }

    // origin label
    DrawText("0", (int)(axisX + 4), (int)(axisY + 4), FS, DARKGRAY);
}

void DrawIntersections() {
    float range = worldRange();
    float step  = 1.0f / (zoom * CELL_SIZE);
    intersectionPoints.clear();

    for (int i = 0; i < (int)plots.size(); i++) {
        for (int j = i + 1; j < (int)plots.size(); j++) {
            if (!plots[i].active || !plots[j].active) continue;

            auto &f = plots[i].func;
            auto &g = plots[j].func;

            float prevDiff = f(pan.x - range) - g(pan.x - range);
            for (float x = pan.x - range + step; x <= pan.x + range; x += step) {
                float diff = f(x) - g(x);
                if (std::isfinite(prevDiff) && std::isfinite(diff)
                    && prevDiff * diff <= 0 && (prevDiff != 0 || diff != 0)) {
                    float t  = fabsf(prevDiff) / (fabsf(prevDiff) + fabsf(diff));
                    float xi = (x - step) + t * step;
                    float yi = f(xi);
                    intersectionPoints.push_back(Vector2{xi, yi});
                    Vector2 pt = toScreen(Vector2{xi, yi});
                    DrawCircleV(pt, 4, WHITE);
                    DrawCircleLinesV(pt, 4, BLACK);
                }
                prevDiff = diff;
            }
        }
    }

    Vector2 mouse = GetMousePosition();
    for (auto &pt_world : intersectionPoints) {
        Vector2 pt = toScreen(pt_world);
        float dx = mouse.x - pt.x, dy = mouse.y - pt.y;
        if (dx*dx + dy*dy < 12*12) {
            char label[64];
            snprintf(label, sizeof(label), "(%.3f, %.3f)", pt_world.x, pt_world.y);
            const int font_size = 16, pad = 4;
            int tw = MeasureText(label, font_size);
            int tx = pt.x + 10, ty = pt.y - 24;
            DrawRectangle(tx - pad, ty - pad, tw + pad*2, font_size + pad*2, RAYWHITE);
            DrawRectangleLines(tx - pad, ty - pad, tw + pad*2, font_size + pad*2, LIGHTGRAY);
            DrawText(label, tx, ty, font_size, BLACK);
        }
    }
}

// Mark x-intercepts (roots): where each curve crosses y = 0. Same sign-change +
// interpolation as DrawIntersections, against the constant 0. Marker is filled with
// the curve's color so roots read differently from the white intersection dots.
void DrawRoots() {
    float range = worldRange();
    float step  = 1.0f / (zoom * CELL_SIZE);
    float T     = range;   // magnitude gate: reject asymptote sign-flips (e.g. 1/x at 0)

    for (auto &p : plots) {
        if (!p.active) continue;
        auto &f = p.func;
        float prev = f(pan.x - range);
        for (float x = pan.x - range + step; x <= pan.x + range; x += step) {
            float curr = f(x);
            if (std::isfinite(prev) && std::isfinite(curr)
                && prev * curr <= 0 && (prev != 0 || curr != 0)
                && fabsf(prev) < T && fabsf(curr) < T) {
                float t  = fabsf(prev) / (fabsf(prev) + fabsf(curr));
                float xi = (x - step) + t * step;
                Vector2 pt = toScreen(Vector2{xi, 0});
                DrawCircleV(pt, 4, p.color);
                DrawCircleLinesV(pt, 4, BLACK);

                Vector2 mouse = GetMousePosition();
                float dx = mouse.x - pt.x, dy = mouse.y - pt.y;
                if (dx*dx + dy*dy < 12*12) {
                    char label[64];
                    snprintf(label, sizeof(label), "(%.3f, 0)", xi);
                    const int font_size = 16, pad = 4;
                    int tw = MeasureText(label, font_size);
                    int tx = pt.x + 10, ty = pt.y - 24;
                    DrawRectangle(tx - pad, ty - pad, tw + pad*2, font_size + pad*2, RAYWHITE);
                    DrawRectangleLines(tx - pad, ty - pad, tw + pad*2, font_size + pad*2, LIGHTGRAY);
                    DrawText(label, tx, ty, font_size, BLACK);
                }
            }
            prev = curr;
        }
    }
}

void DrawMouseHover() {
    Vector2 mouse = GetMousePosition();
    if (mouse.x < SIDEBAR_W) return;   // ignore hover over the sidebar
    float wx = toWorld(mouse).x;
    for (Plot &p: plots) {
        if (!p.active) continue;
        float wy = p.func(wx);
        if (!std::isfinite(wy)) continue;
        Vector2 pt = toScreen({wx, wy});
        if (fabsf(pt.y - mouse.y) < 10) {
            char label[64];
            snprintf(label, sizeof(label), "(%.3f, %.3f)", wx, wy);
            const int font_size = 16, pad = 4;
            int tw = MeasureText(label, font_size);
            int tx = pt.x + 10, ty = pt.y - 24;
            DrawRectangle(tx - pad, ty - pad, tw + pad*2, font_size + pad*2, RAYWHITE);
            DrawRectangleLines(tx - pad, ty - pad, tw + pad*2, font_size + pad*2, LIGHTGRAY);
            DrawText(label, tx, ty, font_size, BLACK);
            DrawCircleV(pt, 4, WHITE);
            DrawCircleLinesV(pt, 4, BLACK);
        }
    }
}

int main() {
    SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);
    InitWindow(WIDTH, HEIGHT, "Function Plotter");
    SetTargetFPS(60);

    // Keep the bottom bar within the visible screen if the window is taller than the display.
    float visibleH = (float)GetMonitorHeight(GetCurrentMonitor()) - 60.0f;
    uiBottom = std::min(HEIGHT, visibleH);

    // seed one row so there's something to edit on launch
    rows.push_back(ExprRow{"sin(x)", {}, false, "", USER_COLORS[0]});
    reparse(rows.back());

    while (!WindowShouldClose()) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 delta = GetMouseDelta();
            pan.x -= delta.x / (zoom * CELL_SIZE);
            pan.y += delta.y / (zoom * CELL_SIZE);
        }
        zoom += GetMouseWheelMove() * 0.1f;
        if (zoom < 0.1f) zoom = 0.1f;

        plots.clear();

        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawCoordinatePlane();
            // plot(sinf,      BLUE);
            // plot(cosf,      RED);
            // plot(coshf,     GREEN);
            // plot(quadratic, PURPLE);
            // plot(expf,      BLACK);
            // plot(atanf,     MAROON);
            // plot(linear1,   BLUE);
            // plot(linear2,   RED);
            // rebuild plots from rows (source of truth)
            for (auto &r : rows) {
                if (r.active && r.ok)
                    plot_impl([&r](float x) { return (float)eval(r.expr, x); }, r.color, r.source.c_str());
            }

            DrawIntersections();
            DrawRoots();
            DrawMouseHover();
            DrawSidebar();
            if (ResetButton()) {
                zoom = 1.0f;
                pan  = Vector2{0, 0};
            }
            if (ClearButton()) {
               rows.clear();
               activeStates.clear();
            }
        EndDrawing();
    }

    CloseWindow();
}
