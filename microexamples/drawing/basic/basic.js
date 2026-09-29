/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function draw(object)
{
    // World coordinates.
    draw_pixel(80, 80, "white");
    draw_line(60, 120, 100, 120, "white");
    draw_rectangle(80, 170, 50, 30, "white");
    draw_text(80, 220, "WORLD", 10, "white");

    // Local coordinates relative to object origin (320, 240).
    draw_pixel(object, 0, -80, "yellow");
    draw_line(object, -20, -40, 20, -40, "yellow");
    draw_rectangle(object, 0, 10, 50, 30, "yellow");
    draw_text(object, 0, 60, "LOCAL", 10, "yellow");
}