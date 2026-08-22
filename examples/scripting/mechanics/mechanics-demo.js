/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function draw(demo)
{
    draw_text(16, 16, "FLX Mechanics Demo", 18, "white");
    draw_text(16, 42, "W/UP: thrust   A/D or LEFT/RIGHT: rotate   SPACE: spawn probe", 10, "white");

    draw_text(16, 270, "DIRECT + apply_velocity: rotation does not curve the path", 10, "cyan");
    draw_text(16, 360, "apply_speed / restore_speed + reflection", 10, "green");
}
