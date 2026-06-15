#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "parser.h"
#include "basic_utils.c"
#include "vector_utils.c"

#define WIDTH          800.0f
#define HEIGHT         800.0f
#define GRID_SIZE      80.0f
#define LINE_THICKNESS 3.0f
#define TEXT_SIZE      15.0f
#define BACKGROUND     (Color) { 0xff, 0xff, 0xff, 0xff }
#define FOREGROUND     (Color) { 0x2e, 0x2e, 0x2e, 0xff }
#define GRID_COLOR1    (Color) { 0xb8, 0xb8, 0xb8, 0xff }
#define GRID_COLOR2    (Color) { 0xe7, 0xe7, 0xe7, 0xff }

#define MAX_EXPRS (2 << 4)
#define STEP      0.2f

typedef struct {
    const char *string;
    NodeTree *tree;
} Expr;

Expr exprs[MAX_EXPRS];
size_t exprsCount;

float scale = 1.0f;
float offsetX = 0.0f, offsetY = 0.0f;
float startPanX = 0.0f, startPanY = 0.0f;

const Color graphColors[] = { RED, GREEN, PURPLE };
const size_t graphColorsCount = sizeof(graphColors) / sizeof(*graphColors);

/* Convert coordinates in the [-scale, scale] range to the corresponding screen coordinates */
Vector2 Screen(float x, float y)
{
    return (Vector2) {
        .x = (x + scale)/(2*scale) * WIDTH,
        .y = (1 - (y + scale)/(2*scale)) * HEIGHT
    };
}

/* Convert coordinates in the [-scale, scale] range to the corresponding screen coordinates
   (vector version) */
Vector2 ScreenV(Vector2 *p)
{
    return (Vector2) {
        .x = (p->x + scale)/(2*scale) * WIDTH,
        .y = (1 - (p->y + scale)/(2*scale)) * HEIGHT
    };
}

/* Render a grid in the center of the screen (if the global offsets are zero) */
void RenderGridField(int gridSize, Color color)
{
    float gridSizeNorm = ((float) gridSize / WIDTH) * scale;

    // The amount of pixels need to be right shifted for centering
    float gridOffsetX = fmod(offsetX, gridSizeNorm);
    // The amount of pixels need to be up shifted for centering
    float gridOffsetY = fmod(offsetY, gridSizeNorm);

    // Vertical lines
    for (float x = -scale - gridOffsetX; x <= scale - gridOffsetX; x += gridSizeNorm)
        DrawLineV(Screen(x, -scale), Screen(x, scale), color);
    // Horizontal lines
    for (float y = -scale - gridOffsetY; y <= scale - gridOffsetY; y += gridSizeNorm)
        DrawLineV(Screen(-scale, y), Screen(scale, y), color);
}

void RenderAxes()
{
    // Vertical axis
    DrawLineEx(Screen(0 - offsetX, -scale),
               Screen(0 - offsetX, scale),
               LINE_THICKNESS, FOREGROUND);
    // Horizontal axis
    DrawLineEx(Screen(-scale, 0 - offsetY),
               Screen(scale, 0 - offsetY),
               LINE_THICKNESS, FOREGROUND);
}

void RenderAxesNumbers()
{
    char num[4];
    float margin = 0.02f;
    // X-axis
    for (int nX = -scale + offsetX; nX <= scale + offsetX; nX++) {
        Vector2 textPos = Screen(nX - offsetX, 0.0f - offsetY - margin);
        itoa(nX, num, sizeof(num));
        DrawText(num, textPos.x, textPos.y, TEXT_SIZE, BLACK);
    }
    // Y-axis
    for (int nY = -scale + offsetY; nY <= scale + offsetY; nY++) {
        // Avoid rendering 0 twice
        if (nY == 0) continue;
        Vector2 textPos = Screen(0.0f - offsetX + margin, nY - offsetY);
        itoa(nY, num, sizeof(num));
        DrawText(num, textPos.x, textPos.y, TEXT_SIZE, BLACK);
    }
}

