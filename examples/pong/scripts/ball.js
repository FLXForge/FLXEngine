/// <reference path="../../../../scripting tools/flx.d.ts" />

/*
    The ball always moves forward. The collision determines its trajectory depending on what it hits:
        -If it collides with the goal, it returns to the starting point.
        -If it collides with the wall, it bounces along the y-axis.
        -If it collides with the paddle, it bounces along the x-axis and accelerates.
*/

function motion(self) {
    advance(self);
}

function collision(self, other) {
    if (other.group == "goal") {
        to_origin(self);
    } else if (other.group == "wall") {
        bounce_y(self);
    } else if (other.group == "paddle") {
        bounce_x(self);
        accelerate(self, 5);
    }
}