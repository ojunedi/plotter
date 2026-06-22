#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>
#include "raylib.h"

#define plot(f, c) plot_impl(f, c, #f)
#define MAX_INTERSECTIONS 256

const float WIDTH     = 800.0f;
const float HEIGHT    = 800.0f;
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

std::vector<Plot> plots;
std::vector<Vector2> intersectionPoints;

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
    return Button(Rectangle{WIDTH - 70, HEIGHT - 40, 60, 28}, "Reset");
}

void plot_impl(MathFunction f, Color color, const char *name) {
    // find existing slot by name to preserve active state
    bool active = true;
    for (auto &p : plots) {
        if (p.name == name) {
            active = p.active;
            break;
        }
    }

    if (active) {
        float range = 10.0f / zoom;
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

void DrawCoordinatePlane() {
    float range = 10.0f / zoom;

    for (float x = floorf(pan.x - range); x <= pan.x + range; x++) {
        DrawLineV(toScreen(Vector2{x, pan.y + range}), toScreen(Vector2{x, pan.y - range}), LIGHTGRAY);
    }
    for (float y = floorf(pan.y - range); y <= pan.y + range; y++) {
        DrawLineV(toScreen(Vector2{pan.x - range, y}), toScreen(Vector2{pan.x + range, y}), LIGHTGRAY);
    }

    Vector2 origin = toScreen(Vector2{0, 0});
    DrawLineEx(Vector2{origin.x, 0},     Vector2{origin.x, HEIGHT}, 5, BLACK);
    DrawLineEx(Vector2{0, origin.y},     Vector2{WIDTH, origin.y},  5, BLACK);
}

void DrawIntersections() {
    float range = 10.0f / zoom;
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
            snprintf(label, sizeof(label), "(%.2f, %.2f)", pt_world.x, pt_world.y);
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
        if (Button(rect, "")) plots[i].active = !plots[i].active;
        Color swatch = plots[i].active ? plots[i].color : Fade(plots[i].color, 0.5f);
        DrawRectangle(legend_x + inner_padding, start_y, swatch_size, swatch_size, swatch);
        DrawText(plots[i].name.c_str(), legend_x + inner_padding + swatch_size + 4, start_y,
                 font_size, plots[i].active ? BLACK : GRAY);
        start_y += entry_height;
    }
}

int main() {
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    InitWindow(WIDTH, HEIGHT, "Function Plotter");
    SetTargetFPS(60);

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
            plot(cosf,      RED);
            plot(coshf,     GREEN);
            plot(quadratic, PURPLE);
            plot(expf,      BLACK);
            plot(atanf,     MAROON);
            plot(linear1,   BLUE);
            plot(linear2,   RED);
            DrawIntersections();
            DrawScaleLegend();
            DrawPlotLegend();
            if (ResetButton()) {
                zoom = 1.0f;
                pan  = Vector2{0, 0};
            }
            DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();
}
