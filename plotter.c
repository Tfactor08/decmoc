#include <raylib.h>
#include <math.h>

#include "parser.c"

#define WIDTH 800
#define HEIGHT 600

Vector2 screen(float x, float y, int scale)
{
    return (Vector2) {
        .x = (x + scale)/(2*scale) * WIDTH,
        .y = (1 - (y + scale)/(2*scale)) * HEIGHT
    };
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s expression\n", argv[0]);
        exit(1);
    }

    char expression[256];
    strncpy(expression, argv[1], strlen(expression));
    NodeTree *tree = parse(expression);

#if 1
    InitWindow(WIDTH, HEIGHT, "Plotter");
    SetTargetFPS(5);

    RenderTexture2D texture = LoadRenderTexture(WIDTH, HEIGHT);
    
    BeginTextureMode(texture);
    
    DrawLine(0, HEIGHT/2, WIDTH, HEIGHT/2, RAYWHITE);
    DrawLine(WIDTH/2, 0, WIDTH/2, HEIGHT, RAYWHITE);
    
    EndTextureMode();

    int scale = 1;
    float wheel;
    while (WindowShouldClose() == 0) {
        if (GetKeyPressed() == KEY_ENTER) {
            printf("f(x): ");
            fgets(expression, sizeof(expression), stdin);
            expression[strcspn(expression, "\n")] = '\0';
            printf("%s\n", expression);
            tree = parse(expression);
            ClearBackground(BLACK);
        }

        BeginDrawing();

        DrawTextureRec(texture.texture, (Rectangle){ 0, 0, WIDTH, HEIGHT }, (Vector2){ 0, 0 }, WHITE);

        if (wheel = GetMouseWheelMove()) {
            scale += -wheel;
            ClearBackground(BLACK);
        }

        for (float x = -scale; x <= scale; x += 0.005) {
            float y = eval(tree, x);
            DrawPixelV(screen(x, y, scale), RED);
        }

        EndDrawing();
    }

    CloseWindow();
#endif

    return 0;
}
