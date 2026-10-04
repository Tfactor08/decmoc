#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "parser.h"
#include "slider.h"
#include "utils/basic_utils.c"
#include "utils/vector_utils.c"

#define ASPECT_W 4.0f
#define ASPECT_H 3.0f
#define FACTOR 288.0f
#define WIDTH FACTOR * ASPECT_W
#define HEIGHT FACTOR * ASPECT_H
#define LEFT_PANEL_RATIO 0.3f
#define RIGHT_PANEL_RATIO (1.0f - LEFT_PANEL_RATIO)

#define BACKGROUND (Color) { 0xff, 0xff, 0xff, 0xff }
#define FOREGROUND (Color) { 0x2e, 0x2e, 0x2e, 0xff }

#define GRID_SIZE 80.0f
#define GRID_COLOR1 (Color) { 0xb8, 0xb8, 0xb8, 0xff }
#define GRID_COLOR2 (Color) { 0xe7, 0xe7, 0xe7, 0xff }

#define LINE_THICKNESS 3.0f
#define TEXT_SIZE 20.0f

#define SLIDER_START_X 20
#define SLIDER_END_X 200
#define SLIDER_POS_Y 100

#define MAX_FUNCS (2 << 4)
#define XSTEP 0.2f
#define PARAM_MIN -5.0f
#define PARAM_MAX 5.0f

typedef struct {
    const char *string;
    ParserTree *expr_tree;
    ParserParams params;
    Slider *sliders[];
} Func;

Func *inputFuncs[MAX_FUNCS];
size_t inputFuncCount;

float scale = 1.0f;
float offsetX = 0.0f, offsetY = 0.0f;
float startPanX = 0.0f, startPanY = 0.0f;

const Color graphColors[] = { RED, GREEN, PURPLE };
const size_t graphColorCount = sizeof(graphColors) / sizeof(*graphColors);

// NOTE: World-to-Screen transform implementation (Screen* functions) is quite chancy,
//       and unfortunately I couldn't be able to find better solutions. One of the flaws
//       is that in some cases the rendering functions have to "know" about screen offsets
//       (offsetX and offsetY) in order to draw objects which must be rendered edge-to-edge:  
//       for example, "RenderAxes" has to add "offsetY" to the y coordinate of the vertical
//       axis and "offsetX" to the the x coordinate of the horizontal axis
//       (similar situation in "RenderGraphs").

/* Convert coordinates in the [-scale, scale] range to the corresponding screen coordinates (ignoring the left panel) */
Vector2 Screen(float x, float y)
{
    return (Vector2) {
        .x = ((x + scale - offsetX)/(2*scale) * RIGHT_PANEL_RATIO*WIDTH) + LEFT_PANEL_RATIO*WIDTH,
        .y = (1 - (y + scale - offsetY)/(2*scale)) * HEIGHT
    };
}

/* Convert coordinates in the [-scale, scale] range to the corresponding screen coordinates (ignoring the left panel) (vector version) */
Vector2 ScreenV(Vector2 *p)
{
    return (Vector2) {
        .x = ((p->x + scale - offsetX)/(2*scale) * RIGHT_PANEL_RATIO*WIDTH) + LEFT_PANEL_RATIO*WIDTH,
        .y = (1 - (p->y + scale - offsetY)/(2*scale)) * HEIGHT
    };
}

/* Render a grid in the center of the screen (if the global offsets are zero) */
void RenderGridField(int gridSize, Color color)
{
    float gridSizeNorm = ((float) gridSize / WIDTH) * scale;
    float gridOffsetX = fmod(offsetX, gridSizeNorm); // The amount of pixels need to be right shifted for centering
    float gridOffsetY = fmod(offsetY, gridSizeNorm); // The amount of pixels need to be up shifted for centering
    // Vertical lines
    for (float x = -scale - gridOffsetX; x <= scale - gridOffsetX; x += gridSizeNorm)
        DrawLineV(Screen(x, -scale), Screen(x, scale), color);
    // Horizontal lines
    for (float y = -scale - gridOffsetY; y <= scale - gridOffsetY; y += gridSizeNorm)
        DrawLineV(Screen(-scale, y), Screen(scale, y), color);
}

void RenderAxes(void)
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

void RenderAxesNumbers(void)
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
        if (nY == 0) continue; // Avoid rendering 0 twice
        Vector2 textPos = Screen(0.0f + margin, nY);
        itoa(nY, num, sizeof(num));
        DrawText(num, textPos.x, textPos.y, TEXT_SIZE, BLACK);
    }
}

void RenderFuncLabels(void)
{
    const int margin = 10;
    const int interval = 25;
    for (size_t i = 0; i < inputFuncCount; i++) {
        int x = 0 + margin;
        int y = i*interval + margin;
        Color color = graphColors[i % graphColorCount];
        DrawText(inputFuncs[i]->string, x, y, TEXT_SIZE, color);
    }
}

