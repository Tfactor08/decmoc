#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "parser.h"
#include "slider.h"
#include "utils/basic_utils.c"
#include "utils/vector_utils.c"

#define WIDTH 960.0f
#define HEIGHT 720.0f
#define GRID_SIZE 80.0f
#define LINE_THICKNESS 3.0f
#define TEXT_SIZE 15.0f
#define BACKGROUND  (Color) { 0xff, 0xff, 0xff, 0xff }
#define FOREGROUND  (Color) { 0x2e, 0x2e, 0x2e, 0xff }
#define GRID_COLOR1 (Color) { 0xb8, 0xb8, 0xb8, 0xff }
#define GRID_COLOR2 (Color) { 0xe7, 0xe7, 0xe7, 0xff }

#define MAX_FUNCS (2 << 4)
#define XSTEP 0.2f

typedef struct {
    const char *string;
    NodeTree *tree;
    // TODO: "Params" struct is quite massive. Maybe allocate it on the heap?
    Params params;
} Func;

Func inputFuncs[MAX_FUNCS];
size_t inputFuncCount;

float scale = 1.0f;
float offsetX = 0.0f, offsetY = 0.0f;
float startPanX = 0.0f, startPanY = 0.0f;

const Color graphColors[] = { RED, GREEN, PURPLE };
const size_t graphColorsCount = sizeof(graphColors) / sizeof(*graphColors);

// NOTE: World-to-Screen transform implementation (Screen* functions) is quite chancy,
//       and unfortunately I couldn't be able to find better solutions. One of the flaws
//       is that in some cases the rendering functions have to "know" about screen offsets
//       (offsetX and offsetY) in order to draw objects which must be rendered edge-to-edge:  
//       for example, "RenderAxes" has to add "offsetY" to the y coordinate of the vertical
//       axis and "offsetX" to the the x coordinate of the horizontal axis
//       (similar situation in "RenderGraphs").

/* Convert coordinates in the [-scale, scale] range to the corresponding screen coordinates */
Vector2 Screen(float x, float y)
{
    return (Vector2) {
        .x = (x + scale - offsetX)/(2*scale) * WIDTH,
        .y = (1 - (y + scale - offsetY)/(2*scale)) * HEIGHT
    };
}

/* Convert coordinates in the [-scale, scale] range to the corresponding screen coordinates
   (vector version) */
