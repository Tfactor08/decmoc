#include "slider.h"
#include "../utils/basic_utils.c"

#define SLIDER_LABEL_MAX 10
#define HANDLE_RADIUS_DEFAULT 10

// TODO: allow setting slider position by clicking on the line

struct Slider {
    int width;
    int startX;
    int endX;
    int posY;
    float min;
    float max;
    float currentValue;
    bool isBeingDragged;
    Vector2 handlePos;
    float handleRadius;
    Color handleColor;
    char label[SLIDER_LABEL_MAX];
};

static bool MouseOnHandler(Slider *slider)
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
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            slider->isBeingDragged = false;
    } else if (MouseOnHandler(slider)) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            slider->isBeingDragged = true;
        slider->handleColor = BLUE;
    } else {
        slider->handleColor = BLACK;
    }
}

static void SetCurrentPos(Slider *slider)
{
    int newPos;
    int mouseX = GetMouseX();
    if (mouseX > slider->endX) newPos = slider->endX;
    else if (mouseX < slider->startX) newPos = slider->startX;
    else newPos = mouseX;
    slider->handlePos.x = newPos;
}

static void SetCurrentValue(Slider *slider)
{
    float magnitude = slider->max - slider->min;
    float handleX = slider->handlePos.x - slider->startX; // handle's x pos relative to the slider (not screen)
    float value = (magnitude * (handleX / slider->width)) + slider->min;
    slider->currentValue = value;
}

Slider *SliderCreate(int startX, int endX, int posY, float min, float max, const char *label)
{
    Slider *slider = malloc(sizeof(Slider));
    MALLOC_CHECK(slider);

    float handleRadius;
#ifdef HANDLE_RADIUS
    handleRadius = HANDLE_RADIUS;
#else
    handleRadius = HANDLE_RADIUS_DEFAULT;
#endif // HANDLE_RADIUS
    int width = endX - startX;
    float magnitude = max - min;
    Vector2 handlePos = {
        .x = startX + width * ((1 - min) / magnitude), // Position the handle at value 1
        .y = posY
    };

    *slider = (Slider) {
        .width = width,
        .startX = startX,
        .endX = endX,
        .posY = posY,
        .min = min,
        .max = max,
        .currentValue = 1.0f,
        .isBeingDragged = false,
        .handlePos = handlePos,
        .handleRadius = handleRadius
    };
    strncpy(slider->label, label, SLIDER_LABEL_MAX);
    return slider;
}

float SliderGetValue(Slider *slider)
{
    return slider->currentValue;
}

// TODO: there must be better naming!
// This function must be called on every frame
void SliderSetCurrentState(Slider *slider)
{
    SetCurrentMode(slider);
    if (slider->isBeingDragged) {
        SetCurrentPos(slider);
        SetCurrentValue(slider);
    }
}

void SliderDraw(Slider *slider)
{
    DrawText(slider->label, slider->startX, slider->posY - 20, 15, BLACK);
    DrawLine(slider->startX, slider->posY, slider->endX, slider->posY, BLACK);
    DrawCircle(slider->handlePos.x, slider->handlePos.y, slider->handleRadius, slider->handleColor);
}

void SliderDestroy(Slider *slider)
{
    free(slider);
}
