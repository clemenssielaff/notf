#include "graphics/mytest.hpp"

#include <math.h>

#include "nanovg/nanovg.h"

void doit(NVGcontext* vg)
{
    float pos_x = 10;
    float pos_y = 10;
    float width = 150;
    float height = 100;
    float mouse_x = 0;
    float mouse_y = 0;
    float time;

    NVGpaint gloss, bg;
    float ex = width * 0.23f;
    float ey = height * 0.5f;
    float lx = pos_x + ex;
    float ly = pos_y + ey;
    float rx = pos_x + width - ex;
    float ry = pos_y + ey;
    float dx, dy, d;
    float br = (ex < ey ? ex : ey) * 0.5f;
    float blink = 1 - pow(sinf(time * 0.5f), 200) * 0.8f;

    bg = nvgLinearGradient(vg, pos_x, pos_y + height * 0.5f, pos_x + width * 0.1f, pos_y + height, nvgRGBA(0, 0, 0, 32), nvgRGBA(0, 0, 0, 16));
    nvgBeginPath(vg);
    nvgEllipse(vg, lx + 3.0f, ly + 16.0f, ex, ey);
    nvgEllipse(vg, rx + 3.0f, ry + 16.0f, ex, ey);
    nvgFillPaint(vg, bg);
    nvgFill(vg);

    bg = nvgLinearGradient(vg, pos_x, pos_y + height * 0.25f, pos_x + width * 0.1f, pos_y + height, nvgRGBA(220, 220, 220, 255), nvgRGBA(128, 128, 128, 255));
    nvgBeginPath(vg);
    nvgEllipse(vg, lx, ly, ex, ey);
    nvgEllipse(vg, rx, ry, ex, ey);
    nvgFillPaint(vg, bg);
    nvgFill(vg);

    dx = (mouse_x - rx) / (ex * 10);
    dy = (mouse_y - ry) / (ey * 10);
    d = sqrtf(dx * dx + dy * dy);
    if (d > 1.0f) {
        dx /= d;
        dy /= d;
    }
    dx *= ex * 0.4f;
    dy *= ey * 0.5f;
    nvgBeginPath(vg);
    nvgEllipse(vg, lx + dx, ly + dy + ey * 0.25f * (1 - blink), br, br * blink);
    nvgFillColor(vg, nvgRGBA(32, 32, 32, 255));
    nvgFill(vg);

    dx = (mouse_x - rx) / (ex * 10);
    dy = (mouse_y - ry) / (ey * 10);
    d = sqrtf(dx * dx + dy * dy);
    if (d > 1.0f) {
        dx /= d;
        dy /= d;
    }
    dx *= ex * 0.4f;
    dy *= ey * 0.5f;
    nvgBeginPath(vg);
    nvgEllipse(vg, rx + dx, ry + dy + ey * 0.25f * (1 - blink), br, br * blink);
    nvgFillColor(vg, nvgRGBA(32, 32, 32, 255));
    nvgFill(vg);

    gloss = nvgRadialGradient(vg, lx - ex * 0.25f, ly - ey * 0.5f, ex * 0.1f, ex * 0.75f, nvgRGBA(255, 255, 255, 128), nvgRGBA(255, 255, 255, 0));
    nvgBeginPath(vg);
    nvgEllipse(vg, lx, ly, ex, ey);
    nvgFillPaint(vg, gloss);
    nvgFill(vg);

    gloss = nvgRadialGradient(vg, rx - ex * 0.25f, ry - ey * 0.5f, ex * 0.1f, ex * 0.75f, nvgRGBA(255, 255, 255, 128), nvgRGBA(255, 255, 255, 0));
    nvgBeginPath(vg);
    nvgEllipse(vg, rx, ry, ex, ey);
    nvgFillPaint(vg, gloss);
    nvgFill(vg);
}
