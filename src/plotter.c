#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "parser.h"
#include "vector_utils.c"

#define WIDTH 800
#define HEIGHT 600
#define GRID_SIZE 80
#define BACKGROUND WHITE
#define FOREGROUND (Color) { 0x2e, 0x2e, 0x2e, 0xff }
#define GRID_COLOR1 (Color) { 0xb8, 0xb8, 0xb8, 0xff }
#define GRID_COLOR2 (Color) { 0xe7, 0xe7, 0xe7, 0xff }
#define LINE_THICKNESS 3
#define ARROW_SIZE 8

#define MAX_TREES (2 << 4)
#define STEP 0.2

typedef struct {
    NodeTree *trees[MAX_TREES];
    size_t count;
} ExprsBuffer; 

float scale = 1, offsetX = 0, offsetY = 0;

Color graphColors[] = { RED, GREEN, PURPLE };
const size_t graphColorsCount = sizeof(graphColors) / sizeof(graphColors[0]);

/* Convert coordinates in the [-scale, scale] range to the corresponding screen coordinates */
Vector2 Screen(float x, float y)
{
    return (Vector2) {
        .x = (x + scale)/(2*scale) * WIDTH,
        .y = (1 - (y + scale)/(2*scale)) * HEIGHT
    };
}

/* Convert coordinates in the [-scale, scale] range to the corresponding screen coordinates (vector version) */
Vector2 ScreenV(Vector2 *p)
{
    return (Vector2) {
        .x = (p->x + scale)/(2*scale) * WIDTH,
        .y = (1 - (p->y + scale)/(2*scale)) * HEIGHT
    };
}

// TODO: this version of the function doesn't respect the scale at all.
//       We can't keep this version since in that case the grid movement
//       won't be aligned with the graphs.
//       Actually, calling the Screen function here seems irrational since
//       the coordinates are in the screen range already. But that's the
//       only way manage the scaling atm.
/* Draw a grid in the center of the screen (if the input offsets are zero) */
void DrawGridField(int grid_size, Color color)
{
    // the amount of pixels need to be right shifted
    //int offsetX = ((WIDTH  / 2) + offsetX) % grid_size;
    // the amount of pixels need to be up shifted
    //int offsetY = ((HEIGHT / 2) + offsetY) % grid_size;

    //DrawLine(x, 0, x, HEIGHT, color);
    for (int x = offsetX; x <= WIDTH; x += grid_size)
        DrawLineV(Screen(x, 0), Screen(x, HEIGHT), color);
    //DrawLine(0, y, WIDTH, y, color);
    for (int y = offsetY; y <= HEIGHT; y += grid_size)
        DrawLineV(Screen(0, y), Screen(WIDTH, y), color);
}

void DrawAxes()
{
    // horizontal axis
    DrawLineEx(Screen(-scale, 0 + -offsetY),
               Screen(scale, 0 + -offsetY),
               LINE_THICKNESS, FOREGROUND);
    // vertical axis
    DrawLineEx(Screen(0 + -offsetX, -scale),
               Screen(0 + -offsetX, scale),
               LINE_THICKNESS, FOREGROUND);
}

void DrawGraphs(ExprsBuffer *exprsBuffer)
{
    float a = -scale + offsetX;
    float b = scale + offsetX;
    for (size_t i = 0; i < exprsBuffer->count; i++) {
        // We need to convert the shifted x coordinates back to -scale..scale range
        // so the Screen* functions can draw them correctly.
        //
        // Steps required:
        // 1) normalize x to be in 0..1 range:
        // n = (x - a) / (b - a) [how far x from the start / the range length]
        // 2) now map to the -c..c range:
        // r = -c + n*2c [first, 0..2c, then -c..c]

        float y = tree_eval(exprsBuffer->trees[i], a) - offsetY;
        Vector2 firstPoint = (Vector2) { -scale, y };
        for (float x = a; x <= b; x += STEP) {
            y = tree_eval(exprsBuffer->trees[i], x) - offsetY;
            float x_norm = (x - a) / (b - a);
            float x_mapped = -scale + x_norm * 2*scale;
            Vector2 secondPoint = (Vector2) { x_mapped, y };
            Color color = graphColors[i % graphColorsCount];
            DrawLineEx(ScreenV(&firstPoint),
                       ScreenV(&secondPoint),
                       LINE_THICKNESS, color);
            firstPoint = secondPoint;
        }
    }
}

void SetCurrentScale()
{
    float wheel = 0;
    if ((wheel = GetMouseWheelMove())) {
        scale += -wheel / 2;
        ClearBackground(BACKGROUND);
    }
}

void SetCurrentOfssets()
{
    if (IsKeyPressed(KEY_RIGHT))     offsetX += 0.1;
    else if (IsKeyPressed(KEY_LEFT)) offsetX -= 0.1;
    else if (IsKeyPressed(KEY_UP))   offsetY += 0.1;
    else if (IsKeyPressed(KEY_DOWN)) offsetY -= 0.1;
}

ExprsBuffer ParseInputExprs(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    ExprsBuffer exprsBuffer = ParseInputExprs(argc, argv);

    InitWindow(WIDTH, HEIGHT, "Decmoc");
    SetTargetFPS(20);

    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(BACKGROUND);

            SetCurrentScale();
            SetCurrentOfssets();

            DrawAxes();
            DrawGraphs(&exprsBuffer);
            //DrawGridField(GRID_SIZE / 4, GRID_COLOR2, scale, offsetX, offsetY);
            //DrawGridField(GRID_SIZE, GRID_COLOR1, scale, offsetX, offsetY);
        EndDrawing();
    }

    CloseWindow();
}

ExprsBuffer ParseInputExprs(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s expressions\n", argv[0]);
        exit(1);
    }
    ExprsBuffer res = {0};
    char expression[256];
    for (int i = 1; i < argc; i++) {
        strncpy(expression, argv[i], sizeof(expression));
        NodeTree *tree = tree_parse(expression);
        if (tree == NULL) {
            fprintf(stderr, "Duck\n");
            exit(EXIT_FAILURE);
        }
        res.trees[i-1] = tree;
    }
    res.count = argc - 1;
    return res;
}
