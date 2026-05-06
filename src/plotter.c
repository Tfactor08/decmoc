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

#define MAX_TREES (2 << 4)
#define STEP 0.005

typedef struct {
    NodeTree *trees[MAX_TREES];
    size_t count;
} TreesBuffer; 

TreesBuffer ParseInputTrees(int argc, char *argv[]);

Color graphColors[] = { RED, GREEN, PURPLE };
const size_t graphColorsCount = sizeof(graphColors) / sizeof(graphColors[0]);

// Convert coordinates in the [-1, 1] range to the corresponding screen coordinates 
Vector2 Screen(float x, float y, float scale)
{
    return (Vector2) {
        .x = (x + scale)/(2*scale) * WIDTH,
        .y = (1 - (y + scale)/(2*scale)) * HEIGHT
    };
}

// Draw a grid in the center of the screen
void DrawGridField(int grid_size, Color color)
{
    int offset_x = (WIDTH  / 2) % grid_size; // the amount of pixels need to be right shifted
    int offset_y = (HEIGHT / 2) % grid_size; // the amount of pixels need to be up shifted

    for (int x = offset_x; x <= WIDTH; x += grid_size)
        DrawLine(x, 0, x, HEIGHT, color);
    for (int y = offset_y; y <= HEIGHT; y += grid_size)
        DrawLine(0, y, WIDTH, y, color);
}

void DrawArrow(Vector2 start, Vector2 end, float size, Color color)
{
    Vector2 dir = Vec2Normalize(Vec2Subtract(end, start));
    Vector2 perp = (Vector2) { -dir.y, dir.x };
    
    Vector2 p1 = end;
    Vector2 p2 = Vec2Subtract(end, Vec2Scale(Vec2Add(dir, perp), size));
    Vector2 p3 = Vec2Subtract(end, Vec2Scale(Vec2Subtract(dir, perp), size));
    
    DrawLineEx(start, end, 2, color);
    DrawTriangle(p1, p2, p3, color);
}

RenderTexture2D CreateAxesTexture()
{
    RenderTexture2D texture = LoadRenderTexture(WIDTH, HEIGHT);
    BeginTextureMode(texture);
        DrawArrow(Screen(-1, 0, 1), Screen(1, 0, 1), 8, FOREGROUND);
        DrawArrow(Screen(0, 1, 1), Screen(0, -1, 1), 8, FOREGROUND);
    EndTextureMode();
    return texture;
}

void DrawAxes(RenderTexture2D axesTexture)
{
    DrawTextureRec(axesTexture.texture,
                   (Rectangle) { 0, 0, WIDTH, HEIGHT },
                   (Vector2) { 0, 0 }, FOREGROUND);
}

void DrawGraphs(TreesBuffer *treesBuf, float scale)
{
    for (size_t i = 0; i < treesBuf->count; i++) {
        for (float x = -scale; x <= scale; x += STEP) {
            float y = tree_eval(treesBuf->trees[i], x);
            Color color = graphColors[i % graphColorsCount];
            DrawPixelV(Screen(x, y, scale), color);
        }
    }
}

float GetCurrentScale()
{
    static float scale = 1;
    static float wheel = 0;
    if ((wheel = GetMouseWheelMove())) {
        scale += -wheel/2;
        ClearBackground(BACKGROUND);
    }
    return scale;
}

int main(int argc, char *argv[])
{
    TreesBuffer treesBuf = ParseInputTrees(argc, argv);

    InitWindow(WIDTH, HEIGHT, "Decmoc");
    SetTargetFPS(20);

    RenderTexture2D axesTexture = CreateAxesTexture();

    float scale = GetCurrentScale();

    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(BACKGROUND);
            // TODO: move to texture (for now)
            DrawGridField(GRID_SIZE / 4, GRID_COLOR2);
            DrawGridField(GRID_SIZE, GRID_COLOR1);
            DrawAxes(axesTexture);

            scale = GetCurrentScale();
            DrawGraphs(&treesBuf, scale);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}

TreesBuffer ParseInputTrees(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s expressions\n", argv[0]);
        exit(1);
    }
    TreesBuffer res = {0};
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
