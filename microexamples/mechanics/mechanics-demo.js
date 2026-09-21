/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function draw(demo)
{
    draw_text(105, 25, "FLX Mechanics Demo", 18, "white");
    draw_text(190, 47, "W/UP: thrust   A/D or LEFT/RIGHT: rotate   SPACE: spawn probe", 10, "white");

    draw_text(175, 275, "DIRECT + apply_velocity: rotation does not curve the path", 10, "cyan");
    draw_text(130, 365, "apply_speed / restore_speed + reflection", 10, "green");
}
