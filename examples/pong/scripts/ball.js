/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    The ball always moves forward. The collision determines its trajectory depending on what it hits:
        -If it collides with the goal, it returns to the starting point.
        -If it collides with the wall, it bounces along the y-axis.
        -If it collides with the paddle, it bounces along the x-axis and accelerates.
*/

function motion(ball) {
    advance(ball);
}

function collision(ball, other) {
    if (other.group == "goal") {
        to_origin(ball);
    } else if (other.group == "wall") {
        bounce_y(ball);
    } else if (other.group == "paddle") {
        bounce_x(ball);
        accelerate(ball, 5);
    }
}