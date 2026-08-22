/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function born(ball)
{
    ball.local["elapsed"] = 0;
    ball.local["fast"] = 0;
}

function action(ball)
{
    ball.local["elapsed"] += delta();

    if (ball.local["fast"] == 0 && ball.local["elapsed"] >= 2.0) {
        apply_speed(ball, 80);
        ball.local["fast"] = 1;
        ball.local["elapsed"] = 0;
    }
    else if (ball.local["fast"] == 1 && ball.local["elapsed"] >= 2.0) {
        restore_speed(ball);
        ball.local["fast"] = 0;
        ball.local["elapsed"] = 0;
    }
}

function motion(ball)
{
    advance(ball);

    if (ball.x >= 300 || ball.x <= 20) {
        reflect_x(ball);
    }
}
