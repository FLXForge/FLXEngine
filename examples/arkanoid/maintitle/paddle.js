/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function motion(paddle) {
    const balls = find_name("title_ball");

    if (balls.length > 0) {
        follow_x(paddle, balls[0]);
    }
}
