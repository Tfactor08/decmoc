#include "slider.h"
#include "../utils/basic_utils.c"

#define SLIDER_WIDTH_DEFAULT  400
#define HANDLE_RADIUS_DEFAULT 10

// TODO: allow setting slider position by clicking on the line

struct Slider {
    int width;
    int startX;
    int endX;
    int posY;
    float min;
    float max;
    bool isBeingDragged;
    Vector2 handlePos;
    float handleRadius;
};

static bool MouseOnSlider(Slider *slider)
{
    return CheckCollisionPointCircle(
        GetMousePosition(),
        slider->handlePos,
        slider->handleRadius
    );
}

static void SetCurrentMode(Slider *slider)
{
    if (slider->isBeingDragged) {
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            slider->isBeingDragged = false;
            float value = SliderGetValue(slider);
            printf("%f\n", value);
        }
    } else if (MouseOnSlider(slider) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        slider->isBeingDragged = true;
    }
}

Slider *SliderCreate(int startX, int endX, int posY, float min, float max)
{
    Slider *slider = malloc(sizeof(Slider));
    MALLOC_CHECK(slider);

    int width;
    float handleRadius;
#ifdef SLIDER_WIDTH
    width = SLIDER_WIDTH,
#else
    width = SLIDER_WIDTH_DEFAULT,
#endif // SLIDER_WIDTH
#ifdef HANDLE_RADIUS
    handleRadius = HANDLE_RADIUS;
#else
    handleRadius = HANDLE_RADIUS_DEFAULT;
#endif // HANDLE_RADIUS
    Vector2 handlePos = {
        .x = startX + width/2,
        .y = posY
    };

    *slider = (Slider) {
        .width = width,
        .startX = startX,
        .endX = endX,
        .posY = posY,
        .min = min,
        .max = max,
        .isBeingDragged = false,
        .handlePos = handlePos,
        .handleRadius = handleRadius
    };
    return slider;
}

float SliderGetValue(Slider *slider)
{
    float magnitude = slider->max - slider->min;
    float handleX = slider->handlePos.x - slider->startX; // handle's x pos relative to the slider (not screen)
    float value = (magnitude * (handleX / slider->width)) + slider->min;
    return value;
}

// TODO: there must be better naming!
// This function must be called on every frame.
void SliderSetCurrentPos(Slider *slider)
{
    SetCurrentMode(slider);
    if (slider->isBeingDragged) {
        int newPos;
        int mouseX = GetMouseX();
        if (mouseX > slider->endX) newPos = slider->endX;
        else if (mouseX < slider->startX) newPos = slider->startX;
        else newPos = mouseX;
        slider->handlePos.x = newPos;
    }
}

void SliderDraw(Slider *slider)
{
    DrawCircle(slider->handlePos.x, slider->handlePos.y, slider->handleRadius, BLACK);
    DrawLine(slider->startX, slider->posY, slider->endX, slider->posY, BLACK);
}

void SliderDestroy(Slider *slider)
{
    free(slider);
}