void RenderGraphs(void)
{
    float a = -scale;
    float b = scale;
    for (size_t funci = 0; funci < inputFuncCount; funci++) {
        Func *func = inputFuncs[funci];
        ParserTree *tree = func->expr_tree;
        Color color = graphColors[funci % graphColorCount];
        float y = parser_eval(tree, a + offsetX);
        Vector2 firstPoint = (Vector2) { a + offsetX, y }, secondPoint;
        for (float x = a; x <= b; x += XSTEP) {
            y = parser_eval(tree, x + offsetX);
            secondPoint = (Vector2) { x + offsetX, y };
            DrawLineEx(ScreenV(&firstPoint),
                       ScreenV(&secondPoint),
                       LINE_THICKNESS, color);
            firstPoint = secondPoint;
        }
        // Make sure last line is rendered as well regardless of the "b" and "XSTEP" values
        y = parser_eval(tree, b + offsetX);
        secondPoint = (Vector2) { b + offsetX, y };
        DrawLineEx(ScreenV(&firstPoint),
                   ScreenV(&secondPoint),
                   LINE_THICKNESS, color);
    }
}

void SetCurrentScale(void)
{
    float wheel = 0.0f;
    if ((wheel = GetMouseWheelMove())) {
        scale += -wheel / 2;
        ClearBackground(BACKGROUND);
    }
}

void SetCurrentOfssets(void)
{
    float mouseX = (float) GetMouseX();
    float mouseY = (float) GetMouseY();
    
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        startPanX = mouseX;
        startPanY = mouseY;
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        // TODO: factor out 300 magic constant
        offsetX -= (mouseX - startPanX) / 300.0f * scale;
        offsetY += (mouseY - startPanY) / 300.0f * scale;

        startPanX = mouseX;
        startPanY = mouseY;
    }
}

// TODO: obsolete function?
/* Acquire parameters for a function (if any) from user */
void AcquireFuncParameters(Func *func)
{
    ParserTree *tree = func->expr_tree;
    ParserParams params = parser_get_params(tree);
    for (size_t parami = 0; parami < params.count; parami++) {
        char param_name = params.param_list[parami];
        float param_value = 0;
        printf("[%s] %c: ", func->string, param_name);
        scanf("%f", &param_value);
        parser_set_param(tree, param_name, param_value);
    }
}

void CreateSliders(Func *func)
{
    ParserParams params = func->params;
    for (size_t parami = 0; parami < params.count; parami++) {
        char *label = CHAR_TO_STR(params.param_list[parami]);
        int posY = SLIDER_POS_Y + parami*50;
        Slider *slider = SliderCreate(
            SLIDER_START_X, SLIDER_END_X, posY, PARAM_MIN, PARAM_MAX, label
        );
        func->sliders[parami] = slider;
    }
}

void ParseInputFuncs(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s FUNCTION...\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    for (int argi = 1; argi < argc; argi++) {
        ParserTree *tree = parser_parse(argv[argi]);
        if (tree == NULL)
            exit(EXIT_FAILURE);
        ParserParams params = parser_get_params(tree);
        Func *func = malloc(sizeof(Func) + params.count * sizeof(Slider *));
        func->string = argv[argi];
        func->expr_tree = tree;
        func->params = params;
        CreateSliders(func);
        inputFuncs[argi-1] = func;
    }
    inputFuncCount = argc - 1;
}

// TODO: better naming
// NOTE: these can be textures
void RenderUIElements()
{
    DrawLine(LEFT_PANEL_RATIO*WIDTH, 0,
             LEFT_PANEL_RATIO*WIDTH, HEIGHT, GRAY); // Function list/Plotting area separator
}

// Must be called on every frame
void RenderSlider(Slider *slider)
{
    SliderSetCurrentPos(slider);
    SliderDraw(slider);
}

void SetInputFuncParam(Func *func, Slider *slider, size_t parami)
{
    char param = func->params.param_list[parami];
    float value = SliderGetValue(slider);
    parser_set_param(func->expr_tree, param, value);
}

void ProcessSliders()
{
    for (size_t funci = 0; funci < inputFuncCount; funci++) {
        Func *func = inputFuncs[funci];
        for (size_t parami = 0; parami < func->params.count; parami++) {
            Slider *slider = func->sliders[parami];
            SetInputFuncParam(func, slider, parami);
            RenderSlider(slider);
        }
    }
}

bool InPlottingArea()
{
    return GetMouseX() > LEFT_PANEL_RATIO*WIDTH;
}

int main(int argc, char *argv[])
{
    ParseInputFuncs(argc, argv);

    InitWindow(WIDTH, HEIGHT, "Decmoc");
    SetTargetFPS(25);

    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(BACKGROUND);
            RenderUIElements();

            if (InPlottingArea()) {
                SetCurrentScale();
                SetCurrentOfssets();
            }

            // TODO: grid fileds became messed up
            //RenderGridField(GRID_SIZE / 4, GRID_COLOR2);
            //RenderGridField(GRID_SIZE, GRID_COLOR1);

            ProcessSliders();

            RenderFuncLabels();
            RenderAxes();
            RenderAxesNumbers();
            RenderGraphs();
        EndDrawing();
    }

    CloseWindow();
}