void RenderExprLabels()
{
    int margin = 10;
    int interval = 25;
    for (size_t i = 0; i < exprsCount; i++) {
        int x = 0 + margin;
        int y = i*interval + margin;
        Color color = graphColors[i % graphColorsCount];
        DrawText(exprs[i].string, x, y, TEXT_SIZE, color);
    }
}

void RenderGraphs()
{
    float a = -scale + offsetX;
    float b = scale + offsetX;
    for (size_t i = 0; i < exprsCount; i++) {
        // We need to convert the shifted x coordinates back to -scale..scale range
        // so the Screen* functions can draw them correctly.
        //
        // Steps required:
        // 1) normalize x to be in 0..1 range:
        // n = (x - a) / (b - a) [how far x from the start / the range length]
        // 2) now map to the -c..c range:
        // r = -c + n*2c [first, 0..2c, then -c..c]
        NodeTree *tree = exprs[i].tree;
        Color color = graphColors[i % graphColorsCount];
        float y = tree_eval(tree, a) - offsetY;
        Vector2 firstPoint = (Vector2) { -scale, y }, secondPoint;
        for (float x = a; x <= b; x += STEP) {
            y = tree_eval(tree, x) - offsetY;
            float xNorm = (x - a) / (b - a);
            float xMapped = -scale + xNorm * 2.0f*scale;
            secondPoint = (Vector2) { xMapped, y };
            DrawLineEx(ScreenV(&firstPoint),
                       ScreenV(&secondPoint),
                       LINE_THICKNESS, color);
            firstPoint = secondPoint;
        }
        // We need to make sure the last line with the end at b is drawn regardless
        // of the STEP value
        y = tree_eval(tree, b) - offsetY;
        float xMapped = -scale + 2*scale;
        firstPoint = secondPoint;
        secondPoint = (Vector2) { xMapped, y };
        DrawLineEx(ScreenV(&firstPoint),
                   ScreenV(&secondPoint),
                   LINE_THICKNESS, color);
    }
}

void SetCurrentScale()
{
    float wheel = 0.0f;
    if ((wheel = GetMouseWheelMove())) {
        scale += -wheel / 2;
        ClearBackground(BACKGROUND);
    }
}

void SetCurrentOfssets()
{
    if (IsKeyPressed(KEY_RIGHT))     offsetX += 0.1f;
    else if (IsKeyPressed(KEY_LEFT)) offsetX -= 0.1f;
    else if (IsKeyPressed(KEY_UP))   offsetY += 0.1f;
    else if (IsKeyPressed(KEY_DOWN)) offsetY -= 0.1f;
    // TODO: below is a first attempt of panning implementation;
    //       doesn't work well on scaling, and has to be reimplemented
    //       to match new screen-world transformation ('refactor/transform-strategy' branch) anyway.
    // float mouseX = (float) GetMouseX();
    // float mouseY = (float) GetMouseY();
    //
    // if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    //     startPanX = mouseX
    //     startPanY = mouseY;
    // }
    // if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    //     offsetX -= (mouseX - startPanX) / 100 * scale;
    //     offsetY += (mouseY - startPanY) / 100 * scale;

    //     startPanX = mouseX;
    //     startPanY = mouseY;
    // }
}

void ParseInputExprs(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s EXPRESSION...\n", argv[0]);
        exit(1);
    }
    for (int i = 1; i < argc; i++) {
        NodeTree *tree = tree_parse(argv[i]);
        if (tree == NULL)
            exit(EXIT_FAILURE);
        Expr expr = {
            .string = argv[i],
            .tree = tree
        };
        exprs[i-1] = expr;
    }
    exprsCount = argc - 1;
}

int main(int argc, char *argv[])
{
    ParseInputExprs(argc, argv);

    InitWindow(WIDTH, HEIGHT, "Decmoc");
    SetTargetFPS(20);

    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(BACKGROUND);

            SetCurrentScale();
            SetCurrentOfssets();

            //RenderGridField(GRID_SIZE / 4, GRID_COLOR2);
            //RenderGridField(GRID_SIZE, GRID_COLOR1);

            RenderExprLabels();
            RenderAxes();
            RenderAxesNumbers();
            RenderGraphs();
        EndDrawing();
    }

    CloseWindow();
}
