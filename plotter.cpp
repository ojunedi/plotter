#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <optional>
#include <unordered_map>
#include <string>
#include <vector>
#include "raylib.h"
#include "parser.hpp"
#include "eval.hpp"

#define plot(f, c) plot_impl(f, c, #f)
#define MAX_INTERSECTIONS 256

const float WIDTH     = 1000.0f;
const float HEIGHT    = 1000.0f;
const float CELL_SIZE = 40.0f;
const float A = 1;
const float B = -1;
const float C = -1;

using MathFunction = std::function<float(float)>;

struct Plot {
    MathFunction func;
    Color        color;
    std::string  name;
    bool         active = true;
};

float zoom = 1.0f;
Vector2 pan = {0, 0};

// Vertical anchor for the bottom UI bar. Set in main() to the visible screen
// height so the bar stays on-screen even when the window is taller than the display.
float uiBottom = HEIGHT;

std::vector<Plot> plots;
std::vector<Vector2> intersectionPoints;
std::unordered_map<std::string, bool> activeStates;

Vector2 toScreen(Vector2 world) {
    return Vector2{
        WIDTH/2  + (world.x - pan.x) * zoom * CELL_SIZE,
        HEIGHT/2 - (world.y - pan.y) * zoom * CELL_SIZE
    };
}

Vector2 toWorld(Vector2 screen) {
    return Vector2{
        (screen.x - WIDTH/2)  / (zoom * CELL_SIZE) + pan.x,
        (HEIGHT/2  - screen.y) / (zoom * CELL_SIZE) + pan.y,
    };
}

// Half-extent of the visible world, derived from the window so the grid and
// curves fill the whole window at any size (was hardcoded to 10 for an 800px window).
float worldRange() {
    return (std::max(WIDTH, HEIGHT) / 2.0f) / (zoom * CELL_SIZE);
}

float quadratic(float x) { return A*x*x + B*x + C; }
float linear1(float x)   { return x; }
float linear2(float x)   { return 3*x + 4; }

bool Button(Rectangle rect, const char *label) {
    bool hovered = CheckCollisionPointRec(GetMousePosition(), rect);
    bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    DrawRectangleRec(rect, hovered ? LIGHTGRAY : RAYWHITE);
    DrawRectangleLinesEx(rect, 1, BLACK);
    if (label[0]) DrawText(label, rect.x + 8, rect.y + 8, 16, BLACK);
    return clicked;
}

bool ResetButton() {
    return Button(Rectangle{WIDTH - 70, uiBottom - 40, 60, 28}, "Reset");
}