Vector2 ScreenV(Vector2 *p)
{
    return (Vector2) {
        .x = (p->x + scale - offsetX)/(2*scale) * WIDTH,
        .y = (1 - (p->y + scale - offsetY)/(2*scale)) * HEIGHT
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
    DrawLineEx(Screen(0, -scale + offsetY),
               Screen(0, scale + offsetY),
               LINE_THICKNESS, FOREGROUND);
    // Horizontal axis
    DrawLineEx(Screen(-scale + offsetX, 0),
               Screen(scale + offsetX, 0),
               LINE_THICKNESS, FOREGROUND);
}

void RenderAxesNumbers()
{
    const float margin = 0.02f;
    char num[1 << 2];
    // X-axis
    for (int nX = -scale + offsetX; nX <= scale + offsetX; nX++) {
        Vector2 textPos = Screen(nX, 0.0f - margin);
        itoa(nX, num, sizeof(num));
        DrawText(num, textPos.x, textPos.y, TEXT_SIZE, BLACK);
    }
    // Y-axis
    for (int nY = -scale + offsetY; nY <= scale + offsetY; nY++) {
        // Avoid rendering 0 twice
        if (nY == 0) continue;
        Vector2 textPos = Screen(0.0f + margin, nY);
        itoa(nY, num, sizeof(num));
        DrawText(num, textPos.x, textPos.y, TEXT_SIZE, BLACK);
    }
}

void RenderFuncLabels()
{
    int margin = 10;
    int interval = 25;
    for (size_t i = 0; i < inputFuncCount; i++) {
        int x = 0 + margin;
        int y = i*interval + margin;
        Color color = graphColors[i % graphColorsCount];
        DrawText(inputFuncs[i].string, x, y, TEXT_SIZE, color);
    }
}

void RenderGraphs()
{
    float a = -scale;
    float b = scale;
    for (size_t funci = 0; funci < inputFuncCount; funci++) {
        Func func = inputFuncs[funci];
        NodeTree *tree = func.tree;
        Color color = graphColors[funci % graphColorsCount];
        float y = tree_eval(tree, a + offsetX, &func.params);
        Vector2 firstPoint = (Vector2) { a + offsetX, y }, secondPoint;
        for (float x = a; x <= b; x += XSTEP) {
            y = tree_eval(tree, x + offsetX, &func.params);
            secondPoint = (Vector2) { x + offsetX, y };
            DrawLineEx(ScreenV(&firstPoint),
                       ScreenV(&secondPoint),
                       LINE_THICKNESS, color);
            firstPoint = secondPoint;
        }
        // Make sure last line is rendered as well regardless of the "b" and "XSTEP" values.
        y = tree_eval(tree, b + offsetX, &func.params);
        secondPoint = (Vector2) { b + offsetX, y };
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
    //if (IsKeyPressed(KEY_LEFT))       offsetX -= 0.1f;
    //else if (IsKeyPressed(KEY_RIGHT)) offsetX += 0.1f;
    //else if (IsKeyPressed(KEY_DOWN))  offsetY -= 0.1f;
    //else if (IsKeyPressed(KEY_UP))    offsetY += 0.1f;

    // TODO: below is a first attempt of panning implementation;
    //       Doesn't work well on scaling (or does it?).
    float mouseX = (float) GetMouseX();
    float mouseY = (float) GetMouseY();
    
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        startPanX = mouseX;
        startPanY = mouseY;
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        offsetX -= (mouseX - startPanX) / 300 * scale;
        offsetY += (mouseY - startPanY) / 300 * scale;

        startPanX = mouseX;
        startPanY = mouseY;
    }
}

/* Acquire parameters for a function (if any) from user (must be dynamic in the future). */
void AcquireFuncParameters(Func *func)
{
    for (size_t parami = 0; parami < func->params.count; parami++) {
        Params *params = &func->params;
        // TODO: naming sucks.
        char param_name = params->params[parami];
        float param_value = 0;
        printf("[%s] %c: ", func->string, param_name);
        scanf("%f", &param_value);
        // TODO: is this cast safe?
        params->param_to_value[(int) param_name] = param_value;
    }

}

void ParseInputFuncs(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s FUNCTION...\n", argv[0]);
        exit(1);
    }
    for (int argi = 1; argi < argc; argi++) {
        Func func = {0};

        NodeTree *tree = tree_parse(argv[argi], &func.params);
        if (tree == NULL) exit(EXIT_FAILURE);
        func.string = argv[argi];
        func.tree = tree;
        AcquireFuncParameters(&func);

        inputFuncs[argi-1] = func;
    }
    inputFuncCount = argc - 1;
}

void SliderTest(void)
{
#define SLIDER_WIDTH 400
#define HANDLE_RADIUS 10
    int startX = WIDTH/2 - SLIDER_WIDTH/2;
    int endX = WIDTH/2 + SLIDER_WIDTH/2;

    Slider *slider = SliderCreate(startX, endX, 150, -5.0f, 5.0f);

    //SliderGetValue(slider);

    SliderSetCurrentPos(slider);
    SliderDraw(slider);
}

int main(int argc, char *argv[])
{
    ParseInputFuncs(argc, argv);

    InitWindow(WIDTH, HEIGHT, "Decmoc");
    SetTargetFPS(20);

    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(BACKGROUND);

            SetCurrentScale();
            SetCurrentOfssets();

            // TODO: grid fileds became messed up.
            //RenderGridField(GRID_SIZE / 4, GRID_COLOR2);
            //RenderGridField(GRID_SIZE, GRID_COLOR1);

            SliderTest();

            RenderFuncLabels();
            RenderAxes();
            RenderAxesNumbers();
            RenderGraphs();
        EndDrawing();
    }

    CloseWindow();
}