void plot_impl(MathFunction f, Color color, const char *name) {
    auto it = activeStates.find(name);
    bool active = (it != activeStates.end()) ? it->second : true;

    if (active) {
        float range = worldRange();
        float step  = 1.0f / (zoom * CELL_SIZE);
        Vector2 prev = toScreen(Vector2{pan.x - range, f(pan.x - range)});
        for (float x = pan.x - range; x <= pan.x + range; x += step) {
            Vector2 curr = toScreen(Vector2{x, f(x)});
            DrawLineEx(prev, curr, 2, color);
            prev = curr;
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
    DrawLineEx(Vector2{0, origin.y}, Vector2{WIDTH, origin.y},  5, BLACK);

    // Ticks + number labels: fixed pixel size in screen space, anchored to the
    // axes and clamped so they stay visible when an axis is panned off-screen.
    const float TICK = 5.0f;   // half-length of a tick, in pixels
    const int   FS   = 14;     // label font size
    float axisY = std::min(std::max(origin.y, 0.0f), HEIGHT); // x-axis row, clamped
    float axisX = std::min(std::max(origin.x, 0.0f), WIDTH);  // y-axis column, clamped

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

void DrawScaleLegend() {
    char label[35];
    snprintf(label, sizeof(label), "1 cell = %.2f units", 1.0f / zoom);
    const int font_size = 20, pad = 4;
    int tw = MeasureText(label, font_size);
    DrawRectangle(10 - pad, 30 - pad, tw + pad*2, font_size + pad*2, RAYWHITE);
    DrawRectangleLines(10 - pad, 30 - pad, tw + pad*2, font_size + pad*2, LIGHTGRAY);
    DrawText(label, 10, 30, font_size, BLACK);
}

void DrawPlotLegend() {
    const int outer_padding = 5;
    const int inner_padding = 2;
    const int entry_height  = 20;
    const int swatch_size   = 16;
    const int font_size     = 16;
    const int legend_width  = 120;
    const int max_entries   = 6;
    const int legend_x      = WIDTH - legend_width - outer_padding;

    int entries     = std::min((int)plots.size(), max_entries);
    int legend_height = outer_padding + inner_padding + entries * entry_height;

    DrawRectangle(legend_x, outer_padding, legend_width, legend_height, RAYWHITE);
    DrawRectangleLines(legend_x, outer_padding, legend_width, legend_height, BLACK);

    int start_y = outer_padding + inner_padding;
    for (int i = 0; i < entries; i++) {
        Rectangle rect{(float)(legend_x + inner_padding), (float)start_y, (float)swatch_size, (float)swatch_size};
        if (Button(rect, "")) {
            plots[i].active = !plots[i].active;
            activeStates[plots[i].name] = plots[i].active;
        }
        Color swatch = plots[i].active ? plots[i].color : Fade(plots[i].color, 0.5f);
        DrawRectangle(legend_x + inner_padding, start_y, swatch_size, swatch_size, swatch);
        DrawText(plots[i].name.c_str(), legend_x + inner_padding + swatch_size + 4, start_y,
                 font_size, plots[i].active ? BLACK : GRAY);
        start_y += entry_height;
    }
}

struct UserExpr {
    std::string  source;
    Expr         expr;
    Color        color;
    bool         active = true;
};

const Color USER_COLORS[] = { ORANGE, PINK, SKYBLUE, LIME, GOLD, VIOLET };
const int   NUM_USER_COLORS = 6;

std::vector<UserExpr> userExprs;

struct TextInput {
    std::string buf;
    bool        focused = false;
    std::string error;
};




void DrawTextInput(TextInput &input) {
    const int pad = 6;
    Rectangle rect{10, uiBottom - 40, WIDTH - 90, 28};

    if (CheckCollisionPointRec(GetMousePosition(), rect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        input.focused = true;
    else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        input.focused = false;

    DrawRectangleRec(rect, RAYWHITE);
    DrawRectangleLinesEx(rect, input.focused ? 2 : 1, input.focused ? BLUE : LIGHTGRAY);
    DrawText(input.buf.c_str(), rect.x + pad, rect.y + pad, 16, BLACK);

    if (!input.error.empty())
        DrawText(input.error.c_str(), rect.x, rect.y - 22, 14, RED);

    if (input.focused) {
        int ch;
        while ((ch = GetCharPressed()) > 0)
            input.buf += (char)ch;

        if (IsKeyPressed(KEY_BACKSPACE) && !input.buf.empty())
            input.buf.pop_back();

        if (IsKeyPressed(KEY_ESCAPE))
            input.focused = false;

        if (IsKeyPressed(KEY_ENTER) && !input.buf.empty()) {
            try {
                auto tokens = lex(input.buf);
                Parser p{tokens, 0};
                Expr expr = p.Parse();
                Color color = USER_COLORS[userExprs.size() % NUM_USER_COLORS];
                userExprs.push_back({input.buf, std::move(expr), color});
                input.buf.clear();
                input.error.clear();
                input.focused = false;
            } catch (std::exception &e) {
                input.error = e.what();
            }
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

    TextInput input;

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
            plot(sinf,      BLUE);
            // plot(cosf,      RED);
            // plot(coshf,     GREEN);
            // plot(quadratic, PURPLE);
            // plot(expf,      BLACK);
            // plot(atanf,     MAROON);
            // plot(linear1,   BLUE);
            // plot(linear2,   RED);
            for (auto &ue : userExprs) {
                plot_impl([&ue](float x) { return (float)eval(ue.expr, x); }, ue.color, ue.source.c_str());
            }
            DrawIntersections();
            DrawScaleLegend();
            DrawPlotLegend();
            DrawTextInput(input);
            if (ResetButton()) {
                zoom = 1.0f;
                pan  = Vector2{0, 0};
            }
            DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();
}
